#!/bin/sh
# Failure cases: sources written to be refused, and the catalog's coverage
# (diag-design §7, [G3]). Run from the repository root.
set -eu
python3 tools/cgen/gen-diags.py --check
python3 tools/cgen/test/diag.py
