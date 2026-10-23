#!/bin/sh
# `--cgen-help` (cgen-tool-spec §4.1; cgen design §2.2, §2.6): prints the
# synopsis and the option table to stdout and exits 0, wherever it appears on
# the line and whatever else is there, without running the C compiler.
# `--help` is the same as `--cgen-help`. `--help=<class>`, `-h` and `--version`
# are the C compiler's.
# Hand-written oracle: help.expected is the exact text.
set -u
cgen=$PWD/tools/cgen/cgen
want=$PWD/tools/cgen/test/cli/help.expected
d=$(mktemp -d)
trap 'rm -rf "$d"' EXIT
fail() { echo "FAIL: help — $1"; exit 1; }

cat > "$d/fakecc" <<FAKE
#!/bin/sh
echo "\$@" > "$d/cc.log"
echo "fake cc help"
exit 7
FAKE
chmod +x "$d/fakecc"

check() {   # check <label> <args...>
    label=$1; shift
    rm -f "$d/cc.log"
    timeout 10 "$cgen" "$@" > "$d/out" 2> "$d/err"; rc=$?
    [ "$rc" = 0 ] || fail "$label: exit code $rc, want 0"
    cmp -s "$want" "$d/out" || { diff "$want" "$d/out" | head -20; fail "$label: stdout differs from help.expected"; }
    [ ! -s "$d/err" ] || fail "$label: stderr not empty: $(head -3 "$d/err")"
    [ ! -e "$d/cc.log" ] || fail "$label: the C compiler was run"
}

check "alone" --cgen-help
check "with cc options" --cc="$d/fakecc" -O2 --cgen-help main.o
check "with a source" --cc="$d/fakecc" --cgen-help nowhere.k
check "before a bad option" --cgen-help --checks=maybe
check "after a bad value" --checks=maybe --cc="$d/fakecc" --cgen-help

check "--help alone" --help
check "--help with cc words" --cc="$d/fakecc" -std=c23 --help main.o
check "--help before a bad option" --help --checks=maybe

# --help=<class>, -h and --version are the C compiler's only
for w in --help=warnings -h --version; do
    rm -f "$d/cc.log"
    timeout 10 "$cgen" --cc="$d/fakecc" "$w" > "$d/out" 2>&1; rc=$?
    [ "$rc" = 7 ] || fail "$w: exit code $rc, want 7 (the fake cc's)"
    [ "$(cat "$d/out")" = "fake cc help" ] || fail "$w: cgen printed something: $(head -3 "$d/out")"
    [ "$(cat "$d/cc.log" 2>/dev/null)" = "$w" ] || fail "$w was not forwarded verbatim"
done

echo "ok: help ($(wc -l < "$want") lines; --help is --cgen-help; --help=, -h, --version forwarded)"
