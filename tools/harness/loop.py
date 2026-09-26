#!/usr/bin/env python3
"""Level-1 generation loop: a local model grinds through tasks in tasks/,
one at a time, retrying without accumulating conversation history. See
README.md for the two-level design; this file is only level 1 — it never
decides that a passing task is actually *correct* against the spec, only
that it satisfies its own acceptance command. Level 2 (a Claude session,
Opus by request) reviews runs/<task>/ afterwards and writes the next batch
of task cards, or a revision note consumed here as extra_note.

Task card format (YAML frontmatter + markdown body):

    ---
    id: m1-token-predicates
    output: tools/cgen/src/engine/token_predicates.c
    acceptance:
      - "gcc -std=c11 -Wall -Wextra -c {output} -o /tmp/keel-harness-check.o"
    max_attempts: 4
    ---
    (prompt body, sent to the model as-is)

`{output}` in an acceptance command is substituted with the real destination
path. Acceptance commands run in the repository root, in order; the first
failure stops the chain and its stderr/stdout feeds the next attempt.
"""

import json
import re
import shutil
import subprocess
import sys
import time
import urllib.request
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
HARNESS_DIR = Path(__file__).resolve().parent
TASKS_DIR = HARNESS_DIR / "tasks"
RUNS_DIR = HARNESS_DIR / "runs"

OLLAMA_URL = "http://localhost:11434/api/generate"
DEFAULT_MAX_ATTEMPTS = 4
ERROR_TAIL_CHARS = 4000

SYSTEM_PROMPT = (
    "You write portable C11 for the keel project. You output only the "
    "requested C source for the single file asked, with no markdown "
    "fences and no prose before or after the code. You implement exactly "
    "the contract given — do not invent behavior, files, or helpers "
    "beyond what is asked."
)

FENCE_RE = re.compile(r"```[a-zA-Z]*\n(.*?)\n```", re.DOTALL)


def strip_fences(text: str) -> str:
    text = text.strip()
    m = FENCE_RE.search(text)
    return m.group(1) if m else text


def load_task(path: Path) -> dict:
    text = path.read_text()
    if not text.startswith("---"):
        raise ValueError(f"{path}: missing YAML frontmatter")
    _, fm, body = text.split("---", 2)
    meta = {}
    key = None
    for line in fm.strip().splitlines():
        if line.startswith("  - "):
            if not isinstance(meta.get(key), list):
                meta[key] = []
            meta[key].append(line[4:].strip().strip('"'))
        elif ":" in line:
            key, val = line.split(":", 1)
            key, val = key.strip(), val.strip()
            meta[key] = val if val else None
    meta["body"] = body.strip()
    meta["_path"] = path
    return meta


def ns_to_tps(count: int, duration_ns: int) -> float:
    if duration_ns <= 0:
        return 0.0
    return count / (duration_ns / 1e9)


SPEED_PROBE_PROMPT = (
    "Write a C function `int add(int a, int b)` that returns a + b. "
    "Output only the function, nothing else."
)


def estimate_speed(model: str) -> tuple[float, float]:
    """A quick throughput reading before committing to a batch. ollama is
    one shared instance — another process against the same model (an
    editor's chat panel, say) drags this number down from its usual
    baseline, and this is how the loop notices before spending a whole
    task's worth of attempts at that slower rate."""
    payload = {
        "model": model,
        "prompt": SPEED_PROBE_PROMPT,
        "stream": False,
        "options": {"temperature": 0, "num_predict": 64},
    }
    req = urllib.request.Request(
        OLLAMA_URL,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=120) as resp:
        body = json.loads(resp.read())
    decode = ns_to_tps(body.get("eval_count", 0), body.get("eval_duration", 0))
    prefill = ns_to_tps(
        body.get("prompt_eval_count", 0), body.get("prompt_eval_duration", 0)
    )
    return decode, prefill


def call_ollama(model: str, prompt: str, temperature: float = 0) -> dict:
    payload = {
        "model": model,
        "system": SYSTEM_PROMPT,
        "prompt": prompt,
        "stream": False,
        "options": {"temperature": temperature},
    }
    req = urllib.request.Request(
        OLLAMA_URL,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=1800) as resp:
        return json.loads(resp.read())


def run_acceptance(commands: list[str], output_real_path: Path) -> tuple[bool, str]:
    for cmd_tpl in commands:
        cmd = cmd_tpl.format(output=output_real_path)
        try:
            r = subprocess.run(
                cmd, shell=True, cwd=REPO_ROOT,
                capture_output=True, text=True, timeout=300,
            )
        except subprocess.TimeoutExpired as e:
            # A generated file that hangs (an infinite loop the oracle's own
            # `timeout` forgot to guard against, or didn't exist for) must
            # not crash the whole batch — it is exactly as ordinary a
            # failure as a wrong answer, and the next attempt needs the same
            # chance to fix it.
            out = (e.stdout or b"").decode(errors="replace") if isinstance(e.stdout, bytes) else (e.stdout or "")
            err = (e.stderr or b"").decode(errors="replace") if isinstance(e.stderr, bytes) else (e.stderr or "")
            return False, f"$ {cmd}\nTIMEOUT after {e.timeout}s\n{out}\n{err}"
        if r.returncode != 0:
            return False, f"$ {cmd}\n{r.stdout}\n{r.stderr}"
    return True, "ok"


def run_task(task: dict, model: str) -> str:
    task_id = task["id"]
    output_rel = Path(task["output"])
    output_real = REPO_ROOT / output_rel
    max_attempts = int(task.get("max_attempts", DEFAULT_MAX_ATTEMPTS))
    acceptance = task["acceptance"]

    run_dir = RUNS_DIR / task_id
    run_dir.mkdir(parents=True, exist_ok=True)

    prompt = task["body"]
    last_code = None
    last_error = None
    stuck_repeats = 0
    log = []

    for attempt in range(1, max_attempts + 1):
        # temperature=0 only on the first shot, for a reproducible baseline.
        # A retry at temperature 0 given the exact same prompt reproduces
        # the exact same (still broken) code — a fixed point that burns the
        # rest of max_attempts for nothing. Retries get deliberate entropy,
        # more of it each time the model proves it is stuck.
        temperature = 0 if attempt == 1 else min(0.3 + 0.15 * stuck_repeats, 0.9)
        print(
            f"[{task_id}] attempt {attempt}/{max_attempts} (temperature={temperature:.2f})",
            flush=True,
        )

        if attempt == 1:
            full_prompt = prompt
        else:
            stuck_note = ""
            if stuck_repeats > 0:
                stuck_note = (
                    "\n\nYour last attempt produced the exact same file as the "
                    "one before it — that \"fix\" changed nothing. Do not "
                    "resubmit the same code. Find a genuinely different cause "
                    "for the failure below."
                )
            full_prompt = (
                f"{prompt}\n\n---\n\nYour previous attempt produced this file:\n\n"
                f"```c\n{last_code}\n```\n\n"
                f"Running it failed with:\n\n```\n{last_error[:ERROR_TAIL_CHARS]}\n```"
                f"{stuck_note}\n\n"
                f"Fix it. Output the complete corrected file, nothing else."
            )

        t0 = time.monotonic()
        resp = call_ollama(model, full_prompt, temperature=temperature)
        elapsed = time.monotonic() - t0
        code = strip_fences(resp.get("response", ""))
        if attempt > 1 and code == last_code:
            stuck_repeats += 1
        else:
            stuck_repeats = 0
        decode_tps = ns_to_tps(resp.get("eval_count", 0), resp.get("eval_duration", 0))
        prefill_tps = ns_to_tps(
            resp.get("prompt_eval_count", 0), resp.get("prompt_eval_duration", 0)
        )
        print(
            f"[{task_id}] attempt {attempt}: {elapsed:.1f}s, "
            f"{decode_tps:.1f} tok/s decode, {prefill_tps:.1f} tok/s prefill",
            flush=True,
        )

        attempt_dir = run_dir / f"attempt-{attempt}"
        attempt_dir.mkdir(exist_ok=True)
        scratch_path = attempt_dir / output_rel.name
        scratch_path.write_text(code)

        # acceptance runs against the real path, so #include-relative and
        # make-relative paths resolve — but we only copy on success.
        output_real.parent.mkdir(parents=True, exist_ok=True)
        backup = None
        if output_real.exists():
            backup = output_real.read_text()
        output_real.write_text(code)

        ok, detail = run_acceptance(acceptance, output_real)

        log.append({
            "attempt": attempt, "elapsed_s": round(elapsed, 1), "temperature": temperature,
            "decode_tps": round(decode_tps, 1), "prefill_tps": round(prefill_tps, 1),
            "pass": ok, "detail": detail if not ok else "ok",
        })
        (attempt_dir / "result.txt").write_text(detail)

        if ok:
            print(f"[{task_id}] PASS on attempt {attempt}")
            (run_dir / "log.json").write_text(json.dumps(log, indent=2))
            return "done", log

        print(f"[{task_id}] FAIL: {detail.splitlines()[0] if detail else ''}")
        last_code, last_error = code, detail
        if backup is not None:
            output_real.write_text(backup)
        else:
            output_real.unlink(missing_ok=True)

    (run_dir / "log.json").write_text(json.dumps(log, indent=2))
    (run_dir / "blocked-report.md").write_text(
        f"# {task_id} — blocked after {max_attempts} attempts\n\n"
        + "\n\n".join(
            f"## attempt {e['attempt']} ({e['elapsed_s']}s)\n```\n{e['detail']}\n```"
            for e in log
        )
    )
    print(f"[{task_id}] BLOCKED after {max_attempts} attempts — see {run_dir}")
    return "blocked", log


def main():
    if len(sys.argv) < 2:
        print("usage: loop.py <model> [task-file ...]", file=sys.stderr)
        sys.exit(1)
    model = sys.argv[1]
    task_files = [Path(p) for p in sys.argv[2:]] or sorted(TASKS_DIR.glob("*.md"))
    task_files = [
        p for p in task_files
        if not p.name.startswith(("bench-", "validation-"))
    ]

    print(f"probing {model}...", flush=True)
    probe_decode, probe_prefill = estimate_speed(model)
    print(
        f"current speed: {probe_decode:.1f} tok/s decode, "
        f"{probe_prefill:.1f} tok/s prefill\n"
    )

    results = {}
    all_decode_tps = []
    for path in task_files:
        task = load_task(path)
        status, task_log = run_task(task, model)
        results[task["id"]] = status
        all_decode_tps.extend(e["decode_tps"] for e in task_log if e["decode_tps"] > 0)

    print("\n=== summary ===")
    print(f"speed probe (start of run): {probe_decode:.1f} tok/s decode")
    if all_decode_tps:
        avg = sum(all_decode_tps) / len(all_decode_tps)
        print(f"average over {len(all_decode_tps)} attempt(s): {avg:.1f} tok/s decode")
        if probe_decode > 0 and avg < probe_decode * 0.7:
            print(
                f"note: attempts ran ~{100 * (1 - avg / probe_decode):.0f}% "
                f"slower than the probe — something else may be sharing ollama"
            )
    for task_id, status in results.items():
        print(f"{status:8} {task_id}")


if __name__ == "__main__":
    main()
