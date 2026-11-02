#!/bin/sh
# T24 — terceiro levantamento (tools/agents/tasks/T24-survey3.md).
# Varre app3/*.k nos dois perfis. Por perfil: OK (sem diagnóstico); WARN (só
# warnings: o .k é aceito); LIMIT (implementation-limit); ERR (outro erro, o
# .k pode estar errado); para OK e WARN, o C gerado é compilado com cc
# -fsyntax-only: BAD = não compila (tradução errada). CGEN aponta para a cópia
# do binário. Temporários na scratchpad.
S=/tmp/claude-1000/-home-andre-code-cprojects-keel/6df5c4e9-8d62-4717-9b96-cb209ab7b812/scratchpad
B=$(cd "$(dirname "$0")/../../.." && pwd)
CGEN=${CGEN:-$S/cgen-t24}
cd "$(dirname "$0")" || exit 2
tmp=$(mktemp -d "$S/run3.XXXXXX")
for f in app3/*.k app3/lib/*.k; do
  n=$(basename "$f" .k); line=""
  for p in c23 c11; do
    std=$p; [ "$p" = c23 ] && std=c2x
    d="$tmp/$n$p"
    out=$("$CGEN" -std=$p --base-dir "$B/base" --stop-after=gen -I . --dest-dir "$d" "$f" 2>&1 | grep -v ": info:")
    errs=$(echo "$out" | grep -v ': warning:\|^$')
    if [ -n "$errs" ]; then
      if echo "$errs" | grep -q implementation-limit; then s=LIMIT
      elif echo "$errs" | grep -q "generic module; use --instance"; then s=LIB
      else s="ERR: $(echo "$errs" | head -1 | cut -c1-120)"; fi
    else
      s=OK; [ -n "$out" ] && s=WARN
      bad=""
      for c in $(find "$d" -name '*.c' 2>/dev/null); do
        err=$(cc -std=$std -fsyntax-only -Wall -I "$d" "$c" 2>&1 | grep -m1 'error')
        [ -n "$err" ] && bad="$bad $(echo "$err" | cut -c1-110)"
      done
      [ -n "$bad" ] && s="BAD:$bad"
    fi
    line="$line [$p $s]"
  done
  echo "$n$line"
done
rm -rf "$tmp"
