#!/bin/sh
# Oracle of the default phase (cgen-tool-spec §4, §7): generate, then hand the unit to the C
# compiler. A fake compiler records its argument list and exits 7, so the order of the
# arguments and the exit code can be asserted; a real build of golden 033 runs the result.
cd "$(dirname "$0")/../../.." || exit 2
BIN=$PWD/tools/cgen/cgen
K=$PWD/base
CASE=$PWD/golden/cases/033-debug-more
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

cat > "$tmp/fakecc" <<EOT
#!/bin/sh
echo "\$@" > "$tmp/log"
exit 7
EOT
chmod +x "$tmp/fakecc"

"$BIN" --cc="$tmp/fakecc" --base-dir "$K" -I "$CASE" --dest-dir "$tmp/gen" --main app.dbg -O2 -Wall \
    "$CASE/app/dbg.k" >/dev/null 2>&1
rc=$?
[ $rc -eq 7 ] || bad "the compiler's exit code did not come through (got $rc)"
want="-I $CASE -O2 -Wall -I$tmp/gen $tmp/gen/app/app_dbg.c $tmp/gen/main_app_dbg.c"
[ "$(cat "$tmp/log")" = "$want" ] || bad "arguments: want [$want] got [$(cat "$tmp/log")]"

# -c: one unit, one object (§4.10), so --main is a second run, with its object beside the module's
cat > "$tmp/fakecc2" <<EOT
#!/bin/sh
echo "\$@" >> "$tmp/log2"
EOT
chmod +x "$tmp/fakecc2"
(cd "$tmp" && "$BIN" --cc="$tmp/fakecc2" --base-dir "$K" -I "$CASE" --dest-dir "$tmp/gen" --main app.dbg --checks=off -MMD -MF z.d -c -o out/x.o \
    "$CASE/app/dbg.k" >/dev/null 2>&1)
want1="-I $CASE -MMD -MF z.d -c -o out/x.o -I$tmp/gen -DKEEL_CHECKS=0 $tmp/gen/app/app_dbg.c"
want2="-I $CASE -c -o out/main_app_dbg.o -I$tmp/gen -DKEEL_CHECKS=0 $tmp/gen/main_app_dbg.c"
[ "$(sed -n 1p "$tmp/log2")" = "$want1" ] || bad "-c module run: want [$want1] got [$(sed -n 1p "$tmp/log2")]"
[ "$(sed -n 2p "$tmp/log2")" = "$want2" ] || bad "-c entry run: want [$want2] got [$(sed -n 2p "$tmp/log2")]"

# a language error stops before the compiler
rm -f "$tmp/log"
printf 'module other;\npub int f(void) { return 0; }\n' > "$tmp/bad.k"
"$BIN" --cc="$tmp/fakecc" --base-dir "$K" -I "$tmp" --dest-dir "$tmp/gen2" "$tmp/bad.k" >/dev/null 2>&1
[ $? -ne 0 ] && [ ! -f "$tmp/log" ] || bad "the compiler ran after a language error"

# the real compiler: the abort of golden 033 reports the column's line
"$BIN" -std=c2x --base-dir "$K" -I "$CASE" --dest-dir "$tmp/gen3" --main app.dbg -o "$tmp/dbg" "$CASE/app/dbg.k" >/dev/null 2>&1 || bad "real build failed"
err=$(sh -c '"$0" 1 2>&1 >/dev/null' "$tmp/dbg" 2>/dev/null | head -n1)
[ "$err" = "app/dbg.k:14: keel: extent-index-out-of-bounds" ] || bad "abort report: [$err]"

# the depfile: what the compiler reported, and the closure of .k files, in one rule (§4.10)
mkdir "$tmp/w" && (cd "$tmp/w" &&
    "$BIN" -std=c2x --base-dir "$K" -I "$CASE" --dest-dir gen -MMD -c -o d.o "$CASE/app/dbg.k" >/dev/null 2>&1) || bad "-MMD build failed"
dep=$tmp/w/d.d
[ "$(grep -c '^[^ ].*:' "$dep")" = 1 ] || bad "the depfile has more than one rule"
grep -q "gen/app/app_dbg.h" "$dep" || bad "the depfile lacks the headers the compiler reported"
grep -q "$CASE/app/dbg.k" "$dep" || bad "the depfile lacks the module's own .k"
grep -q "$K/keel/routine.k" "$dep" || bad "the depfile lacks a .k the module reaches"
(cd "$tmp/w" && "$BIN" --base-dir "$K" -I "$CASE" --dest-dir gen --stop-after=gen -MMD -MF g.d -MT tgt "$CASE/app/dbg.k" >/dev/null 2>&1)
head -1 "$tmp/w/g.d" | grep -q '^tgt:' || bad "--stop-after=gen depfile target"

# --instance is a unit of its own (§4.3): compiled to an object, with its depfile
(cd "$tmp/w" && "$BIN" -std=c2x --base-dir "$K" --dest-dir gi -MMD -c --instance "keel.buffer.buffer i32" -o bi.o >/dev/null 2>&1) || bad "--instance -c failed"
[ -f "$tmp/w/bi.o" ] || bad "--instance -c wrote no object"
head -1 "$tmp/w/bi.d" | grep -q '^bi.o:' || bad "--instance depfile target"
grep -q "$K/keel/buffer.k" "$tmp/w/bi.d" || bad "--instance depfile lacks the generic's .k"
"$BIN" --cc="$tmp/fakecc" --base-dir "$K" --dest-dir "$tmp/gj" --instance "keel.buffer.buffer i32" >/dev/null 2>&1
[ $? -eq 7 ] || bad "--instance did not reach the compiler"

[ $fail -eq 0 ] && echo "cc phase: ok"
exit $fail
