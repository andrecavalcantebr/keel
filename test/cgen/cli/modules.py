"""Hand-derived cases for the file-level checks of keel-spec §4.1 that a
failure case under diag/ cannot hold: names of files on disk."""
from pathlib import Path
import subprocess
import tempfile

repo = Path.cwd()
base = repo / 'base'
binary = repo / 'tools/cgen/cgen'

def run(root, path, expected, diagnostic):
    result = subprocess.run([str(binary), '--base-dir', str(base), '-I', str(root),
                             '--stop-after=parse', str(root / path)],
                            text=True, capture_output=True, timeout=5)
    assert result.returncode == expected, (path, result.returncode, result.stderr)
    assert '[' + diagnostic + ']' in result.stderr, result.stderr
    return result

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    # the stem names the module, so it is a C identifier
    (root / 'my-file.k').write_text('module my_file;\n')
    r = run(root, 'my-file.k', 1, 'invalid-stem')
    assert 'my-file.k:1:1:' in r.stderr, r.stderr
    (root / 'case').mkdir()
    (root / 'case' / 'Point.k').write_text('module case.Point;\n')
    (root / 'case' / 'point.k').write_text('module case.point;\n')
    run(root, 'case/point.k', 1, 'case-ambiguous-stem')
    # `module` comes first, before any directive
    (root / 'nomod.k').write_text('int x;\n')
    run(root, 'nomod.k', 1, 'missing-module')
    (root / 'late.k').write_text('#include <stdio.h>\nmodule late;\n')
    r = run(root, 'late.k', 1, 'missing-module')
    assert 'late.k:1:1:' in r.stderr, r.stderr

print('ok')
