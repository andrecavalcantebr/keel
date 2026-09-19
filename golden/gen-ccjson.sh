#!/usr/bin/env sh
# Emits golden/compile_commands.json so clangd (Zed, VSCode, vim) can resolve
# `#include "keel.type.h"` and each case's generated headers.
#
# The fixed headers live in golden/<profile>/keel/; the generated ones, in
# cases/<case>/expected/<profile>/. Without this file the editor has no way to
# know that, and finds nothing.
#
# A file's profile comes from its PATH. A file that belongs to no profile —
# proof.c, which is the harness and not output — is entered once, under c23.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$root"

entry() {  # $1 = relative file, $2 = profile, $3 = case
  [ "$2" = c23 ] && std=c2x || std=c11
  inc="-I$root/$2"
  [ -d "$3/expected/$2" ] && inc="$inc -I$root/$3/expected/$2"
  [ -d "$3/$2" ]          && inc="$inc -I$root/$3/$2"
  inc="$inc -I$root/$3"
  printf '%s  {"directory":"%s","file":"%s/%s","command":"cc -std=%s %s -c %s/%s"}\n' \
         "$sep" "$root" "$root" "$1" "$std" "$inc" "$root" "$1"
  sep=","
}

printf '[\n' > compile_commands.json
sep=""
for case_dir in cases/*; do
  for f in $(find "$case_dir" -name '*.c' | sort); do
    case "$f" in
      */expected/c23/*) p=c23 ;;
      */expected/c11/*) p=c11 ;;
      *.c11.c)          p=c11 ;;
      *)                p=c23 ;;     # proof.c and anything not profile-specific
    esac
    entry "$f" "$p" "$case_dir" >> compile_commands.json
  done
done
printf ']\n' >> compile_commands.json
echo "compile_commands.json: $(grep -c '"file"' compile_commands.json) entries"
