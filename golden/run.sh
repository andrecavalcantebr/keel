#!/usr/bin/env sh
# golden/run.sh — compiles what the cgen must produce, in both profiles, and
# runs each case's proof.
#
#   cases/<case>/<module-path>.k     the keel source; the path comes from `module`
#   cases/<case>/expected/<profile>/ what the cgen must produce
#   cases/<case>/proof.c             the harness — NOT transpiler output
#   cases/<case>/VERIFY              asserts what is not "it compiles"; the script obeys it
#   cases/<case>/XFAIL               expected NOT to compile
#
# -Werror=vla enforces a normative claim: language §4.4 says keel emits neither
# VLA nor alloca anywhere.
CC=${CC:-gcc}
FLAGS="-pedantic-errors -Wall -Wextra -Wno-unused-parameter -Wvla -Werror=vla"
ok=0; fail=0; xfail=0; xpass=0; wip=0

# ---------------------------------------------------------------- structure
# The layer cut of backend §4.3.2 is checkable without compiling anything, and
# it is checked here: output that violated it would compile all the same, and
# the suite would stop being the oracle of the very rule that kills include
# cycles.
structure=0
for h in $(find c23 c11 cases/*/expected -name '*.h' ! -name '*.type.h' | sort); do
  [ -f "${h%.h}.type.h" ] || { printf 'STRUCT %s — missing %s\n' "$h" "${h%.h}.type.h"; structure=$((structure+1)); }
done
for d in $(find c23 c11 cases/*/expected -name '*.proto.h' | sort); do
  printf 'STRUCT %s — the cut is in two, .type.h and .h (backend §4.3.2)\n' "$d"; structure=$((structure+1))
done

# The .k's path is the `module` name: that is `module-path-mismatch` (spec
# §4.1), and the suite violated it in 19 of 22 sources — all called caso.k,
# while the `#line`s in the generated files next to them already cited the
# right path.
for case_dir in cases/*/; do
  [ -f "$case_dir/WIP" ] && continue
  for k in $(find "$case_dir" -name '*.k' | sort); do
    mod=$(grep -m1 '^module' "$k" | sed 's/^module *//; s/[; ].*//')
    want="$case_dir$(printf '%s' "$mod" | tr '.' '/').k"
    [ "$k" = "$want" ] || { printf 'STRUCT %s — declares `module %s`, should be %s\n' "$k" "$mod" "$want"; structure=$((structure+1)); }
  done
done

# The generated name and the source name follow DIFFERENT rules, and both are
# enforced: the .k lives at the module's path (above), and the header carries
# the mangled symbol under the parent components' directory (backend §4.1).
# `module app.cfg;` lives in app/cfg.k and generates app/app_cfg.h. And the
# case's module is what the invocation compiles, so it also generates the .c
# (backend §4.1) — always, even if only with the include — except for a
# generic module, which does not compile on its own (`generic-source-without-instance`).
for case_dir in cases/*/; do
  [ -f "$case_dir/WIP" ] && continue
  for profile in c23 c11; do
    gen="$case_dir/expected/$profile"
    [ -d "$gen" ] || continue
    for k in $(find "$case_dir" -name '*.k' | sort); do
      mod=$(grep -m1 '^module' "$k" | sed 's/^module *//; s/[; ].*//')
      dir=$(printf '%s' "$mod" | sed 's/\.[^.]*$//; t; s/.*//' | tr '.' '/')
      sym=$(printf '%s' "$mod" | tr '.' '_')
      [ -n "$dir" ] && target="$gen/$dir/$sym.h" || target="$gen/$sym.h"
      [ -f "$target" ] || { printf 'STRUCT %s — `module %s` should generate %s\n' "$k" "$mod" "$target"; structure=$((structure+1)); }
      # a generic is not a compiled unit: it has the headers, not the .c
      if grep -m1 '^module' "$k" | grep -Eq '[[:space:]](type|dim|tags)[[:space:]]'; then
        [ ! -f "${target%.h}.c" ] || { printf 'STRUCT %s — `module %s` is generic and generates no .c\n' "$k" "$mod"; structure=$((structure+1)); }
      else
        [ -f "${target%.h}.c" ] || { printf 'STRUCT %s — `module %s` should generate %s\n' "$k" "$mod" "${target%.h}.c"; structure=$((structure+1)); }
      fi
    done
  done
done

# I1: a .type.h includes only .type.h — that is what makes the layout graph a DAG
for t in $(find c23 c11 cases/*/expected -name '*.type.h' | sort); do
  bad=$(grep '^#include "' "$t" | grep -v '\.type\.h"')
  [ -z "$bad" ] || { printf 'STRUCT %s — .type.h including outside its layer:\n%s\n' "$t" "$bad"; structure=$((structure+1)); }
done

# I2: the .h sections (rule 2) — every #include of a .type.h comes before the
# first #include of a .h. The prototypes sit between the two blocks, and it is
# that order which makes every reachable prototype arrive before the first body.
for h in $(find c23 c11 cases/*/expected -name '*.h' ! -name '*.type.h' | sort); do
  bad=$(grep '^#include "' "$h" | awk '/\.type\.h"/ { if (seen) print; next } { seen = 1 }')
  [ -z "$bad" ] || { printf 'STRUCT %s — .type.h included after a .h:\n%s\n' "$h" "$bad"; structure=$((structure+1)); }
done
[ $structure -eq 0 ] && printf 'ok     layer structure              (backend §4.3.2)\n'
fail=$((fail+structure))

for case_dir in cases/*/; do
  name=$(basename "$case_dir")
  # a case under construction: its source exists, its expected does not yet
  if [ -f "$case_dir/WIP" ]; then
    printf 'wip    %-24s      — %s\n' "$name" "$(head -1 "$case_dir/WIP")"; wip=$((wip+1)); continue
  fi
  for profile in c23 c11; do
    [ "$profile" = c23 ] && std=c2x || std=c11
    gen="$case_dir/expected/$profile"
    [ -d "$gen" ] || { printf 'MISSING %-23s %s  — no expected/%s\n' "$name" "$profile" "$profile"; fail=$((fail+1)); continue; }

    if [ -x "$case_dir/VERIFY" ]; then
      if "$case_dir/VERIFY" "$CC" "-std=$std" "$profile"; then
        printf 'ok     %-24s %s  (verify)\n' "$name" "$profile"; ok=$((ok+1))
      else
        printf 'FAIL   %-24s %s  (verify)\n' "$name" "$profile"; fail=$((fail+1))
      fi
      continue
    fi

    srcs=$(find "$gen" -name '*.c' | sort | tr '\n' ' ')
    [ -f "$case_dir/proof.c" ] && srcs="$srcs $case_dir/proof.c"
    inc="-I$profile -I$gen -I$case_dir"

    # every module with `pub` must have a `.h` — this slipped by 16 times
    if find "$case_dir" -name '*.k' -exec grep -lq '^pub ' {} + 2>/dev/null && ! find "$gen" -name '*.h' ! -name '*.type.h' | grep -q .; then
      printf 'FAIL   %-24s %s  — module with `pub` and no generated .h\n' "$name" "$profile"; fail=$((fail+1)); continue
    fi

    mode=compile; exe=/dev/null
    grep -lq '^int main' $srcs 2>/dev/null && { mode=run; exe=$(mktemp); }
    omps=""; grep -lq '#pragma omp' $srcs 2>/dev/null && omps=" -fopenmp"

    good=1
    # both check modes (backend §5.17): the generated C is the same, only the
    # KEEL_CHECKS switch given to the C compiler changes
    for chk in 1 0; do
    for omp in "" $omps; do
      [ -z "$omp" ] && extra=-Wno-unknown-pragmas || extra=""
      $CC "-std=$std" $FLAGS -DKEEL_CHECKS=$chk $extra $omp $inc $([ $mode = compile ] && echo -c) $srcs -o "$exe" 2>/dev/null \
        && { [ $mode = compile ] || "$exe" >/dev/null; } || good=0
    done
    done
    [ -n "$omps" ] && mode="$mode, ±omp"

    if [ $good -eq 1 ]; then
      if [ -f "$case_dir/XFAIL" ]; then
        printf 'XPASS  %-24s %s  — compiled, but there is an XFAIL\n' "$name" "$profile"; xpass=$((xpass+1))
      else
        printf 'ok     %-24s %s  (%s)\n' "$name" "$profile" "$mode"; ok=$((ok+1))
      fi
    elif [ -f "$case_dir/XFAIL" ]; then
      printf 'xfail  %-24s %s  — %s\n' "$name" "$profile" "$(head -1 "$case_dir/XFAIL")"; xfail=$((xfail+1))
    else
      printf 'FAIL   %-24s %s\n' "$name" "$profile"; fail=$((fail+1))
      for s1 in $srcs; do
        $CC "-std=$std" $FLAGS -Wno-unknown-pragmas $omps $inc -fsyntax-only "$s1" 2>&1 | sed 's/^/         /'
      done | head -8
    fi
  done
done
printf '\n%d ok, %d fail, %d xfail, %d xpass, %d wip\n' $ok $fail $xfail $xpass $wip
[ $fail -eq 0 ] && [ $xpass -eq 0 ]
