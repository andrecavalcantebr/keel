"""Island stage 4a through the CLI (parser design §3.2): what the three golden
.parse oracles do not reach — an instance chosen by a selection `type`
parameter, a pointer receiver that takes no `&`, an unresolved call that prints
nothing, a local that shadows a constant, `extern` and initialized arenas, tags
values. The expectation is written by hand, against backend §2.1.1 and spec
§4.4; positions come from the needle's place in the line, not from cgen."""
from pathlib import Path
import subprocess
import tempfile

repo = Path.cwd()
base = repo / 'base'
binary = repo / 'tools/cgen/cgen'

SOURCE = [
    'module app.t;',                                              # 1
    'import keel.arena as arena types;',                          # 2
    'import keel.buffer as buffer types;',                        # 3
    'import keel.slice as slice types;',                          # 4
    '',                                                           # 5
    'pub constexpr size_t N = 4;',                                # 6
    'pub tags Mode [FAST, SLOW];',                                # 7
    '',                                                           # 8
    'priv i32 twice(i32 v) { return v * 2; }',                    # 9
    'pub slice i32 view(i32 *p) {',                               # 10
    '    return slice.from(i32, p, N);',                          # 11
    '}',                                                          # 12
    'pub size_t count(buffer i32 *b, buffer i32 c) {',            # 13
    '    size_t n = buffer.length(b) + buffer.length(c);',        # 14
    '    n += twice((i32)n);',                                    # 15
    '    n += unknown(b);',                                       # 16
    '    return n;',                                              # 17
    '}',                                                          # 18
    'pub void scratch(void) {',                                   # 19
    '    arena a, b = {0};',                                      # 20
    '    Mode m = FAST;',                                         # 21
    '}',                                                          # 22
    'pub i32 shadow(void) {',                                     # 23
    '    i32 N = 1;',                                             # 24
    '    return N;',                                              # 25
    '}',                                                          # 26
    'pub array i32 grid[2,3];',                                   # 27
    'pub i32 pick(buffer i32 c, slice i32 s, array i32 m[2,2]) {',  # 28
    '    grid[1,2] = c[3] + m[1,1];',                             # 29
    '    slice i32 t = s[1..];',                                  # 30
    '    return t[0] + unknown[1,2];',                            # 31
    '}',                                                          # 32
]

# (kind, detail, line, needle, which occurrence of the needle in the line)
EXPECTED = [
    ('type', 'slice i32 → keel_slice_i32', 10, 'slice', 0),
    ('call', 'slice.from/3 → keel_slice_i32_from', 11, 'slice', 0),
    ('name', 'N → app_t_N', 11, 'N', 0),
    ('type', 'buffer i32 → keel_buffer_i32', 13, 'buffer', 0),
    ('type', 'buffer i32 → keel_buffer_i32', 13, 'buffer', 1),
    ('call', 'buffer.length/1 → keel_buffer_i32_length', 14, 'buffer.length', 0),
    ('call', 'buffer.length/1 → keel_buffer_i32_length &1', 14, 'buffer.length', 1),
    ('call', 'twice/1 → app_t_twice', 15, 'twice', 0),
    ('type', 'arena → keel_arena', 20, 'arena', 0),
    ('implicit-init', 'a', 20, 'a,', 0),
    ('type', 'Mode → app_t_Mode', 21, 'Mode', 0),
    ('name', 'FAST → app_t_Mode_FAST', 21, 'FAST', 0),
    # stage 4b
    ('array', 'grid [2,3]', 27, 'array', 0),
    ('type', 'buffer i32 → keel_buffer_i32', 28, 'buffer', 0),
    ('type', 'slice i32 → keel_slice_i32', 28, 'slice', 0),
    ('array', 'm [2,2]', 28, 'array', 0),
    ('array-index', 'grid rank 2', 29, 'grid', 0),
    ('index', 'c → keel_buffer_i32_ptr1 &1', 29, 'c[', 0),
    ('array-index', 'm rank 2', 29, 'm[', 0),
    ('type', 'slice i32 → keel_slice_i32', 30, 'slice', 0),
    ('range-index', 's 1.. → keel_slice_i32_of2 keel_slice_i32_length', 30, 's[', 0),
    ('index', 't → keel_slice_i32_ptr1', 31, 't[', 0),
]

def column(line, needle, which):
    at = -1
    for _ in range(which + 1):
        at = SOURCE[line - 1].index(needle, at + 1)
    return at + 1

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    (root / 'app').mkdir()
    (root / 'app/t.k').write_text('\n'.join(SOURCE) + '\n')
    result = subprocess.run([str(binary), '--base-dir', str(base), '-I', str(root),
                             '--stop-after=parse', 'app/t.k'],
                            text=True, capture_output=True, timeout=5, cwd=root)
    assert result.returncode == 0, (result.returncode, result.stderr)
    assert not result.stderr, result.stderr
    got = [l for l in result.stdout.split('\n') if l.startswith('ilha\t')]
    want = ['ilha\t%s\t%s\tapp/t.k:%d:%d' % (k, d, line, column(line, n, w))
            for k, d, line, n, w in EXPECTED]
    assert got == want, '\n'.join(['got:'] + got + ['want:'] + want)
print('ok')
