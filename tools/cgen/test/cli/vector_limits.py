"""CLI capacity boundaries, including argv terminator and paired options."""
from pathlib import Path
import os
import subprocess
import tempfile

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    cc = root / 'cc'
    cc.write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n')
    cc.chmod(0o755)
    sources = sorted(Path('tools/cgen/src').rglob('*.c'))
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    for roots_cap, args_cap in [(128, 256), (2, 4), (129, 260)]:
        binary = root / 'cgen'
        subprocess.run(['gcc', '-std=c2x', '-Itools/cgen/src', '-Itools/cgen/gen',
                        f'-DCGEN_CLI_ROOTS_CAP={roots_cap}', f'-DCGEN_CLI_ARGS_CAP={args_cap}',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-g',
                        *map(str, sources), '-o', str(binary)], check=True)
        def run(args, error=None, setting=None):
            result = subprocess.run([str(binary), '--cc', str(cc), *args],
                                    text=True, capture_output=True, env=env, timeout=10)
            assert 'AddressSanitizer' not in result.stderr and 'runtime error:' not in result.stderr, result.stderr
            if error:
                assert result.returncode == 2 and not result.stdout, result
                assert f'[{error}]' in result.stderr, result.stderr
                if setting:
                    assert setting in result.stderr and 'configured limit' in result.stderr, result.stderr
            else:
                assert result.returncode == 0 and not result.stderr, result
                assert result.stdout.splitlines() == args, result.stdout
        # Full passthrough vector still leaves executable + NULL in execvp argv.
        run(['x.o'] * args_cap)
        run(['-g'] * args_cap)
        run(['x.o'] * (args_cap + 1), 'implementation-limit', 'CGEN_CLI_ARGS_CAP')
        run(['x.o'] * (args_cap - 2) + ['-o', 'program'])
        run(['x.o'] * (args_cap - 1) + ['-o', 'program'], 'implementation-limit', 'CGEN_CLI_ARGS_CAP')
        run(['-I.'] * roots_cap)
        run(['-I.'] * (roots_cap + 1), 'implementation-limit', 'CGEN_CLI_ROOTS_CAP')
        run(['-I', '.'] * roots_cap)
        run(['-I'], 'invalid-option')
        run(['-o'], 'invalid-option')
        # All explicit roots plus the implicit base survive the parser/loader handoff.
        source = root / 'sample.k'
        source.write_text('module sample;')
        result = subprocess.run([str(binary), '--base-dir', str(Path('base').resolve()),
                                 '--stop-after=parse', *(['-I', str(root)] * roots_cap), str(source)],
                                text=True, capture_output=True, env=env, timeout=10)
        assert result.returncode == 0 and not result.stderr, result.stderr
print('ok')
