#!/bin/sh
# `-o` with a stopping phase, with gcc's meaning (cgen-tool-spec §4.1, §4.2):
# the file of the last phase; `-o -` is stdout; refused with gen, which writes
# several files; and, as §6 asks, not created when the phase fails.
set -u
d=$(mktemp -d)
trap 'rm -rf "$d"' EXIT
cgen=$PWD/tools/cgen/cgen
base=$PWD/base
printf 'module app;\npub i32 one(void) { return 1; }\n' > "$d/app.k"
printf 'module bad;\n#define keel_bad 1\n' > "$d/bad.k"
cd "$d" || exit 1
fail() { echo "FAIL: output — $1"; exit 1; }

"$cgen" --base-dir "$base" --stop-after=parse app.k > want.parse || fail "parse to stdout"
"$cgen" --base-dir "$base" -o out.parse --stop-after=parse app.k > stdout.txt || fail "parse with -o"
cmp -s want.parse out.parse || fail "-o file differs from stdout"
[ ! -s stdout.txt ] || fail "-o still wrote to stdout"
"$cgen" --base-dir "$base" -o - --stop-after=parse app.k > dash.parse || fail "-o -"
cmp -s want.parse dash.parse || fail "-o - is not stdout"

"$cgen" --stop-after=lex app.k > want.lex || fail "lex to stdout"
"$cgen" -o out.lex --stop-after=lex app.k || fail "lex with -o"
cmp -s want.lex out.lex || fail "-o lex file differs from stdout"

"$cgen" --base-dir "$base" -o bad.parse --stop-after=parse bad.k 2>/dev/null && fail "bad source succeeded"
[ ! -e bad.parse ] || fail "a failed parse created its -o file"
"$cgen" -o bad.lex --stop-after=lex bad.k > /dev/null 2>&1 && fail "bad lex succeeded"
[ ! -e bad.lex ] || fail "a failed lex created its -o file"

err=$("$cgen" --base-dir "$base" -o x --stop-after=gen app.k 2>&1); rc=$?
[ $rc -eq 2 ] && printf '%s' "$err" | grep -q '\[output-with-gen\]' || fail "gen with -o not refused ($rc: $err)"
echo ok
