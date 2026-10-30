#!/bin/sh
# Oracle of the base, written by cgen (cgen design §2.8, §8.7; backend §4.3.2):
# golden/<p>/keel/* is what the base's sources produce, byte for byte, in each
# profile. The non-generic modules are compiled as any module (`cgen
# base/keel/<m>.k`); the generic ones through the instances the golden holds
# (`--instance "<M.mod> <args>"`; `"<M> <args>"` for a generic without a
# modifier, `keel.array`; `"<M.fn> <concrete type>"` for a function over a
# protocol, backend §5.19), each call writing its whole closure. A golden
# file nothing here produces is `wip`, and so
# is a use the emitter refuses with implementation-limit; any other failure,
# and any file that differs, is a FAIL.
cd "$(dirname "$0")/../.." || exit 2
BIN=$PWD/tools/cgen/cgen
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
MODULES="arena corot parallel range"
USES="keel.array i32|keel.buffer.buffer i32|keel.buffer.buffer size_t|keel.slice.slice f32|keel.slice.slice i32|keel.slice.slice size_t|keel.outcome.outcome f32|keel.outcome.outcome i32|keel.outcome.outcome size_t|keel.outcome.outcome u32|keel.outcome.outcome void|keel.slice.of keel.array i32|keel.slice.of keel.buffer.buffer i32|keel.slice.of keel.slice.slice i32"
bad=0 ok=0 wip=0
for p in c23 c11; do
    out=$tmp/$p
    for m in $MODULES; do
        "$BIN" -std=$p -I base --base-dir base --stop-after=gen --dest-dir "$out" "base/keel/$m.k" >/dev/null 2>"$tmp/err" ||
            { printf 'FAIL   %s keel.%s\n' "$p" "$m"; sed -n '1,5s/^/       /p' "$tmp/err"; bad=$((bad+1)); }
    done
    IFS='|'
    for u in $USES; do
        unset IFS
        if ! "$BIN" -std=$p --base-dir base --stop-after=gen --dest-dir "$out" --instance "$u" >/dev/null 2>"$tmp/err"; then
            if grep -q 'implementation-limit' "$tmp/err"; then printf 'wip    %s %s\n' "$p" "$u"; wip=$((wip+1))
            else printf 'FAIL   %s %s\n' "$p" "$u"; sed -n '1,5s/^/       /p' "$tmp/err"; bad=$((bad+1)); fi
        fi
        IFS='|'
    done
    unset IFS
    for f in $(cd "golden/$p" && find keel.h keel.type.h keel -type f | sort); do
        if [ ! -f "$out/$f" ]; then wip=$((wip+1)); continue; fi
        if cmp -s "golden/$p/$f" "$out/$f"; then ok=$((ok+1)); else
            printf 'FAIL   %s/%s differs\n' "$p" "$f"
            diff "golden/$p/$f" "$out/$f" 2>&1 | head -6 | sed 's/^/       /'
            bad=$((bad+1))
        fi
    done
done
echo "$ok ok, $bad fail, $wip wip"
[ $bad -eq 0 ]
