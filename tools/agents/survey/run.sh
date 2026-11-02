#!/bin/sh
# Survey of the forms the emitter still refuses (tools/agents/levantamento-2026-11-02.md).
# Each app/*.k is one form; the line says, per profile, OK, LIMIT (implementation-limit)
# or ERR (any other diagnostic: the survey's .k is wrong). Run from anywhere.
B=$(cd "$(dirname "$0")/../../.." && pwd)
cd "$(dirname "$0")" || exit 2
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
for f in app/*.k app/lib/*.k; do
  n=$(basename "$f" .k); line=""
  for p in c23 c11; do
    inst=$(sed -n 's|^// instance: ||p' "$f" | head -1)
    if [ -n "$inst" ]; then
      out=$("$B/tools/cgen/cgen" -std=$p --base-dir "$B/base" --stop-after=gen -I . --dest-dir "$tmp/$n$p" --instance "$inst" 2>&1 | grep -v ": info:\|redundant-instance")
    else
      out=$("$B/tools/cgen/cgen" -std=$p --base-dir "$B/base" --stop-after=gen -I . --dest-dir "$tmp/$n$p" "$f" 2>&1 | grep -v ": info:")
    fi
    if [ -z "$out" ]; then s=OK
    elif echo "$out" | grep -q implementation-limit; then s=LIMIT
    else s="ERR: $(echo "$out" | head -1 | cut -c1-100)"; fi
    line="$line [$p $s]"
  done
  echo "$n$line"
done
