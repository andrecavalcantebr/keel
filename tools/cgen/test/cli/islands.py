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
    'import keel.slice as slice;',                                # 4
    'import keel.outcome as outcome types; import keel.array as array;',  # 5
    'pub constexpr size_t N = 4;',                                # 6
    'pub tags Mode [FAST, SLOW];',                                # 7
    '',                                                           # 8
    'priv i32 twice(i32 v) { return v * 2; }',                    # 9
    'pub slice.slice i32 view(i32 *p) {',                               # 10
    '    return slice.from(i32, p, N);',                          # 11
    '}',                                                          # 12
    'pub size_t count(buffer i32 *b) { buffer i32 c = {0};',     # 13
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
    'pub i32 pick(slice.slice i32 s, array i32 m[2,2]) { buffer i32 c = {0};',  # 28
    '    grid[1,2] = c[3] + m[1,1];',                             # 29
    '    slice.slice i32 t = s[1..];',                                  # 30
    '    return t[0] + unknown[1,2];',                            # 31
    '}',                                                          # 32
    'pub buffer i32 gbuf;',                                       # 33
    'pub i32 more(arena *a) { buffer i32 c = {0};',              # 34
    '    arena.reset_all(a);',                                    # 35
    '    slice.slice i32 w = slice.of(c);',                             # 36
    '    return gbuf[0] + outcome.OK + (i32)buffer.length(c);',   # 37
    '}',                                                          # 38
    'pub typedef struct { buffer i32 items; array i32 tab[2,2]; } Box;',   # 39
    'pub i32 pick(Box *w, Box v) { buffer buffer i32 grid = {0};',  # 40
    '    i32 a = w->items[1];',                                   # 41
    '    i32 b = v.tab[1,1];',                                    # 42
    '    i32 d = grid[3][7];',                                    # 43
    '    return a + b + d + (i32)buffer.length(w->items) + (i32)slice.length(buffer.as_slice(v.items));',  # 44
    '}',                                                          # 45
    'pub size_t rows(void) { return keel.length(grid) + keel.dim(grid, 1); }',   # 46
    'pub size_t views(void) {',                                   # 47
    '    array f32 fx[8];',                                       # 48
    '    slice.slice f32 p = fx[2..5];',                                # 49
    '    slice.slice f32 q = fx[3..];',                                 # 50
    '    slice.slice f32 r = fx[..4];',                                 # 51
    '    slice.slice f32 w = fx[..];',                                  # 52
    '    return slice.length(p) + slice.length(q) + slice.length(r) + slice.length(w);',  # 53
    '}',                                                          # 54
]

# (kind, detail, line, needle, which occurrence of the needle in the line)
EXPECTED = [
    ('type', 'slice.slice i32 → keel_slice_i32', 10, 'slice', 0),
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
    ('type', 'slice.slice i32 → keel_slice_i32', 28, 'slice', 0),
    ('array', 'm [2,2]', 28, 'array', 0),
    ('type', 'buffer i32 → keel_buffer_i32', 28, 'buffer', 0),
    ('array-index', 'grid rank 2', 29, 'grid', 0),
    ('index', 'c → keel_buffer_i32_ptr1 &1', 29, 'c[', 0),
    ('array-index', 'm rank 2', 29, 'm[', 0),
    ('type', 'slice.slice i32 → keel_slice_i32', 30, 'slice', 0),
    ('range-index', 's 1.. → keel_slice_i32_as_slice keel_slice_i32_length', 30, 's[', 0),
    ('index', 't → keel_slice_i32_ptr1', 31, 't[', 0),
    # a qualified name is an island whether or not the module declares it, an
    # index over a file-scope symbol is one, and slice.of over a buffer is its instance (backend §5.19)
    ('type', 'buffer i32 → keel_buffer_i32', 33, 'buffer', 0),
    ('type', 'arena → keel_arena', 34, 'arena', 0),
    ('type', 'buffer i32 → keel_buffer_i32', 34, 'buffer', 0),
    ('call', 'arena.reset_all/1 → keel_arena_reset_all', 35, 'arena', 0),
    ('type', 'slice.slice i32 → keel_slice_i32', 36, 'slice', 0),
    ('call', 'slice.of/1 → keel_slice_of_keel_buffer_i32 &1', 36, 'slice.of', 0),
    ('index', 'gbuf → keel_buffer_i32_ptr1 &1', 37, 'gbuf', 0),
    ('name', 'outcome.OK → keel_outcome_OK', 37, 'outcome.OK', 0),
    ('call', 'buffer.length/1 → keel_buffer_i32_length &1', 37, 'buffer.length', 0),
    # a container is any expression of the `container` production: a field, an
    # element that is itself a container, the result of a verb
    ('type', 'Box → app_t_Box', 40, 'Box', 0),
    ('type', 'Box → app_t_Box', 40, 'Box', 1),
    # nested names keep the whole identity, as the golden's expected C does (backend §2.1, rule 3 says otherwise)
    ('type', 'buffer buffer i32 → keel_buffer_keel_buffer_i32', 40, 'buffer', 0),
    ('index', 'w->items → keel_buffer_i32_ptr1 &1', 41, 'w->items', 0),
    ('array-index', 'v.tab rank 2', 42, 'v.tab', 0),
    ('index', 'grid → keel_buffer_keel_buffer_i32_ptr1 &1', 43, 'grid', 0),
    ('index', 'grid[3] → keel_buffer_i32_ptr1 &1', 43, 'grid', 0),
    ('call', 'buffer.length/1 → keel_buffer_i32_length &1', 44, 'buffer.length', 0),
    ('call', 'slice.length/1 → keel_slice_i32_length', 44, 'slice.length', 0),
    ('call', 'buffer.as_slice/1 → keel_buffer_i32_as_slice &1', 44, 'buffer.as_slice', 0),
    # the core's operations lower to no function; they are islands all the same
    ('call', 'keel.length/1 → core dim:1', 46, 'keel.length', 0),
    ('name', 'grid → app_t_grid', 46, 'grid', 0),
    ('call', 'keel.dim/2 → core dim:1', 46, 'keel.dim', 0),
    ('name', 'grid → app_t_grid', 46, 'grid', 1),
    # x[a..b] over an array is the as_slice of keel.array: the element gives the instance
    ('array', 'fx [8]', 48, 'array', 0),
    ('type', 'slice.slice f32 → keel_slice_f32', 49, 'slice', 0),
    ('range-index', 'fx 2..5 → keel_array_f32_as_slice2 dim:1', 49, 'fx', 0),
    ('type', 'slice.slice f32 → keel_slice_f32', 50, 'slice', 0),
    ('range-index', 'fx 3.. → keel_array_f32_as_slice2 core dim:1', 50, 'fx', 0),
    ('type', 'slice.slice f32 → keel_slice_f32', 51, 'slice', 0),
    ('range-index', 'fx ..4 → keel_array_f32_as_slice2 dim:1', 51, 'fx', 0),
    ('type', 'slice.slice f32 → keel_slice_f32', 52, 'slice', 0),
    ('range-index', 'fx .. → keel_array_f32_as_slice2 core dim:1', 52, 'fx', 0),
    ('call', 'slice.length/1 → keel_slice_f32_length', 53, 'slice.length', 0),
    ('call', 'slice.length/1 → keel_slice_f32_length', 53, 'slice.length', 1),
    ('call', 'slice.length/1 → keel_slice_f32_length', 53, 'slice.length', 2),
    ('call', 'slice.length/1 → keel_slice_f32_length', 53, 'slice.length', 3),
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
