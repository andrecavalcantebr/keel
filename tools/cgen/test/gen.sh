#!/bin/sh
# Oracle of the M3 emission (cgen design §8.3 and §10), expected files written
# by hand from the documents, never captured from cgen:
#   - gen/hello/expected/<p>/  the four files of the module, byte for byte
#   - golden/<p>/keel.type.h, keel.h  the prelude, byte for byte
#   - the destination holds exactly those six files and nothing else
#   - determinism: two runs into different directories are identical
#   - a second run into the same directory writes nothing (no mtime changes)
# Exit 3 while `--stop-after=gen` is not implemented: run.sh reports `wip`.
cd "$(dirname "$0")/../../.." || exit 2
BIN=$PWD/tools/cgen/cgen
SRC=$PWD/tools/cgen/test/gen/hello
BASE=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
bad=0

gen() { # profile dest
    "$BIN" -std="$1" --base-dir "$BASE" --stop-after=gen --main hello \
        -I "$SRC" --dest-dir "$2" "$SRC/hello.k"
}

if ! gen c23 "$tmp/probe" >/dev/null 2>"$tmp/probe.err"; then
    if grep -q 'not yet implemented' "$tmp/probe.err"; then
        echo "--stop-after=gen not implemented yet"; exit 3
    fi
fi

for p in c23 c11; do
    out=$tmp/$p
    if ! gen $p "$out" 2>"$tmp/err"; then
        echo "FAIL: cgen --stop-after=gen ($p)"; sed 's/^/  /' "$tmp/err"; bad=1; continue
    fi
    for f in hello.type.h hello.h hello.c main_hello.c; do
        cmp -s "$SRC/expected/$p/$f" "$out/$f" || { echo "FAIL: $p/$f differs"
            diff "$SRC/expected/$p/$f" "$out/$f" | head -10 | sed 's/^/  /'; bad=1; }
    done
    for f in keel.type.h keel.h; do
        cmp -s "golden/$p/$f" "$out/$f" || { echo "FAIL: $p/$f (prelude) differs"
            diff "golden/$p/$f" "$out/$f" | head -10 | sed 's/^/  /'; bad=1; }
    done
    got=$(cd "$out" && find . -type f | sort | tr '\n' ' ')
    want="./hello.c ./hello.h ./hello.type.h ./keel.h ./keel.type.h ./main_hello.c "
    [ "$got" = "$want" ] || { echo "FAIL: $p file set [$got]"; bad=1; }
    # determinism: another directory, same bytes
    gen $p "$tmp/$p.again" 2>/dev/null
    diff -r "$out" "$tmp/$p.again" >/dev/null || { echo "FAIL: $p not deterministic"; bad=1; }
    # idempotence: a second run into the same directory touches nothing
    find "$out" -type f -exec stat -c '%n %Y' {} + | sort > "$tmp/m1"
    sleep 1
    gen $p "$out" 2>/dev/null
    find "$out" -type f -exec stat -c '%n %Y' {} + | sort > "$tmp/m2"
    cmp -s "$tmp/m1" "$tmp/m2" || { echo "FAIL: $p second run changed an mtime"; bad=1; }
done
[ $bad -eq 0 ] && echo "ok     hello (§8.3) in c23 and c11, prelude, determinism, mtime"
exit $bad
