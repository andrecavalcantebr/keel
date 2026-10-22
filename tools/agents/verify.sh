#!/usr/bin/env sh
# The acceptance that every agent task runs before it reports (tools/agents/README.md).
# Run from anywhere; it works from the repository root.
#
#   sh tools/agents/verify.sh                 build (warning count), make check, golden/run.sh
#   sh tools/agents/verify.sh snapshot DIR    record the behaviour of cgen into DIR
#   sh tools/agents/verify.sh compare DIR     the full check, plus: behaviour equal to DIR
#
# The snapshot is the oracle of a refactor (no behaviour change): for every .k of
# golden/cases/* and of tools/cgen/test/diag/*, the stdout, the stderr and the exit
# code of `cgen --stop-after=parse`, run from the case directory as parse_dump.sh
# and diag.py do. A refactor takes the snapshot BEFORE touching code.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT" || exit 2
MODE=${1:-check}
DIR=${2:-}

warnings_file=tools/agents/warnings.baseline

build() {
    make -s -C tools/cgen clean >/dev/null 2>&1
    make -C tools/cgen -j8 >/tmp/agents_build.$$ 2>&1 || {
        echo "FAIL build"; grep -E 'error' /tmp/agents_build.$$ | head -20; rm -f /tmp/agents_build.$$; exit 1; }
    n=$(grep -c 'warning:' /tmp/agents_build.$$)
    base=$(cat "$warnings_file" 2>/dev/null || echo 0)
    if [ "$n" -gt "$base" ]; then
        echo "FAIL build: $n warnings, the baseline is $base"; grep 'warning:' /tmp/agents_build.$$ | head -20
        rm -f /tmp/agents_build.$$; exit 1
    fi
    echo "ok   build ($n warnings, baseline $base)"
    rm -f /tmp/agents_build.$$
}

snapshot() {
    out=$1; mkdir -p "$out"
    for root in golden/cases/*/ tools/cgen/test/diag/*/; do
        [ -d "$root" ] || continue
        (cd "$root" && find . -name '*.k' -not -path './expected/*' | sort) | while read -r k; do
            k=${k#./}
            key=$(printf '%s/%s' "${root%/}" "$k" | tr '/' '%')
            (cd "$root" && timeout 20 "$ROOT/tools/cgen/cgen" --base-dir "$ROOT/base" \
                --stop-after=parse "$k" >"$out/$key.out" 2>"$out/$key.err"; echo $? >"$out/$key.rc")
        done
    done
    echo "ok   snapshot $(ls "$out" | grep -c '\.rc$') sources -> $out"
}

build
make -C tools/cgen check >/tmp/agents_check.$$ 2>&1
rc=$?
if [ $rc -ne 0 ] || grep -qE '^(FAIL|fail)' /tmp/agents_check.$$; then
    echo "FAIL make check"; grep -vE '^ok' /tmp/agents_check.$$ | grep -iE 'fail|error' | head -20
    rm -f /tmp/agents_check.$$; exit 1
fi
echo "ok   make check ($(grep -c '^ok' /tmp/agents_check.$$) lines ok)"
rm -f /tmp/agents_check.$$

g=$(cd golden && ./run.sh 2>&1 | tail -1)
case "$g" in *" 0 fail"*) echo "ok   golden: $g" ;; *) echo "FAIL golden: $g"; exit 1 ;; esac

case "$MODE" in
check) ;;
snapshot)
    [ -n "$DIR" ] || { echo "usage: verify.sh snapshot DIR"; exit 2; }
    snapshot "$DIR" ;;
compare)
    [ -n "$DIR" ] && [ -d "$DIR" ] || { echo "usage: verify.sh compare DIR (taken before)"; exit 2; }
    now=$(mktemp -d)
    snapshot "$now" >/dev/null
    if diff -r "$DIR" "$now" >/tmp/agents_diff.$$; then
        echo "ok   behaviour identical to $DIR"
    else
        echo "FAIL behaviour changed:"; head -40 /tmp/agents_diff.$$; rm -rf "$now" /tmp/agents_diff.$$; exit 1
    fi
    rm -rf "$now" /tmp/agents_diff.$$ ;;
*) echo "unknown mode $MODE"; exit 2 ;;
esac
echo "ALL OK"
