#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tool/tool.h"
#include "tool/cli_limits.h"

static bool absolute(const char *path,char *out,size_t cap) {
    if(path[0]=='/')return cgen_path_normalize(path,out,cap);
    char cwd[4096],joined[8192];
    if(!getcwd(cwd,sizeof cwd))return false;
    int n=snprintf(joined,sizeof joined,"%s/%s",cwd,path);
    return n>=0&&(size_t)n<sizeof joined&&cgen_path_normalize(joined,out,cap);
}
int cgen_write_output(const char *path, const char *data, size_t n);

/* `out` is the file of `-o`, or NULL for stdout (cgen-tool-spec §4.2) */
int cgen_stop_after_parse(keel_arena *arena,const char *path,const char *const *roots,int n,const char *base,const char *out) {
    if (n < 0 || n > CGEN_CLI_ROOTS_CAP) {
        fprintf(stderr,"cgen: error: search roots exceed configured limit %d; adjust CGEN_CLI_ROOTS_CAP [implementation-limit]\n", CGEN_CLI_ROOTS_CAP);
        return 2;
    }
    char source[8192],normalized[8192],name[8192];
    if(!absolute(path,source,sizeof source))return 2;
    bool found=false;
    for(int i=0;i<n;i++)
        if(absolute(roots[i],normalized,sizeof normalized)&&
           cgen_module_name_of(source,normalized,name,sizeof name)){found=true;break;}
    /* Still lex the input first, so a lexical error keeps its diagnostic. */
    if(!found)name[0]='\0';
    const char *search[CGEN_SEARCH_ROOTS_CAP];
    for(int i=0;i<n;i++)search[i]=roots[i];
    search[n++]=base;
    KDiagnostic items[4096];KDiagnosticSink sink;k_diag_init(&sink,items,4096);
    void *tool=cgen_alloc(arena,1,cgen_loader_size(),_Alignof(max_align_t),true,"loader");if(!tool)return 2;
    KLoader loader;cgen_loader_init(tool,arena,search,n,&sink,&loader);
    KModule *module=NULL;
    KLoadResult result=cgen_load_path(tool,path,name,&module);
    cgen_loader_report(tool,&sink);
    int rc=1;
    if(result==K_LOAD_NOT_FOUND)fprintf(stderr,"%s: error: cannot read source [module-not-found]\n",path);
    if(result==K_LOAD_OK&&module&&k_diag_count(&sink,K_ERROR)==0) {
        size_t need=k_dump_ast(module->ast,path,(keel_slice_char){0});
        char *dump=cgen_alloc(arena,need?need:1,1,1,false,"AST dump");
        if(dump){k_dump_ast(module->ast,path,(keel_slice_char){need,dump});
            rc=cgen_write_output(out,dump,need);}
        else rc=2;
    }
    cgen_loader_destroy(tool);return rc;
}
