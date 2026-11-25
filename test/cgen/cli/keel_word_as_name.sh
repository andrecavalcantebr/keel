#!/bin/sh
# A keel word as a file-scope name, followed by a declaration whose visibility word has the same
# spelling (achados 2026-11-25, A1): `k_em_decl_edits` pushed the visibility edit for the keyword
# position, and the renaming loop pushed a second `EDIT_RENAME` at that same offset (the parser
# leaves a named island there too, since the spelling matches a declared symbol). The subtraction in
# `k_em_copy_edited` underflowed and read past the source, caught only by accident at `LINE_CAP`.
# Expected by hand: the warning stays (keel-name-shadowed; the word is still legal as a name), but the
# module compiles, with the symbol renamed like any other — `owner_pub`, `owner_value` — and nothing
# else. The control (no homonymous declaration using the word as a construction) keeps working.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

check() {
    name=$1; src=$2; grep_for=$3
    printf '%s' "$src" > "$tmp/owner.k"
    out=$("$BIN" -I "$tmp" --base-dir "$K" --stop-after=gen --dest-dir "$tmp/gen_$name" "$tmp/owner.k" 2>&1); rc=$?
    [ $rc -eq 0 ] || { bad "$name: rc=$rc [$out]"; return; }
    case $out in
        *keel-name-shadowed*) : ;;
        *) bad "$name: lost the keel-name-shadowed warning [$out]" ;;
    esac
    grep -q "$grep_for" "$tmp/gen_$name/owner.h" || bad "$name: owner.h does not hold '$grep_for'"
    cc -std=c2x -fsyntax-only -Wall -Werror -I "$tmp/gen_$name" "$tmp/gen_$name/owner.c" 2>"$tmp/cc_$name" \
        || bad "$name: the C does not compile [$(head -3 "$tmp/cc_$name")]"
}

check typedef   'module owner;
typedef i32 pub;
pub inline i32 value(void) { return 17; }
' 'static inline i32 owner_value(void) { return 17; }'

check variable  'module owner;
i32 pub = 1;
pub inline i32 value(void) { return 17; }
' 'static inline i32 owner_value(void) { return 17; }'

# control: the same homonymous declaration, but no function reuses the word as a construction
printf '%s' 'module owner;
typedef i32 pub;
inline i32 value(void) { return 17; }
' > "$tmp/owner.k"
out=$("$BIN" -I "$tmp" --base-dir "$K" --stop-after=gen --dest-dir "$tmp/gen_control" "$tmp/owner.k" 2>&1); rc=$?
[ $rc -eq 0 ] || bad "control: rc=$rc [$out]"

[ $fail -eq 0 ] && echo ok
exit $fail
