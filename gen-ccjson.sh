#!/usr/bin/env sh
# Emits the compilation databases that clangd (Zed, VSCode, vim) reads to
# resolve the `#include`s and, through them, the types.
#
# The repository has SEVERAL header trees with the SAME names and different
# contents — `gen/keel/keel_buffer_i32.type.h` is not the one in
# `golden/c23/keel/`. A global `-I` would pick one of them for everyone and
# silently give wrong definitions. So each tree has its own database, and
# clangd finds the right one walking up from the open file:
#
#   ./compile_commands.json          tools/cgen/, tools/transform/ and the generated trees
#   golden/compile_commands.json     the suite, in both profiles
#
# The `-I`s are RELATIVE to each entry's "directory" field, which is the
# repository root. Only that one is absolute; moving the repo means rerunning
# this script.
#
# Headers get entries of their own. Without that, clangd guesses a `.h`'s
# flags from the "most similar" `.c`, and in a project that is nearly all
# headers the guess is wrong.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$root"

STD=c2x
sep=""

entry() {  # $1 = relative file, $2... = flags
  file=$1; shift
  printf '%s  {"directory":"%s","file":"%s","command":"cc -std=%s %s -c %s"}\n' \
         "$sep" "$root" "$file" "$STD" "$*" "$file"
  sep=","
}

{
  printf '[\n'

  # the cgen: sees the base generated in tools/cgen/gen and its own sources.
  # No -maxdepth: the sources live in src/tool/ and src/engine/ (cgen design
  # 3.1, D8), and emit/ is one level further down.
  for c in $(find tools/cgen/src -name '*.c' 2>/dev/null | sort); do
    entry "$c" "-I tools/cgen/gen -I tools/cgen/src"
  done

  # the bootstrap tools: self-contained
  for c in $(find tools/transform/src -name '*.c' 2>/dev/null | sort); do
    entry "$c" "-I tools/transform/src"
  done

  # each generated header tree answers for its own root: that is what makes
  # `#include "keel.type.h"` find the prelude and, with it, i32 and the rest.
  # tools/transform/gen is the local copy the transform generates to validate
  # itself (same source as tools/transform/base, a different dest-dir from
  # tools/cgen/gen).
  for tree in tools/cgen/gen tools/transform/gen; do
    [ -d "$tree" ] || continue
    for h in $(find "$tree" -name '*.h' | sort); do
      entry "$h" "-I $tree"
    done
  done

  printf ']\n'
} > compile_commands.json

echo "compile_commands.json: $(grep -c '"file"' compile_commands.json) entries"
[ -x golden/gen-ccjson.sh ] && ./golden/gen-ccjson.sh
