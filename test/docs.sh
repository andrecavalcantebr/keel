#!/usr/bin/env sh
# Marked code blocks of the documents are literal excerpts of golden/ files
# (see test/docs.py). Runs from any directory.
cd "$(dirname "$0")/.." || exit 2
exec python3 -I test/docs.py README.md design/keel-llm-context.md
