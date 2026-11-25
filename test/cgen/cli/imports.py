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
        assert ': error: ' not in result.stderr and ': warning: ' not in result.stderr, result.stderr
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
    for source, diagnostic in [('module truncated\ni32 x;', 'unexpected-token'),
                               ('module truncated; import_c <stdio.h;', 'unexpected-token'),
                               ('module truncated; int f(void) {return 0;', 'unmatched-delimiter'),
                               ('module truncated; import keel.slice', 'unexpected-eof')]:
        write('truncated', source)
        run(root, 'truncated', 1, diagnostic)
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
    # Generated names: 215 characters accepted, 216 diagnosed, no truncation (the symbol is `s_`, the modifier, `_i32`).
    for size in (209, 210):
        modifier = 'M' * size
        write('s', f'module s type T; pub modifier {modifier} {{ T value; }}')
        write('name_limit', f'module name_limit; import s types; {modifier} i32 value;')
        run(root, 'name_limit', 0 if size == 209 else 1,
            None if size == 209 else 'name-too-long')

    # Published paths are direct, but neither a private edge nor an
    # injected name becomes part of a facade's interface.
    write('leaf', '''module leaf;
pub typedef i32 Count;
pub inline i32 value(void) { return 7; }
priv i32 secret = 8;
''')
    write('middle', 'module middle;\npub import leaf as child types;\n')
    write('facade', 'module facade;\npub import middle as mid;\n')
    write('accepted', 'module accepted; import facade as f; '
                      'pub i32 f1(void) { f.mid.child.Count n = f.mid.child.value(); return n; }')
    r = run(root, 'accepted')
    assert '[indirect-import]' not in r.stderr, r.stderr
    assert 'leaf_value' in r.stdout, r.stdout

    write('closed', 'module closed;\nimport leaf as child;\n')
    for name, dependency, use in [
        ('private_edge', 'closed', 'return f.child.value();'),
        ('private_decl', 'facade', 'return f.mid.child.secret;'),
        ('own_name', 'facade', 'return f.mid.leaf.value();'),
        ('injection', 'facade', 'Count n = 0; return n;'),
    ]:
        write(name, f'''module {name};
import {dependency} as f;
pub i32 use(void) {{
    {use}
}}
''')
        out = root / name
        generated = subprocess.run(
            [binary, '--base-dir', base, '-I', root, '--stop-after=gen',
             '--dest-dir', out, root / (name + '.k')],
            text=True, capture_output=True, timeout=5)
        if generated.returncode == 0:
            compiled = subprocess.run(
                [os.environ.get('CC', 'cc'), '-std=c11', '-Werror', '-fsyntax-only',
                 '-I', out, out / (name + '.c')],
                text=True, capture_output=True, timeout=5)
            assert compiled.returncode != 0, (name, compiled.stdout, compiled.stderr)
        else:
            assert '[implementation-limit]' not in generated.stderr, generated.stderr
            assert ': error:' in generated.stderr, generated.stderr

    write('priv_import', 'module priv_import; priv import leaf;')
    run(root, 'priv_import', 1, 'unexpected-token')

    # The leaf's mangled prefix also matches the directly imported package
    # module. The qualified use must select the leaf's header.
    write('pkg', 'module pkg;')
    write('pkg/leaf', '''module pkg.leaf;
pub inline i32 value(void) { return 7; }
''')
    write('facade', 'module facade;\npub import pkg . leaf;\n')
    write('dotted', '''module dotted;
import pkg;
import facade as f;
pub i32 use(void) {
    return f . pkg . leaf . value();
}
''')
    run(root, 'dotted')

    # A published path can reach beyond the 64 modules used by the
    # historical indirect-import worklist.
    for index in range(67):
        write(f'edge{index}', f'module edge{index};\npub import edge{index + 1} as next;\n')
    write('edge67', 'module edge67;\npub inline i32 value(void) { return 7; }\n')
    use = 'start' + '.next' * 67 + '.value()'
    write('deep', f'''module deep;
import edge0 as start;
pub i32 use(void) {{
    return {use};
}}
''')
    for name, header in [('dotted', 'pkg/pkg_leaf.h'), ('deep', 'edge67.h')]:
        out = root / name
        subprocess.run([binary, '--base-dir', base, '-I', root, '--stop-after=gen',
                        '--dest-dir', out, root / (name + '.k')],
                       check=True, timeout=10)
        assert f'#include "{header}"' in (out / (name + '.c')).read_text()
        subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Werror', '-fsyntax-only',
                        '-I', out, out / (name + '.c')],
                       check=True, capture_output=True, timeout=5)

print('ok')
