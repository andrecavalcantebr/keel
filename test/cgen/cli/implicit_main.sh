#!/bin/sh
# Oracle of the implicit entry (cgen-tool-spec §4.7): an invocation that links and gives no --main
# takes the compiled module's own public main; -c, -S, -E, -shared and --stop-after do not link, and
# a module without a main has no entry either. A fake compiler records its arguments.
cd "$(dirname "$0")/../../.." || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
APP=$PWD/golden/cases/033-debug-more
LIB=$PWD/golden/cases/019-walk
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

cat > "$tmp/fakecc" <<EOT
#!/bin/sh
echo "\$@" >> "$tmp/log"
EOT
chmod +x "$tmp/fakecc"

# links, module with main: the entry unit goes to the compiler without --main
"$BIN" --cc="$tmp/fakecc" --base-dir "$K" -I "$APP" --dest-dir "$tmp/g1" -O2 "$APP/app/dbg.k" >/dev/null 2>&1
want="-I $APP -O2 -I$tmp/g1 $tmp/g1/app/app_dbg.c $tmp/g1/main_app_dbg.c"
[ "$(cat "$tmp/log")" = "$want" ] || bad "implicit --main: want [$want] got [$(cat "$tmp/log")]"
[ -f "$tmp/g1/main_app_dbg.c" ] || bad "implicit --main: main_app_dbg.c not written"

# -c: no entry unit, one run of the compiler
: > "$tmp/log"
"$BIN" --cc="$tmp/fakecc" --base-dir "$K" -I "$APP" --dest-dir "$tmp/g2" -c -o "$tmp/x.o" "$APP/app/dbg.k" >/dev/null 2>&1
want="-I $APP -c -o $tmp/x.o -I$tmp/g2 $tmp/g2/app/app_dbg.c"
[ "$(cat "$tmp/log")" = "$want" ] || bad "-c: want [$want] got [$(cat "$tmp/log")]"
[ ! -e "$tmp/g2/main_app_dbg.c" ] || bad "-c: an entry unit was written"

# --stop-after=gen: nothing links
"$BIN" --base-dir "$K" -I "$APP" --dest-dir "$tmp/g3" --stop-after=gen "$APP/app/dbg.k" >/dev/null 2>&1
[ ! -e "$tmp/g3/main_app_dbg.c" ] || bad "--stop-after=gen: an entry unit was written"

# a module without main: links, but there is no entry to take
: > "$tmp/log"
"$BIN" --cc="$tmp/fakecc" --base-dir "$K" -I "$LIB" --dest-dir "$tmp/g4" "$LIB/lst.k" >/dev/null 2>&1
[ ! -e "$tmp/g4/main_lst.c" ] || bad "no main: an entry unit was written"
grep -q "main_" "$tmp/log" && bad "no main: the compiler was given an entry unit"

[ $fail -eq 0 ] && echo "ok     implicit main (cgen-tool-spec §4.7)"
exit $fail
