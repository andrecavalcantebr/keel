#!/usr/bin/env python3
"""Brings tools/cgen/diag-messages.md up to date with the documents, asking
Claude (Haiku 5.5) for the English condition and the message of each row that
is new or whose condition changed. See --help.

    python3 tools/cgen/diag-sync.py --dry-run   # what would be asked, no API call
    python3 tools/cgen/diag-sync.py             # ask, write the rows, regenerate
    python3 tools/cgen/diag-sync.py --lock      # accept the file as it is now
    python3 tools/cgen/diag-sync.py --review    # an opinion on each message, nothing written
"""
import argparse
import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
LOCK = HERE / 'diag-messages.lock.json'
DESIGN = ROOT / 'design/diag-design.md'
MODEL = 'claude-haiku-5-5'

sys.dont_write_bytecode = True   # importing gen-diags.py leaves no __pycache__ in tools/cgen
spec = importlib.util.spec_from_file_location('gen_diags', HERE / 'gen-diags.py')
gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)

SECTION_SPEC = 'keel-spec §6.2'
SECTION_TOOL = 'cgen-tool-spec §7.1'
SECTION_OWN = 'cgen'

HELP = """\
Keeps tools/cgen/diag-messages.md in step with the normative tables, and asks
Claude for the text that a person would otherwise write by hand.

how it works:
  1. reads the rows of keel-spec.md §6.2 and cgen-tool-spec.md §7.1 (identifier,
     condition in pt-BR, severity), the support file, and the lock file
     tools/cgen/diag-messages.lock.json, which keeps a hash of the pt-BR
     condition each row was last synced with;
  2. a row is to be asked when its identifier is not in the support file yet,
     or when its condition no longer matches the hash in the lock; a row of the
     support file whose identifier left the documents is dropped;
  3. for each row to be asked, sends to Claude the rules of a message
     (diag-design §4, [G2]), examples from the support file, the row, and the
     spec section its Contract column names. Claude answers with the condition
     in English and, for a diagnostic the translation emits, the message (a
     format string: each %s takes the next argument). An existing message is
     kept unless the new condition makes it wrong, and its number of %s never
     changes, because the C code that emits it passes that many arguments;
  4. rewrites the support file in the order of the documents, updates the lock
     and runs gen-diags.py, which regenerates the .def files and refuses the
     result if it disagrees with the tables.

Review the outcome with `git diff`: the model proposes, the person decides.

--review is a separate mode: it sends every message (or the --id ones) to be
judged against the rules of [G2], and prints a Markdown report of the ones to
improve, each with a suggestion that keeps its %s. It writes nothing: a
suggestion worth taking is copied into diag-messages.md by hand, and then
gen-diags.py regenerates the code. Redirect the report to keep it:

  .venv/bin/python tools/cgen/diag-sync.py --review > /tmp/diag-review.md

requires the anthropic package and the variable ANTHROPIC_API_KEY, except with
--dry-run and --lock. Where the system Python is externally managed (PEP 668),
install the package in a venv and run the script with its Python:

  python3 -m venv .venv && .venv/bin/pip install anthropic
  .venv/bin/python tools/cgen/diag-sync.py --dry-run"""


def parse_args(argv):
    ap = argparse.ArgumentParser(
        prog='diag-sync.py', add_help=False, description=HELP,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('-h', '--help', '-help', action='help', help='show this help and exit')
    ap.add_argument('--dry-run', action='store_true',
                    help='list the rows that would be asked or dropped; no API call, nothing written')
    ap.add_argument('--lock', action='store_true',
                    help='record the current pt-BR conditions in the lock without asking: '
                         'the support file, as it is, is declared in step')
    ap.add_argument('--id', action='append', default=[], metavar='IDENT',
                    help='also ask this identifier, even if it is in step (repeatable)')
    ap.add_argument('--review', action='store_true',
                    help='ask for an opinion on every message against the rules of [G2] '
                         '(or only on the --id ones) and print a Markdown report of the ones '
                         'to improve, with a suggestion; writes nothing')
    ap.add_argument('--jobs', type=int, default=8, metavar='N',
                    help='requests in flight with --review (default: %(default)s)')
    ap.add_argument('--model', default=MODEL, help='the model to ask (default: %(default)s)')
    return ap.parse_args(argv)


def condition_hash(condition):
    return hashlib.sha256(condition.encode('utf-8')).hexdigest()[:16]


def table_rows(path, start, end):
    """(identifier, condition, severity, contract) of a normative table. The
    tool's table has no severity or contract column: all its rows are errors."""
    text = path.read_text(encoding='utf-8')
    a = text.index(start)
    b = text.index(end, a)
    rows = []
    for line in text[a:b].splitlines():
        if not line.startswith('| `'):
            continue
        cells = line.strip().strip('|').split(' | ')
        ident = cells[0].strip().strip('`')
        if len(cells) >= 5:
            rows.append((ident, ' | '.join(cells[1:-3]).strip(), cells[-3].strip().strip('`'),
                         cells[-1].strip()))
        else:
            rows.append((ident, ' | '.join(cells[1:]).strip(), 'error', ''))
    return rows


def spec_section(number):
    """The text of `### <number> ...` of keel-spec.md, up to the next heading
    of the same or a higher level; empty if there is none."""
    m = re.fullmatch(r'§?(\d+(?:\.\d+)*)', number.strip())
    if not m:
        return ''
    text = gen.SPEC.read_text(encoding='utf-8')
    head = re.search(r'^(#{2,4}) %s\s' % re.escape(m.group(1)), text, re.M)
    if not head:
        return ''
    level = len(head.group(1))
    tail = re.compile(r'^#{2,%d} \S' % level, re.M)
    nxt = tail.search(text, head.end())
    return text[head.start():nxt.start() if nxt else len(text)].strip()


def design_rules():
    text = DESIGN.read_text(encoding='utf-8')
    a = text.index('## 4. Como se escreve')
    b = text.index('\n## ', a + 1)
    return text[a:b].strip()


def emittable(rows):
    """The identifiers that can have a message: they are in the KDiagId enum."""
    live = {i for i, _, sev, _ in rows if sev != 'debug'}
    return live | set(gen.engine_tool_names())


def system_prompt(examples):
    lines = '\n'.join('- `%s` (%s): message: %s | condition: %s' % e for e in examples)
    return f"""You write the diagnostic texts of cgen, the translator of keel, a language of \
islands of keel in a sea of C, translated to C11/C23. The documents of the project are in \
Brazilian Portuguese; every text you write is in English, as the code is.

For each diagnostic you receive the identifier, its condition in Portuguese as the normative \
table writes it, its severity, and the section of the specification it belongs to. You give back:

- `condition`: the condition translated into English. One sentence, or two when the original has \
two; the same facts, nothing added, nothing dropped. Keep the code spans (`...`) exactly as \
written, and start with a capital letter, without a final period.
- `message`: what cgen prints when it finds the condition, following the rules below. It is a \
printf format string: each `%s` takes the next argument, which is text of the source (a name, \
a token, a spelling) or a short phrase from the tool. Use `%s` only where a part of the program \
fills the hole, at most three of them, and no other conversion. It starts in lower case and has \
no final period. When you are told that the diagnostic has no message, give an empty string.
- `notes`: empty, unless something in the row is ambiguous or seems wrong to you; then say what, \
in one or two sentences, for the person who reviews your answer.

The rules for a message (the design document of the diagnostics, in Portuguese):

{design_rules()}

Examples of rows already written:

{lines}"""


def ask(client, model, system, row, current, has_message, section_text):
    ident, condition, severity, contract = row
    parts = [f'identifier: `{ident}`', f'severity: {severity}',
             f'condition (pt-BR): {condition}']
    if contract:
        parts.append(f'contract: keel-spec {contract}')
    if not has_message:
        parts.append('this diagnostic has no message: the translation never emits it '
                     '(a check of the generated program, or a tool error printed by the driver); '
                     'give an empty `message`')
    elif current and current[0]:
        n = current[0].count('%s')
        parts.append(f'current message: {current[0]}\nkeep it unless the condition, as it is '
                     f'now, makes it wrong; whatever you give must have exactly {n} `%s`')
    if current and current[1]:
        parts.append(f'current English condition: {current[1]}')
    if section_text:
        parts.append('the section of the specification:\n\n' + section_text)
    schema = {
        'type': 'object',
        'properties': {'condition': {'type': 'string'}, 'message': {'type': 'string'},
                       'notes': {'type': 'string'}},
        'required': ['condition', 'message', 'notes'],
        'additionalProperties': False,
    }
    response = client.messages.create(
        model=model,
        max_tokens=4000,
        system=[{'type': 'text', 'text': system, 'cache_control': {'type': 'ephemeral'}}],
        messages=[{'role': 'user', 'content': '\n\n'.join(parts)}],
        output_config={'format': {'type': 'json_schema', 'schema': schema}},
    )
    if response.stop_reason == 'refusal':
        raise RuntimeError('the model declined this row')
    if response.stop_reason == 'max_tokens':
        raise RuntimeError('the answer was cut at max_tokens')
    text = next(b.text for b in response.content if b.type == 'text')
    return json.loads(text)


def check_answer(answer, current, has_message):
    """The answer as (message, condition), or an error that keeps the row as it was."""
    message = answer['message'].strip()
    condition = answer['condition'].strip().replace('\n', ' ')
    if not condition:
        raise ValueError('no condition')
    if not has_message:
        message = ''
    elif current and current[0] and message.count('%s') != current[0].count('%s'):
        raise ValueError('the message changed its number of %%s (%d, was %d)'
                         % (message.count('%s'), current[0].count('%s')))
    if re.search(r'%(?!s)', message.replace('%%', '')):
        raise ValueError('the message has a conversion other than %s')
    if '\n' in message:
        raise ValueError('the message has a line break')
    return message, condition


def render(head, sections):
    def cell(text):
        return text.replace('|', '\\|')
    out = [head.rstrip('\n')]
    for title, entries in sections:
        out += ['', '## ' + title, '', '| Identifier | Message | Condition |', '| --- | --- | --- |']
        out += ['| `%s` | %s | %s |' % (i, cell(m), cell(c)) for i, m, c in entries]
    return '\n'.join(out) + '\n'


def open_client():
    """(the anthropic module, a client), or (None, None) after saying why not."""
    if not os.environ.get('ANTHROPIC_API_KEY', '').strip():
        print('diag-sync.py: ANTHROPIC_API_KEY is empty or unset: export the key, '
              'or use --dry-run to see what would be asked', file=sys.stderr)
        return None, None
    try:
        import anthropic
    except ImportError:
        print('diag-sync.py: the anthropic package is not installed for %s.\n'
              'Where the system Python is externally managed (PEP 668), use a venv:\n'
              '  python3 -m venv .venv && .venv/bin/pip install anthropic\n'
              '  .venv/bin/python tools/cgen/diag-sync.py ...\n'
              '(.venv/ is ignored by git; a venv outside the tree, such as ~/.venvs/keel, '
              'works the same)' % sys.executable, file=sys.stderr)
        return None, None
    return anthropic, anthropic.Anthropic()


REVIEW_SCHEMA = {
    'type': 'object',
    'properties': {
        'verdict': {'type': 'string', 'enum': ['ok', 'improve']},
        'suggestion': {'type': 'string'},
        'reason': {'type': 'string'},
    },
    'required': ['verdict', 'suggestion', 'reason'],
    'additionalProperties': False,
}


def review_prompt():
    return f"""You review the diagnostic messages of cgen, the translator of keel, a language of \
islands of keel in a sea of C, translated to C11/C23. The documents of the project are in \
Brazilian Portuguese; the messages are in English, as the code is.

For each diagnostic you receive the identifier, its condition (in Portuguese, from the normative \
table, and in English), the current message, and the section of the specification it belongs to. \
The message is a printf format string: each `%s` is filled, in order, with text of the source or \
a short phrase from the tool, and the C code that emits it passes exactly that many arguments.

Judge the message against the rules below, and nothing else: not taste, not a synonym you prefer.
- `verdict`: `ok` when it follows the rules; `improve` when it breaks one of them.
- `suggestion`: with `improve`, the message rewritten to follow the rules, with exactly the same \
number of `%s`, meaning the same things, in the same order; starts in lower case, no final period, \
no line break. With `ok`, an empty string.
- `reason`: with `improve`, which rule it breaks and how, in one sentence. With `ok`, empty.

The rules for a message (the design document of the diagnostics, in Portuguese):

{design_rules()}"""


def review_one(client, model, system, row, current, section_text):
    ident, condition, severity, contract = row
    parts = [f'identifier: `{ident}`', f'severity: {severity}',
             f'condition (pt-BR): {condition}', f'condition (English): {current[1]}',
             f'message: {current[0]}', 'number of %%s: %d' % current[0].count('%s')]
    if section_text:
        parts.append('the section of the specification:\n\n' + section_text)
    response = client.messages.create(
        model=model,
        max_tokens=4000,
        system=[{'type': 'text', 'text': system, 'cache_control': {'type': 'ephemeral'}}],
        messages=[{'role': 'user', 'content': '\n\n'.join(parts)}],
        output_config={'format': {'type': 'json_schema', 'schema': REVIEW_SCHEMA}},
    )
    if response.stop_reason == 'refusal':
        raise RuntimeError('the model declined this row')
    if response.stop_reason == 'max_tokens':
        raise RuntimeError('the answer was cut at max_tokens')
    answer = json.loads(next(b.text for b in response.content if b.type == 'text'))
    if answer['verdict'] == 'improve':
        suggestion = answer['suggestion'].strip()
        if suggestion.count('%s') != current[0].count('%s'):
            answer['reason'] += (' [the suggestion has %d %%s and the message %d: '
                                 'it cannot replace it as written]'
                                 % (suggestion.count('%s'), current[0].count('%s')))
    return answer


def review(args, rows, current):
    """--review: asks for an opinion on every message, writes nothing."""
    chosen = [r for r in rows if r[0] in current and current[r[0]][0]
              and (not args.id or r[0] in args.id)]
    print('review %d message(s) with %s; nothing is written' % (len(chosen), args.model),
          file=sys.stderr)
    if args.dry_run:
        for row in chosen:
            print('review %s' % row[0])
        return 0
    anthropic, client = open_client()
    if client is None:
        return 2
    system = review_prompt()
    results, failures = {}, []

    def one(row):
        return row[0], review_one(client, args.model, system, row, current[row[0]],
                                  spec_section(row[3]) if row[3] else '')

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(one, row) for row in chosen]
        for done, future in enumerate(futures, 1):
            ident = chosen[done - 1][0]
            try:
                results[ident] = future.result()[1]
            except anthropic.AuthenticationError:
                print('diag-sync.py: the API refused the key (401): check ANTHROPIC_API_KEY',
                      file=sys.stderr)
                pool.shutdown(cancel_futures=True)
                return 2
            except anthropic.APIStatusError as e:
                failures.append((ident, 'API error %d: %s' % (e.status_code, e.message)))
            except anthropic.APIConnectionError as e:
                failures.append((ident, 'connection failed: %s' % e))
            except (RuntimeError, KeyError, json.JSONDecodeError) as e:
                failures.append((ident, str(e)))
            print('\r%d/%d' % (done, len(chosen)), end='', file=sys.stderr, flush=True)
    print(file=sys.stderr)

    improve = [(r[0], results[r[0]]) for r in chosen
               if r[0] in results and results[r[0]]['verdict'] == 'improve']
    print('# Review of the diagnostic messages\n')
    print('%d reviewed, %d ok, %d to improve, %d failed (model: %s).\n'
          % (len(chosen), len(results) - len(improve), len(improve), len(failures), args.model))
    for ident, answer in improve:
        print('## `%s`\n' % ident)
        print('- now: %s' % current[ident][0])
        print('- suggested: %s' % answer['suggestion'].strip())
        print('- why: %s\n' % answer['reason'].strip())
    for ident, why in failures:
        print('FAIL  %s: %s' % (ident, why), file=sys.stderr)
    return 1 if failures else 0


def main(argv=None):
    args = parse_args(sys.argv[1:] if argv is None else argv)
    spec_rows = table_rows(gen.SPEC, '### 6.2 Cat', '### 6.3')
    tool_rows = table_rows(gen.TOOL_SPEC, '### 7.1 Diag', '\n## ')
    own = gen.engine_tool_names()
    tool_ids = [r[0] for r in tool_rows]
    own_rows = [(i, '', 'error', '') for i in own if i not in tool_ids]
    wanted = [(SECTION_SPEC, spec_rows), (SECTION_TOOL, tool_rows), (SECTION_OWN, own_rows)]

    text = gen.MESSAGES.read_text(encoding='utf-8')
    head = text[:re.search(r'^## ', text, re.M).start()]
    current = {}
    for entries in gen.message_sections().values():
        for ident, message, condition in entries:
            current[ident] = (message, condition)
    lock = json.loads(LOCK.read_text()) if LOCK.exists() else {}

    if args.review:
        unknown = [i for i in args.id if i not in current]
        if unknown:
            print('unknown identifier: %s' % ', '.join(unknown), file=sys.stderr)
            return 2
        return review(args, [r for _, rows in wanted for r in rows], current)

    if args.lock:
        LOCK.write_text(json.dumps({r[0]: condition_hash(r[1]) for _, rows in wanted for r in rows},
                                   indent=1, sort_keys=True) + '\n')
        print('lock written: %d identifiers' % sum(len(rows) for _, rows in wanted))
        return 0

    every = {r[0] for _, rows in wanted for r in rows}
    dropped = sorted(set(current) - every)
    todo = [r for _, rows in wanted for r in rows
            if r[1] and (r[0] not in current or lock.get(r[0]) != condition_hash(r[1])
                         or r[0] in args.id)]
    unknown = [i for i in args.id if i not in every]
    if unknown:
        print('unknown identifier: %s' % ', '.join(unknown), file=sys.stderr)
        return 2
    for ident in dropped:
        print('drop  %s (no longer in the documents)' % ident)
    for row in todo:
        print('ask   %s (%s)' % (row[0], 'new' if row[0] not in current else 'condition changed'
                                 if lock.get(row[0]) != condition_hash(row[1]) else 'asked for'))
    if not todo and not dropped:
        print('diag-messages.md is in step with the documents')
        return 0
    if args.dry_run:
        return 0

    client = None
    if todo:
        anthropic, client = open_client()
        if client is None:
            return 2

    can_have = emittable(spec_rows)
    examples = [(i, sev, current[i][0], current[i][1]) for i, _, sev, _ in spec_rows
                if i in current and current[i][0] and current[i][1]][:16]
    system = system_prompt(examples)
    answers, failures, notes = {}, [], []
    for row in todo:
        ident = row[0]
        has_message = ident in can_have
        try:
            answer = ask(client, args.model, system, row, current.get(ident), has_message,
                         spec_section(row[3]) if row[3] else '')
            answers[ident] = check_answer(answer, current.get(ident), has_message)
            if answer['notes'].strip():
                notes.append((ident, answer['notes'].strip()))
            print('ok    %s' % ident)
        except anthropic.AuthenticationError:
            print('diag-sync.py: the API refused the key (401): check ANTHROPIC_API_KEY',
                  file=sys.stderr)
            return 2
        except anthropic.RateLimitError as e:
            failures.append((ident, 'rate limited (429): %s' % e.message))
        except anthropic.APIStatusError as e:
            failures.append((ident, 'API error %d: %s' % (e.status_code, e.message)))
        except anthropic.APIConnectionError as e:
            failures.append((ident, 'connection failed: %s' % e))
        except (RuntimeError, ValueError, KeyError, json.JSONDecodeError) as e:
            failures.append((ident, str(e)))

    sections = []
    for title, rows in wanted:
        entries = []
        for ident, condition, _, _ in rows:
            if ident in answers:
                message, english = answers[ident]
            elif ident in current:
                message, english = current[ident]
            else:
                continue   # a new row whose answer failed: it stays out, and the check says so
            entries.append((ident, message, english))
        sections.append((title, entries))
    gen.MESSAGES.write_text(render(head, sections), encoding='utf-8')
    for _, rows in wanted:
        for ident, condition, _, _ in rows:
            if ident in answers or (ident in current and ident not in [r[0] for r in todo]):
                lock[ident] = condition_hash(condition)
    for ident in dropped:
        lock.pop(ident, None)
    LOCK.write_text(json.dumps(lock, indent=1, sort_keys=True) + '\n')

    for ident, note in notes:
        print('note  %s: %s' % (ident, note))
    for ident, why in failures:
        print('FAIL  %s: %s' % (ident, why), file=sys.stderr)
    rc = subprocess.run([sys.executable, str(HERE / 'gen-diags.py')]).returncode
    if failures or rc:
        print('diag-sync.py: %d row(s) failed; the others were written; run again to retry'
              % len(failures), file=sys.stderr)
        return 1
    print('%d row(s) written; review them with git diff' % len(answers))
    return 0


if __name__ == '__main__':
    sys.exit(main())
