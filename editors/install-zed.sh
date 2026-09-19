#!/usr/bin/env sh
# Publishes the Zed extension into its OWN directory, with `git init`.
#
# This is required: Zed demands that an extension be a Git repository, and
# refuses a subdirectory of another repository — verified in practice, not
# just in the docs. And `editors/` cannot simply live outside the keel
# repository, because `generate.py` reads the §2.2 table of `keel-spec.md`,
# which sits next to it.
#
#   ./editors/install-zed.sh [dest]     (default: ~/.local/share/keel-zed)
#
# Then, in Zed: command palette → `zed: install dev extension` → pick the
# destination. Idempotent: running again updates the files and commits if
# anything changed. Touched highlights.scm? Run again and `zed: reload extensions`.
set -eu

source_dir=$(CDPATH= cd -- "$(dirname -- "$0")/zed" && pwd)
dest=${1:-$HOME/.local/share/keel-zed}

[ -e "$dest/.git" ] || [ ! -e "$dest" ] || {
    echo "refusing: $dest exists and is not a Git repository" >&2
    echo "pick another destination, or delete that directory." >&2
    exit 1
}

mkdir -p "$dest"
(cd "$source_dir" && find . -type f -not -path './.git/*' -print) | while read -r f; do
    mkdir -p "$dest/$(dirname "$f")"
    cp "$source_dir/$f" "$dest/$f"
done

if [ ! -d "$dest/.git" ]; then
    git -C "$dest" init -q
    echo "git init in $dest"
fi
git -C "$dest" add -A
if git -C "$dest" diff --cached --quiet; then
    echo "nothing changed."
else
    git -C "$dest" commit -qm "keel: Zed extension, generated from editors/zed/"
    echo "updated."
fi

echo
echo "extension in: $dest"
echo "in Zed: command palette → 'zed: install dev extension' → that directory"
