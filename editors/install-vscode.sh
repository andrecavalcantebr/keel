#!/usr/bin/env sh
# Installs the VSCode extension as a development extension.
#
# VSCode loads any directory under `extensions/` that has a valid
# `package.json`, and follows symlinks — which is what we want here: running
# `generate.py` again updates the installed extension without reinstalling.
#
#   ./editors/install-vscode.sh              # link to editors/vscode/
#   ./editors/install-vscode.sh --copy       # copy, to take to another machine
#
# Destination: $VSCODE_EXT, or ~/.vscode/extensions. For Remote-SSH use
# VSCODE_EXT=~/.vscode-server/extensions; for VSCodium, ~/.vscode-oss/extensions.
# Then: command palette → `Developer: Reload Window`.
set -eu

source_dir=$(CDPATH= cd -- "$(dirname -- "$0")/vscode" && pwd)
mode=${1:---link}
root=${VSCODE_EXT:-$HOME/.vscode/extensions}

field() { sed -n "s/.*\"$1\": *\"\([^\"]*\)\".*/\1/p" "$source_dir/package.json" | head -1; }
name="$(field publisher).$(field name)-$(field version)"
dest="$root/$name"

[ -d "$root" ] || { echo "$root not found — is VSCode installed?" >&2; exit 1; }

if [ -e "$dest" ] && [ ! -L "$dest" ]; then
    echo "refusing: $dest exists and is not a link." >&2
    echo "delete it, or install with VSCODE_EXT pointing elsewhere." >&2
    exit 1
fi
rm -f "$dest"

case "$mode" in
  --link)
    ln -s "$source_dir" "$dest"
    echo "link: $dest -> $source_dir" ;;
  --copy)
    mkdir -p "$dest"
    (cd "$source_dir" && find . -type f -print) | while read -r f; do
        mkdir -p "$dest/$(dirname "$f")"
        cp "$source_dir/$f" "$dest/$f"
    done
    echo "copied to: $dest" ;;
  *) echo "usage: $0 [--link|--copy]" >&2; exit 2 ;;
esac

echo
echo "in VSCode: command palette → 'Developer: Reload Window'"
echo "to check: open a .k and run 'Developer: Inspect Editor Tokens and Scopes'"
