#!/bin/sh
# Oracle of --stop-after=gen over golden/cases (cgen design §10): for each case
# listed in gen/golden-cases.txt (a name with a slash is a path from the
# repository root: the hand-written oracles under gen/) and each profile, every file under
# expected/<p>/ must be produced byte for byte. Files cgen writes beyond those
# are not judged here. A case whose emission hits `implementation-limit` is
# `wip`, as in golden/run.sh; any other failure is a FAIL.
cd "$(dirname "$0")/../.." || exit 2
BIN=$PWD/tools/cgen/cgen
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
bad=0 ok=0 wip=0
while IFS='|' read -r case src main flag; do
    case "$case" in ''|'#'*) continue ;; esac
    case "$case" in */*) dir=$PWD/$case ;; *) dir=$PWD/golden/cases/$case ;; esac
    state=ok
    for p in c23 c11; do
        key=$(printf %s "$case" | tr / _); out=$tmp/$key/$p; err=$tmp/$key.$p.err
        set -- -std="$p" --base-dir "$PWD/base" --stop-after=gen -I "$dir" --dest-dir "$out"
        [ "$main" = - ] || set -- "$@" --main "$main"
        if ! "$BIN" "$@" "$dir/$src" >/dev/null 2>"$err"; then
            if grep -q 'implementation-limit' "$err"; then state=wip; break; fi
            [ "$flag" = wip ] || { printf 'FAIL   %s %s: cgen\n' "$case" "$p"; sed -n '1,5s/^/       /p' "$err"; }
            state=fail; continue
        fi
        for f in $(cd "$dir/expected/$p" && find . -type f | sort); do
            cmp -s "$dir/expected/$p/$f" "$out/$f" || {
                [ "$flag" = wip ] || { printf 'FAIL   %s %s/%s differs\n' "$case" "$p" "${f#./}"
                diff "$dir/expected/$p/$f" "$out/$f" 2>&1 | head -6 | sed 's/^/       /'; }
                state=fail; }
        done
    done
    case $state in ok) ok=$((ok+1)) ;; wip) wip=$((wip+1)); printf 'wip    %s\n' "$case" ;; fail) if [ "$flag" = wip ]; then wip=$((wip+1)); printf 'wip    %s (differs)\n' "$case"; else bad=$((bad+1)); fi ;; esac
done < test/cgen/gen/golden-cases.txt
echo "$ok ok, $bad fail, $wip wip"
[ $bad -eq 0 ]
