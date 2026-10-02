"""Load-state and closure regressions against a temporary module graph."""
from pathlib import Path
import os
import subprocess
import tempfile

probe = r'''
#include <stdio.h>
#include "tool/memory.h"
#include <stdlib.h>
#include <string.h>
#include "tool/tool.h"
int main(int argc,char **argv) {
    keel_arena arena; if (!cgen_memory_init(&arena)) return 2;
    KDiagnostic ds[128]; KDiagnosticSink sink; k_diag_init(&sink,ds,128);
    void *tool=calloc(1,cgen_loader_size()); KLoader loader;
    const char *roots[]={argv[1]}; cgen_loader_init(tool,&arena,roots,1,&sink,&loader);
    for(int i=2;i<argc;i++) {
        KModule *m=NULL; size_t before=k_diag_count(&sink,K_ERROR);
        KLoadResult result=loader.load(loader.tool,k_diag_text(argv[i]),&m);
        printf("%s %d %zu %lld %zu\n",argv[i],result,k_diag_count(&sink,K_ERROR)-before,
               m?m->closure_mtime:0,m?m->symbol_count:0);
    }
    cgen_loader_destroy(tool);free(tool);cgen_memory_destroy(&arena);
}
'''
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    source=root/'probe.c'; source.write_text(probe)
    binary=root/'probe'
    files=list(Path('tools/cgen/src/engine').glob('*.c'))
    files += [Path('tools/cgen/src/tool')/x for x in ['tool.c','paths.c','read_source.c','report.c']]
    subprocess.run(['gcc','-std=c2x','-Itools/cgen/src','-Itools/cgen/gen',
                    '-fsanitize=address,undefined','-fno-sanitize-recover=all','-g',
                    str(source),*map(str,files),'-o',str(binary)],check=True)
    modules={'keel':'module keel;', 'new':'module new;', 'old':'module old;',
             'parent':'module parent; import old;', 'wrong':'module other;',
             'bad':'module bad;\n#define keel_bad 1\n',
             'missing':'module missing; import absent;',
             'many':'module many;\n'+'\n'.join(f'constexpr int C{i}={i};' for i in range(300))}
    for name,text in modules.items():
        f=root/(name+'.k');f.write_text(text);os.utime(f,(1000,1000))
    os.utime(root/'new.k',(2000,2000))
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
    result=subprocess.run([str(binary),str(root),'new','old','parent','wrong','bad','bad','missing','many'],
                          capture_output=True,text=True,timeout=10,env=env)
    assert result.returncode==0,result.stderr
    assert result.stdout.splitlines()==[
        'new 0 0 2000 0','old 0 0 1000 0','parent 0 0 1000 0',
        'wrong 4 1 0 0','bad 4 1 0 0','bad 4 0 0 0','missing 4 1 0 0',
        'many 0 0 1000 300'],result.stdout
print('ok')
