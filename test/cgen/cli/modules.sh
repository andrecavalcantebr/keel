#!/bin/sh
# File-level checks of keel-spec 4.1 that need files on disk.
set -eu
python3 test/cgen/cli/modules.py
