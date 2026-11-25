"""A bodyless draft is visible to analysis but cannot become a C symbol."""
from pathlib import Path
import subprocess
import tempfile

repo = Path.cwd()
binary = __import__('pathlib').Path(__import__('os').environ.get('CGEN_BIN') or repo / 'tools/cgen/cgen')

with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)

    def write(name, source):
        (root / (name + '.k')).write_text(source)

    def run(name, phase='gen'):
        out = root / name
        result = subprocess.run(
            [binary, '--base-dir', repo / 'base', '-I', root,
             '--stop-after=' + phase, '--dest-dir', out, root / (name + '.k')],
            text=True, capture_output=True, timeout=10)
        return result, out

    def compile_c(name, out):
        subprocess.run(['cc', '-std=c11', '-pedantic-errors', '-Werror', '-I', out,
                        '-fsyntax-only', out / (name + '.c')], check=True, timeout=10)

    write('draft', '''module draft;
pub i32 future(i32 n);
priv i32 local_later(void);
pub i32 ready(void) { return 7; }
''')
    result, out = run('draft')
    assert result.returncode == 0, result.stderr
    assert result.stderr.count('[function-body-pending]') == 2, result.stderr
    assert ':2:9: warning:' in result.stderr, result.stderr
    for generated in out.glob('draft.*'):
        assert 'future' not in generated.read_text(), generated
    assert 'i32 draft_ready(void);' in (out / 'draft.h').read_text()
    compile_c('draft', out)

    write('unfinished', '''module unfinished;
pub i32 future(i32 n);
pub i32 use(void) { return future(7); }
''')
    result, _ = run('unfinished', 'parse')
    assert result.returncode == 0, result.stderr
    assert '[function-body-pending]' in result.stderr, result.stderr
    assert '[use-of-draft-function]' not in result.stderr, result.stderr
    assert 'future/1' in result.stdout, result.stdout
    result, out = run('unfinished')
    assert result.returncode != 0, result.stderr
    assert 'unfinished.k:3:28: error:' in result.stderr, result.stderr
    assert '[use-of-draft-function]' in result.stderr, result.stderr
    assert not (out / 'unfinished.c').exists()

    write('facade', 'module facade;\npub import draft as api;\n')
    write('client', '''module client;
import facade as f;
pub i32 use(void) { return f.api.future(7); }
pub i32 (*callback)(i32) = f.api.future;
''')
    result, _ = run('client')
    assert result.returncode != 0, result.stderr
    assert result.stderr.count('[use-of-draft-function]') == 2, result.stderr
    assert 'client.k:3:28: error:' in result.stderr, result.stderr

    write('forward', '''module forward;
priv i32 first(i32 n);
pub i32 second(i32 n);
pub i32 run(void) { return first(3) + second(4); }
priv i32 first(i32 n) { return n; }
pub i32 second(i32 n) { return n; }
''')
    result, out = run('forward')
    assert result.returncode == 0 and 'function-body-pending' not in result.stderr, result.stderr
    compile_c('forward', out)

    write('overload', '''module overload;
pub i32 future(i32 n);
pub i32 future(void) { return 7; }
pub i32 use(void) { return future(); }
''')
    result, out = run('overload')
    assert result.returncode == 0, result.stderr
    assert result.stderr.count('[function-body-pending]') == 1, result.stderr
    assert 'i32 overload_future(void);' in (out / 'overload.h').read_text()
    assert 'return overload_future();' in (out / 'overload.c').read_text()
    compile_c('overload', out)

    write('boxes', '''module boxes type T;
pub modifier box { T value; }
pub T fallback(box *b);
pub inline T get(box *b) { return b->value; }
''')
    write('consumer', '''module consumer;
import boxes as b;
pub i32 use(void) {
    b.box i32 value = {.value = 7};
    return b.get(value);
}
''')
    result, out = run('consumer')
    assert result.returncode == 0, result.stderr
    assert '[function-body-pending]' in result.stderr, result.stderr
    assert 'fallback' not in (out / 'boxes_box_i32.h').read_text()
    assert 'fallback' not in (out / 'boxes.h').read_text()
    compile_c('consumer', out)

    boxes = (root / 'boxes.k').read_text()
    write('boxes', boxes.replace('return b->value;', 'return fallback(b);'))
    result, _ = run('consumer')
    assert result.returncode != 0, result.stderr
    assert 'boxes.k:4:35: error:' in result.stderr, result.stderr
    assert '[use-of-draft-function]' in result.stderr, result.stderr

    write('boxes', boxes + '''
priv i32 unsupported(void);
pub inline i32 shared(void) { return unsupported(); }
''')
    result, _ = run('consumer')
    assert result.returncode != 0, result.stderr
    assert 'boxes.k:7:38: error:' in result.stderr, result.stderr
    assert '[use-of-draft-function]' in result.stderr, result.stderr

print('ok')
