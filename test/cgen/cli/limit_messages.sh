#!/bin/sh
# An implementation limit says WHAT ran out, its value and the way out (cgen-tool-spec §7.1, spec §6.4), and not one
# sentence for everything (E02, E03): each point below is refused with its own text, and with the id
# [implementation-limit]. A construction at the limit passes, one past it is refused.
cd "$(dirname "$0")/../.." && cd .. || exit 2
BIN=${CGEN_BIN:-$PWD/tools/cgen/cgen}
K=$PWD/base
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail=0
bad() { echo "FAIL: $1"; fail=1; }

# run NAME < source   -> $tmp/NAME/{log,rc}; the module is called t
run() { d=$tmp/$1; mkdir -p "$d"; cat > "$d/t.k"; "$BIN" --base-dir "$K" -I "$d" --stop-after=gen --dest-dir "$d/out" "$d/t.k" >"$d/log" 2>&1; echo $? > "$d/rc"; }
# refused NAME 'text of the message'
refused() {
    [ "$(cat $tmp/$1/rc)" != 0 ] && grep -qF "$2" "$tmp/$1/log" && grep -qF '[implementation-limit]' "$tmp/$1/log" \
        || bad "$1: wanted '$2' (rc=$(cat $tmp/$1/rc)): $(grep -v 'info:' $tmp/$1/log | head -2)"
}
passes() { [ "$(cat $tmp/$1/rc)" = 0 ] || bad "$1 must pass: $(grep -v 'info:' $tmp/$1/log | head -2)"; }
seq_() { i=0; while [ $i -lt "$2" ]; do [ $i -gt 0 ] && printf '%s' "${3:-}"; printf '%s' "$(echo "$1" | sed "s/@/$i/g")"; i=$((i + 1)); done; }

# the binders of a `module` line: 8 per list
printf 'module t dim A, B, C, D, E, F, G, H, I;\n' | run binders
refused binders 'implementation limit: more than 8 binders in one `dim`, `tags` or `type` list of the `module` line: the cgen holds 8 per list; split the module'

# variables visible at once: 128
{ echo 'module t;'; seq_ 'i32 g@ = @;' 129 '
'; echo; } | run globals
refused globals 'implementation limit: more than 128 variables visible at once (locals and the file'"'"'s own): the cgen holds 128; split the function or the module'

# delimiters nested: 256
{ printf 'module t;\npub i32 f(void) { return '; seq_ '(' 300; printf '1'; seq_ ')' 300; printf '; }\n'; } | run delimiters
refused delimiters 'implementation limit: delimiters nested deeper than 256: the cgen holds 256 open delimiters at once; flatten the nesting'

# `array` declarators: 8
{ printf 'module t;\narray i32 '; seq_ 'v@[2]' 9 ', '; printf ';\n'; } | run array9
refused array9 'implementation limit: more than 8 declarators in one `array` declaration: the cgen holds 8; split the declaration'
{ printf 'module t;\narray i32 '; seq_ 'v@[2]' 8 ', '; printf ';\n'; } | run array8
passes array8

# `protocol` items: 16 per kind
{ printf 'module t;\nprotocol P type C {\n'; seq_ '  i32 v@(C *c);' 17 '
'; echo; printf '}\n'; } | run protocol17
refused protocol17 'implementation limit: a `protocol` with more than 16 verbs, associated types or composed protocols (16 of each kind): the cgen holds 16; split the protocol'

# `defer [now ...]`: 8 entries
{ printf 'module t;\npub void f(void) {\n    int '; seq_ 'a@' 9 ', '; printf ';\n    defer [now '; seq_ 'int a@' 9 ', '; printf '] { }\n}\n'; } | run defer9
refused defer9 'a `defer [now ...]` with more than 8 entries: the emitter holds 8 (MAX_CAP in engine/emit/defer.c); capture fewer names, or one struct'

# `parallel` captures: 16
mkpar() { printf 'module t;\nimport keel.buffer as buffer types;\nimport keel.slice as slice;\nimport keel.parallel as parallel;\npub void f(buffer i32 *xs) {\n    i32 '; seq_ 'c@' "$1" ', '; printf ';\n    parallel v ALL (size_t w : 0..4; slice.slice i32 part : xs; ('; seq_ 'c@' "$1" ', '; printf ')) { }\n}\n'; }
mkpar 17 | run par17
refused par17 'a `parallel` capture list with more than 16 names: the emitter holds 16 (MAX_CAPS in engine/emit/parallel.c); capture fewer names, or one struct'

# the edits of one line (a `match` with many labels is one line): the text says it is the labels, not the line
{ printf 'module t;\nimport keel.tagged as tagged types;\npub tags T ['; seq_ 'Label_with_a_long_name_@' 15 ', '; printf '];\npub i32 f(tagged T void *s) {\n    match (s) {\n'; seq_ '        Label_with_a_long_name_@: return @;' 15 '
'; echo; printf '    }\n    return 0;\n}\n'; } | run match15
grep -qF 'fewer or shorter names (a `match` with many labels is one line)' "$tmp/match15/log" \
    || bad "match15: the scratch limit must name the labels: $(grep -v 'info:' $tmp/match15/log | head -2)"

[ $fail -eq 0 ] && echo ok
exit $fail
