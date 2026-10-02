/* File ownership and the recursive load state belong to the tool. The
 * engine only sees the callback and retained module interfaces. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "tool/tool.h"
#include "tool/cli_limits.h"
#include "engine/instances.h"
#include "engine/islands.h"
#include "engine/names.h"
#include "keel/keel_buffer_char.type.h"

bool cgen_read_source(keel_arena *, const char *, keel_buffer_char *);
void cgen_report(const char *, keel_slice_char, const KDiagnosticSink *);

typedef enum { CGEN_PENDING, CGEN_DONE, CGEN_FAILED } CgenModState;
typedef struct CgenModuleEntry {
    struct CgenModuleEntry *next;
    string path;
    CgenModState state;
    KModule module;
    KAst ast;
    KSymbolTable symbols;
    KSymbol *exports;
} CgenModuleEntry;
typedef struct {
    keel_arena *arena;
    const char *roots[CGEN_SEARCH_ROOTS_CAP];
    int root_count;
    KDiagnosticSink *diag;
    CgenModuleEntry *mods;
    CgenModuleEntry *active;
} CgenTool;

static CgenModuleEntry *find(CgenTool *t, keel_slice_char name) {
    for (CgenModuleEntry *e = t->mods; e; e = e->next)
        if (k_symtab_same_name(e->module.name, name)) return e;
    return NULL;
}
static keel_slice_char span(const KAst *a, size_t first, size_t end) {
    if (first >= end) return (keel_slice_char){0};
    KToken x = keel_buffer_KLexeme_ptr(&a->tokens, first)->token, y = keel_buffer_KLexeme_ptr(&a->tokens, end-1)->token;
    return (keel_slice_char){(size_t)(y.ptr+y.len-x.ptr), x.ptr};
}
static int identifiers(const KAst *a, size_t first, size_t end) {
    int n = 0;
    for (size_t i=first; i<end; i++) if (k_token_is_ident(keel_buffer_KLexeme_ptr(&a->tokens, i)->token)) n++;
    return n;
}
static KLoadResult failure(CgenTool *t, CgenModuleEntry *e, KDiagId id,
                           size_t before) {
    e->state = CGEN_FAILED;
    if (k_diag_count(t->diag, K_ERROR) == before) {
        keel_slice_char at = e->ast.source.ptr ? e->ast.source : e->module.name;
        if (e->ast.error_token < e->ast.tokens.len)
            at = keel_buffer_KLexeme_ptr(&e->ast.tokens, e->ast.error_token)->token;
        k_diag_emit(t->diag,id,at,(KDiagArgs){{e->module.name}});
    }
    return K_LOAD_ERROR;
}
static bool exports(CgenTool *t, CgenModuleEntry *e) {
    KAst *a=&e->ast;
    KAstNode *mod=keel_buffer_KAstNode_ptr(&a->nodes, a->module);
    int dims=identifiers(a,mod->dim_first,mod->dim_end);
    int arity=dims+identifiers(a,mod->tags_first,mod->tags_end)+identifiers(a,mod->type_first,mod->type_end);
    e->exports=CGEN_NEW(t->arena,KSymbol,a->nodes.len+1);
    if (!e->exports) return false;
    for (size_t i=0;i<a->nodes.len;i++) {
        KAstNode *n=keel_buffer_KAstNode_ptr(&a->nodes, i);
        if (n->name_first==n->name_end) continue;
        KSymKind kind;
        switch(n->kind) {
        case K_AST_TYPE: kind=K_SYM_TYPE; break;
        case K_AST_MODIFIER: kind=K_SYM_MODIFIER; break;
        case K_AST_TAGS: kind=K_SYM_TAGS; break;
        case K_AST_FUNCTION: kind=K_SYM_FUNCTION; break;
        case K_AST_VARIABLE: kind=K_SYM_VARIABLE; break;
        case K_AST_CONSTEXPR: kind=K_SYM_CONSTANT; break;
        case K_AST_PROTOCOL: kind=K_SYM_PROTOCOL; break;
        default: continue;
        }
        KSymbol sym={.name=span(a,n->name_first,n->name_end),.kind=kind,
                     .arity=kind==K_SYM_MODIFIER?arity:0,.origin=&e->module,
                     .dim_arity=kind==K_SYM_MODIFIER?dims:0};
        if (kind==K_SYM_CONSTANT) {
            for(size_t j=n->name_end;j+1<n->end;j++)
                if(k_token_is_punct(keel_buffer_KLexeme_ptr(&a->tokens, j)->token,"=")) {
                    sym.value=span(a,j+1,n->end-1); break;
                }
        }
        /* Complete the retained local entry (or insert functions/objects
           found by the general declarator path). Imported identities stay. */
        bool found=false;
        for(size_t j=0;j<e->symbols.len;j++) {
            KSymbol *s=keel_buffer_KSymbol_ptr(&e->symbols, j);
            if(!s->origin && k_symtab_same_name(s->name,sym.name)) {*s=sym;found=true;break;}
        }
        if(!found) {
            if(e->symbols.len==e->symbols.cap)return false;
            (*keel_buffer_KSymbol_ptr(&e->symbols, e->symbols.len++))=sym;
        }
        if(n->is_public)e->exports[e->module.symbol_count++]=sym;
    }
    e->module.symbols=e->exports;
    return true;
}

static KLoadResult load_file(CgenTool *t, keel_slice_char name, const char *path,
                             long long mtime, KModule **out) {
    size_t before=k_diag_count(t->diag,K_ERROR);
    CgenModuleEntry *e=CGEN_NEW(t->arena,CgenModuleEntry,1);
    if(!e) {
        keel_slice_char at=k_diag_text("module entry");
        k_diag_emit(t->diag,K_DIAG_CAPACITY,at,(KDiagArgs){{at}});
        return K_LOAD_ERROR;
    }
    e->path=cgen_string_dup(t->arena,path,strlen(path));
    string saved=cgen_string_dup(t->arena,name.ptr,name.len);
    e->module.name=(keel_slice_char){saved.len,saved.ptr};
    e->state=CGEN_PENDING;e->next=t->mods;t->mods=e;
    if(!saved.ptr||!e->path.ptr)return failure(t,e,K_DIAG_CAPACITY,before);
    keel_buffer_char source;
    if(!cgen_read_source(t->arena,path,&source))return failure(t,e,K_DIAG_LOAD_FAILED,before);
    e->ast.source=(keel_slice_char){source.len,source.ptr};
    size_t count=k_lexemes(e->ast.source,NULL,0,NULL);
    if(count>SIZE_MAX/sizeof(KLexeme)-1 || count>SIZE_MAX/sizeof(KAstNode)-1)
        return failure(t,e,K_DIAG_CAPACITY,before);
    e->ast.tokens=keel_buffer_KLexeme_from(CGEN_NEW(t->arena,KLexeme,count+1),count+1);
    e->ast.nodes=keel_buffer_KAstNode_from(CGEN_NEW(t->arena,KAstNode,count+1),count+1);
    if(!e->ast.tokens.ptr||!e->ast.nodes.ptr)return failure(t,e,K_DIAG_CAPACITY,before);
    e->ast.tokens.len=count;
    k_lexemes(e->ast.source,e->ast.tokens.ptr,count,t->diag);
    if(k_diag_count(t->diag,K_ERROR)>before)return failure(t,e,K_DIAG_LOAD_FAILED,before);
    if(!k_parse_headers(&e->ast))return failure(t,e,K_DIAG_PARSE_FAILED,before);
    KAstNode *mod=keel_buffer_KAstNode_ptr(&e->ast.nodes, e->ast.module);
    keel_slice_char declared=span(&e->ast,mod->name_first,mod->name_end);
    char declared_name[4096];
    size_t declared_size=k_name_normalize(declared,(keel_slice_char){sizeof declared_name,declared_name});
    if(declared_size==SIZE_MAX||!k_symtab_same_name(name,(keel_slice_char){declared_size,declared_name})) {
        k_diag_emit(t->diag,K_DIAG_MODULE_PATH_MISMATCH,declared,(KDiagArgs){{e->module.name,declared}});
        return failure(t,e,K_DIAG_LOAD_FAILED,before);
    }
    /* Imports contribute at most their exports plus qualifier and alias.
       First load them through a counting-capacity table sized from the
       source graph's actual exports, by reserving through the callback. */
    size_t capacity=count+1;
    CgenModuleEntry *parent=t->active;
    t->active=e;e->module.closure_mtime=mtime;
    /* Source roots are finite; reserve symbols after loading interfaces.
       The header resolver is run once, using a growable tool-side bound:
       each imported interface is loaded here, then the engine registers it. */
    for(size_t i=0;i<e->ast.nodes.len;i++) {
        KAstNode *n=keel_buffer_KAstNode_ptr(&e->ast.nodes, i);
        if(n->kind!=K_AST_IMPORT)continue;
        KModule *m=NULL;
        KLoadResult x=cgen_load(t,span(&e->ast,n->name_first,n->name_end),&m);
        if(x==K_LOAD_OK||x==K_LOAD_ALREADY)capacity+=m->symbol_count+2;
    }
    if(!k_symtab_same_name(name,k_diag_text("keel"))) {
        KModule *m=NULL;KLoadResult x=cgen_load(t,k_diag_text("keel"),&m);
        if(x==K_LOAD_OK||x==K_LOAD_ALREADY)capacity+=m->symbol_count+1;
    }
    if(capacity>SIZE_MAX/sizeof(KSymbol)) {t->active=parent;return failure(t,e,K_DIAG_CAPACITY,before);}
    KSymbol *storage=CGEN_NEW(t->arena,KSymbol,capacity);
    k_symtab_init(&e->symbols,storage,capacity);
    KLoader loader={cgen_load,t};
    bool ok=storage && k_resolve_imports(&e->ast,&loader,&e->symbols,t->diag);
    t->active=parent;
    if(!ok||k_diag_count(t->diag,K_ERROR)>before)return failure(t,e,K_DIAG_LOAD_FAILED,before);
    /* Bind generic parameters before collecting declarations. */
    for(size_t i=mod->type_first;i<mod->type_end;i++)
        if(k_token_is_ident(keel_buffer_KLexeme_ptr(&e->ast.tokens, i)->token))
            if(!k_symtab_insert(&e->symbols,keel_buffer_KLexeme_ptr(&e->ast.tokens, i)->token,K_SYM_TYPE,0))return failure(t,e,K_DIAG_CAPACITY,before);
    if(!k_collect_ast(&e->ast,&e->symbols))return failure(t,e,K_DIAG_PARSE_FAILED,before);
    if(!exports(t,e))return failure(t,e,K_DIAG_CAPACITY,before);
    e->ast.instances=keel_buffer_KInstanceUse_from(CGEN_NEW(t->arena,KInstanceUse,count+1),count+1);
    if(!e->ast.instances.ptr||!k_collect_instances(&e->ast,t->diag))
        return failure(t,e,K_DIAG_CAPACITY,before);
    /* An island holds a few tokens' worth of text; the bound is generous, and
       running into it is a diagnostic, never a truncation. */
    size_t text=count*CGEN_ISLAND_TEXT_PER_TOKEN+CGEN_ISLAND_TEXT_BASE;
    e->ast.islands=keel_buffer_KIsland_from(CGEN_NEW(t->arena,KIsland,count+1),count+1);
    e->ast.island_text=(keel_buffer_char){.ptr=CGEN_NEW(t->arena,char,text),.len=0,.cap=text};
    if(!e->ast.islands.ptr||!e->ast.island_text.ptr||!k_collect_islands(&e->ast,t->diag))
        return failure(t,e,K_DIAG_CAPACITY,before);
    e->module.ast=&e->ast;e->state=CGEN_DONE;*out=&e->module;
    return K_LOAD_OK;
}
KLoadResult cgen_load(void *v, keel_slice_char name, KModule **out) {
    CgenTool *t=v;*out=NULL;
    char canonical[4096];
    size_t length=k_name_normalize(name,(keel_slice_char){sizeof canonical,canonical});
    if(length==SIZE_MAX){k_diag_emit(t->diag,K_DIAG_LOAD_FAILED,name,(KDiagArgs){{name}});return K_LOAD_ERROR;}
    name=(keel_slice_char){length,canonical};
    CgenModuleEntry *e=find(t,name);
    KLoadResult result;
    if(e) {
        if(e->state==CGEN_PENDING)return K_LOAD_CYCLE;
        if(e->state==CGEN_FAILED)return K_LOAD_ERROR;
        *out=&e->module;result=K_LOAD_ALREADY;
    } else {
        char path[4096];struct stat st;bool found=false;
        for(int i=0;i<t->root_count;i++)
            if(cgen_module_path(name.ptr,name.len,t->roots[i],path,sizeof path)&&
               stat(path,&st)==0&&S_ISREG(st.st_mode)){found=true;break;}
        if(!found)return K_LOAD_NOT_FOUND;
        result=load_file(t,name,path,(long long)st.st_mtime,out);
    }
    if(*out&&t->active&&(*out)->closure_mtime>t->active->module.closure_mtime)
        t->active->module.closure_mtime=(*out)->closure_mtime;
    return result;
}
KLoadResult cgen_load_path(void *v,const char *path,const char *name,KModule **out) {
    struct stat st;*out=NULL;
    if(stat(path,&st)!=0) return K_LOAD_NOT_FOUND;
    return load_file(v,k_diag_text(name),path,(long long)st.st_mtime,out);
}
void cgen_loader_init(void *v,keel_arena *arena,const char *const *roots,int n,KDiagnosticSink *diag,KLoader *out) {
    CgenTool *t=v;memset(t,0,sizeof *t);t->arena=arena;t->root_count=0;
    t->diag=diag;*out=(KLoader){cgen_load,t};
    if(n<0 || n>CGEN_SEARCH_ROOTS_CAP) {
        fprintf(stderr,"cgen: error: search roots exceed configured limit %d; adjust CGEN_CLI_ROOTS_CAP [implementation-limit]\n",CGEN_SEARCH_ROOTS_CAP);
        k_diag_emit(diag,K_DIAG_CAPACITY,k_diag_text(""),(KDiagArgs){{k_diag_text("search roots")}});
        return;
    }
    t->root_count=n;
    for(int i=0;i<t->root_count;i++)t->roots[i]=roots[i];
}
size_t cgen_loader_size(void){return sizeof(CgenTool);}
void cgen_loader_destroy(void *v) {
    CgenTool *t=v;
    /* Storage belongs to the invocation arena, including failed loads. */
    t->mods=NULL;
}
void cgen_loader_report(void *v,const KDiagnosticSink *sink) {
    CgenTool *t=v;
    for(size_t i=0;i<sink->len;i++) {
        KDiagnostic item=sink->items[i];CgenModuleEntry *owner=NULL;
        for(CgenModuleEntry *e=t->mods;e;e=e->next) {
            uintptr_t p=(uintptr_t)item.at.ptr,b=(uintptr_t)e->ast.source.ptr;
            if(b&&p>=b&&p<=b+e->ast.source.len){owner=e;break;}
        }
        KDiagnosticSink one={.items=&item,.len=1};
        if(owner)cgen_report(str_cstr(owner->path),owner->ast.source,&one);
        else {item.at=k_diag_text("");cgen_report("cgen",item.at,&one);}
    }
}
