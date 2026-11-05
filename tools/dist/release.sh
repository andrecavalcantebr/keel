#!/bin/sh
# Publica uma release da distribuição: tag vX.Y.Z + GitHub Release com o
# tarball e o SHA256. A dist é um instantâneo congelado; o master segue evoluindo.
#
#   tools/dist/release.sh 0.1.0
#
# Pré-condições (o script confere): árvore limpa, em master, oráculos verdes.
set -eu
ver=${1:?uso: release.sh X.Y.Z}
cd "$(dirname "$0")/../.."
case $ver in [0-9]*.[0-9]*.[0-9]*) ;; *) echo "versão inválida: $ver" >&2; exit 1 ;; esac
[ "$(git rev-parse --abbrev-ref HEAD)" = master ] || { echo "release só a partir do master" >&2; exit 1; }
[ -z "$(git status --porcelain)" ] || { echo "árvore suja: commite antes" >&2; exit 1; }
git rev-parse "v$ver" >/dev/null 2>&1 && { echo "a tag v$ver já existe" >&2; exit 1; }
make check
make release VERSION="$ver"
git tag -a "v$ver" -m "keel $ver"
git push origin master "v$ver"
gh release create "v$ver" "release/keel-$ver.tar.gz" "release/keel-$ver.tar.gz.sha256" \
  --title "keel $ver" --generate-notes
