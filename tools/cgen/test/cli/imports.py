"""Hand-derived integration cases; no expected output is regenerated from cgen."""
from pathlib import Path
import os
import subprocess
import tempfile

repo = Path.cwd()
base = repo / 'base'
binary = repo / 'tools/cgen/cgen'

def run(root, module, expected=0, diagnostic=None):
    result = subprocess.run([str(binary), '--base-dir', str(base), '-I', str(root),
                             '--stop-after=parse', str(root / (module + '.k'))],
                            text=True, capture_output=True, timeout=5)
    assert result.returncode == expected, (module, result.returncode, result.stderr)
    if diagnostic:
        assert '[' + diagnostic + ']' in result.stderr, result.stderr
        assert not result.stdout, result.stdout
    else:
        assert not result.stderr, result.stderr
    return result

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    def write(name, source):
        p = root / (name + '.k')
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(source)
    write('missing', 'module missing;\nimport absent;\n')
    r = run(root, 'missing', 1, 'module-not-found')
    assert 'missing.k:2:1:' in r.stderr
    write('wrong', 'module another;\n')
    run(root, 'wrong', 1, 'module-path-mismatch')
    write('a', 'module a; import b;')
    write('b', 'module b; import a;')
    run(root, 'a', 1, 'circular-import')
    write('bad', 'module bad;\n#define keel_bad 1\n')
    write('usebad', 'module usebad; import bad;')
    r = run(root, 'usebad', 1, 'define-over-keel-name')
    assert r.stderr.count('[define-over-keel-name]') == 1, r.stderr
    assert 'bad.k:2:' in r.stderr
    for source in ['module truncated\ni32 x;',
                   'module truncated; import_c <stdio.h;',
                   'module truncated; int f(void) {return 0;']:
        write('truncated', source)
        run(root, 'truncated', 1, 'unexpected-token')
    # Diamond: a completed shared dependency is not a cycle.
    write('a', 'module a; import b; import c;')
    write('b', 'module b; import c;')
    write('c', 'module c;')
    run(root, 'a')
    write('coll', 'module coll type T; pub modifier stack { T *ptr; }')
    write('dims', 'module dims dim N type T; pub modifier block { T data[N]; }')
    write('types', 'module types; pub struct Point { i32 x; }; priv struct Hidden { i32 x; };')
    write('sample', '''module sample;
import coll as c types;
import dims types;
import types as g;
constexpr size_t N = 3;
c . stack i32 first, second;
stack int32_t same;
stack char const text;
stack const char same_text;
block(03) i32 grid;
block(N) i32 same_grid;
c.stack struct g.Point points;
array i32 x[2], y[3];
struct S { i32 k; } s1, s2;
''')
    output = run(root, 'sample').stdout
    declarations = [line.split('\t')[2] for line in output.splitlines() if line.startswith('decl\t')]
    for name in ['N', 'first', 'second', 'same', 'text', 'same_text', 'grid', 'same_grid', 'points', 'x', 'y', 'S', 's1', 's2']:
        assert name in declarations, (name, output)
    instances = [line.split('\t')[2] for line in output.splitlines() if line.startswith('inst\t')]
    assert instances == ['coll_stack_i32', 'coll_stack_const_char', 'dims_block_3_i32', 'coll_stack_types_Point'], output
    # The library root comes last; explicit source roots take precedence.
    write('keel/buffer', 'module keel.buffer type T; modifier special { T x; }')
    write('override', 'module override; import keel.buffer as b; b.special i32 value;')
    assert 'keel_buffer_special_i32' in run(root, 'override').stdout
    # Generated names: 255 characters accepted, 256 diagnosed, no truncation.
    for size in (249, 250):
        modifier = 'M' * size
        write('s', f'module s type T; pub modifier {modifier} {{ T value; }}')
        write('name_limit', f'module name_limit; import s types; {modifier} i32 value;')
        run(root, 'name_limit', 0 if size == 249 else 1,
            None if size == 249 else 'name-too-long')

print('ok')
