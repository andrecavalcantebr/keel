#!/bin/sh
# The prelude's primitive types keep their spelling in C (backend §4.2, rule 1:
# `typedef int8_t i8;`), so the symbol column of the parse dump has no module
# prefix for `module keel`, while any other module's declarations do.
set -u
BIN=tools/cgen/cgen
out=$(cd base && "../$BIN" --base-dir . --stop-after=parse keel.k 2>&1) || { echo "FAIL: cgen on base/keel.k"; printf '%s\n' "$out"; exit 1; }
bad=0
want='decl	pub type	i8	i8	keel.k:19:1'
printf '%s\n' "$out" | grep -qxF "$want" || { echo "FAIL: missing line: $want"; bad=1; }
printf '%s\n' "$out" | grep -q 'keel_i8\|keel_f64' && { echo "FAIL: the prelude's types got the module prefix"; bad=1; }
dir=$(mktemp -d); trap 'rm -rf "$dir"' EXIT
printf 'module hello;\npub typedef int T;\n' > "$dir/hello.k"
got=$(cd "$dir" && "$OLDPWD/$BIN" --base-dir "$OLDPWD/base" --stop-after=parse hello.k 2>&1 | grep '^decl')
[ "$got" = 'decl	pub type	T	hello_T	hello.k:2:1' ] || { echo "FAIL: other module: got [$got]"; bad=1; }
[ $bad -eq 0 ] && echo "ok     prelude symbols unprefixed, other modules prefixed"
exit $bad
