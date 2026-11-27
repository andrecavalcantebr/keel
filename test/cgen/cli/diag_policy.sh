#!/bin/sh
# `-Wno-<name>` and `-Werror` over the diagnostics of the language (keel_libs finding A2, cgen 0.4.2;
# manual §7.7, tool spec §7). Expected by hand, for a program that draws the warning `return-may-escape`
# and the info `injected-names`:
#   plain             exit 0, both printed
#   -Werror           exit 1, the warning is an error, the info stays an info
#   -Wno-<warning>    exit 0, the warning is gone (and -Werror then has nothing to stop)
#   -Wno-<info>       the info is gone
#   a name of the table is not forwarded: the fake cc receives no -Wno-return-may-escape
#   a name outside the table (-Wall, -Wno-unused) is forwarded; -Werror goes to both sides
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }
cat > "$tmp/u.k" <<'EOF'
module u;
import keel.arena as arena types;

pub i32 leak(void) {
    array u8 storage[16];
    arena a;
    arena.from_array(a, storage);
    u8 *p = arena.alloc(a, u8, 4);
    return p ? 0 : 1;
}
EOF
cat > "$tmp/fakecc" <<EOF
#!/bin/sh
echo "\$@" > "$tmp/cc.log"
exit 0
EOF
chmod +x "$tmp/fakecc"
run() { "$BIN" --base-dir "$K" --cc="$tmp/fakecc" -I "$tmp" --dest-dir "$tmp/g" -std=c2x "$@" -c "$tmp/u.k" -o "$tmp/u.o" 2>"$tmp/err"; echo $?; }

rc=$(run);                                   [ "$rc" = 0 ] || bad "plain: exit $rc"
grep -q 'return-may-escape' "$tmp/err" && grep -q 'injected-names' "$tmp/err" || bad "plain: a diagnostic is missing"
rc=$(run -Werror);                           [ "$rc" = 1 ] || bad "-Werror: exit $rc"
grep -q 'error: .*\[return-may-escape\]' "$tmp/err" || bad "-Werror: the warning is not an error"
grep -q 'info: .*\[injected-names\]' "$tmp/err" || bad "-Werror: the info changed"
rc=$(run -Wno-return-may-escape);            [ "$rc" = 0 ] || bad "-Wno: exit $rc"
grep -q 'return-may-escape' "$tmp/err" && bad "-Wno: the warning stayed"
rc=$(run -Werror -Wno-return-may-escape);    [ "$rc" = 0 ] || bad "-Werror -Wno: exit $rc"
rc=$(run -Wno-injected-names);               [ "$rc" = 0 ] || bad "-Wno-info: exit $rc"
grep -q 'injected-names' "$tmp/err" && bad "-Wno-info: the info stayed"
rc=$(run -Wno-return-may-escape -Wall -Wno-unused -Werror)
[ "$rc" = 0 ] || bad "forwarding: exit $rc"
grep -q -- '-Wno-return-may-escape' "$tmp/cc.log" && bad "a name of the table reached the cc"
for o in -Wall -Wno-unused -Werror; do grep -q -- "$o" "$tmp/cc.log" || bad "$o was not forwarded"; done
[ $fail -eq 0 ] && echo ok
exit $fail
