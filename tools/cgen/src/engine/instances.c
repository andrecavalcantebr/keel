#include "engine/instances.h"
#include "engine/parser.h"
#include "engine/names.h"

typedef struct {char *ptr;size_t len,cap;bool ok;} Name;
static void append(Name *n,keel_slice_char s) {
    if(s.len>n->cap-n->len){n->ok=false;return;}
    memcpy(n->ptr+n->len,s.ptr,s.len);n->len+=s.len;
}
static void text(Name *n,const char *s){append(n,k_diag_text(s));}
static int qualifier(KToken t) {
    if(k_token_spelled(t,"const"))return 1;
    if(k_token_spelled(t,"volatile"))return 2;
    if(k_token_spelled(t,"_Atomic"))return 4;
    return 0;
}
static bool type_name(keel_slice_char source,const KSymbolTable *symbols,Name *name,int depth);
static bool dimension(keel_slice_char s,const KSymbolTable *symbols,Name *name) {
    const KSymbol *sym=k_symbol_resolve(symbols,s);
    if(sym&&sym->kind==K_SYM_CONSTANT)s=sym->value;
    while(s.len&&s.ptr[0]==' '){s.ptr++;s.len--;}
    while(s.len&&s.ptr[s.len-1]==' ')s.len--;
    if(!s.len)return false;
    for(size_t i=0;i<s.len;i++)if(s.ptr[i]<'0'||s.ptr[i]>'9')return false;
    while(s.len>1&&s.ptr[0]=='0'){s.ptr++;s.len--;}
    append(name,s);return name->ok;
}
static bool type_name(keel_slice_char source,const KSymbolTable *symbols,Name *name,int depth) {
    if(depth>K_SPEC_MAX_DEPTH)return false;
    KLexer lexer;k_lexer_init(&lexer,source,NULL);TKPpKind pp;
    KToken first=k_lexer_next(&lexer,&pp);int quals=0;
    while(qualifier(first)){quals|=qualifier(first);first=k_lexer_next(&lexer,&pp);}
    bool tag=k_token_spelled(first,"struct")||k_token_spelled(first,"union")||k_token_spelled(first,"enum");
    if(tag)first=k_lexer_next(&lexer,&pp);
    KLexer look=lexer;KToken next;
    keel_slice_char qualified=k_scan_qualified_name(&look,first,&next,&pp);
    const KSymbol *sym=k_symbol_resolve(symbols,qualified);
    const char *primitive=k_primitive_name(qualified);
    KSpecifier spec={0};
    if(sym&&sym->kind==K_SYM_MODIFIER) {
        if(!k_scan_known_type(&lexer,first,symbols,&spec,&next,&pp))return false;
    } else lexer=look;
    while(qualifier(next)){quals|=qualifier(next);next=k_lexer_next(&lexer,&pp);}
    if(next.len)return false;
    if(quals&1)text(name,"const_");
    if(quals&2)text(name,"volatile_");
    if(quals&4)text(name,"atomic_");
    if(primitive)text(name,primitive);
    else {
        /* A generic binder has no origin yet: it is not a concrete use. */
        if(!sym||!sym->origin)return false;
        size_t need=k_mangle_symbol(sym,(keel_slice_char){name->cap-name->len,name->ptr+name->len});
        if(need>name->cap-name->len){name->ok=false;return false;}
        name->len+=need;
        if(sym->kind==K_SYM_MODIFIER) {
            if(spec.dim_count!=(size_t)sym->dim_arity)return false;
            for(size_t i=0;i<spec.dim_count;i++){
                text(name,"_");if(!dimension(spec.dims[i],symbols,name))return false;
            }
            for(size_t i=0;i<spec.arg_count;i++){
                text(name,"_");if(!type_name(spec.args[i],symbols,name,depth+1))return false;
            }
        }
    }
    return name->ok;
}
bool k_collect_instances(KAst *a,KInstanceUse *out,size_t cap,KDiagnosticSink *diag) {
    a->instances=out;a->instance_count=0;
    if(!a->symbols)return true;
    const KAstNode *module=&a->nodes[a->module];
    /* Generic definitions are templates; their concrete closure is not the
       direct-use pass, and cannot be named before substitution. */
    if(module->dim_first!=module->dim_end||module->tags_first!=module->tags_end||module->type_first!=module->type_end)return true;
    for(size_t i=0;i<a->token_count;i++) {
        if(a->tokens[i].directive||!k_token_is_ident(a->tokens[i].token))continue;
        bool opaque=false;
        for(size_t j=0;j<a->node_count;j++){
            KAstNode *n=&a->nodes[j];
            if((n->kind==K_AST_EXTERN_C||n->kind==K_AST_IMPORT||n->kind==K_AST_MODULE)&&i>=n->first&&i<n->end){opaque=true;break;}
        }
        if(opaque)continue;
        if(i&&k_token_is_punct(a->tokens[i-1].token,"."))continue;
        KToken first=a->tokens[i].token;
        keel_slice_char rest={(size_t)(a->source.ptr+a->source.len-first.ptr),first.ptr};
        KLexer lexer;k_lexer_init(&lexer,rest,NULL);TKPpKind pp;first=k_lexer_next(&lexer,&pp);
        KSpecifier spec;KToken next={0};
        if(!k_scan_known_type(&lexer,first,a->symbols,&spec,&next,&pp)||spec.kind!=K_SPEC_MODIFIER)continue;
        KInstanceUse use={.first=i};
        Name name={use.symbol,0,sizeof use.symbol,true};
        if(!type_name(spec.text,a->symbols,&name,0)) {
            if(!name.ok){k_diag_emit(diag,K_DIAG_CAPACITY,first,(KDiagArgs){{first}});return false;}
            continue;
        }
        use.symbol_len=name.len;
        use.end=i+1;
        while(use.end<a->token_count&&a->tokens[use.end].token.ptr<spec.text.ptr+spec.text.len)use.end++;
        bool exists=false;
        for(size_t j=0;j<a->instance_count;j++)
            if(out[j].symbol_len==use.symbol_len&&!memcmp(out[j].symbol,use.symbol,use.symbol_len)){exists=true;break;}
        if(exists)continue;
        if(a->instance_count==cap){k_diag_emit(diag,K_DIAG_CAPACITY,first,(KDiagArgs){{first}});return false;}
        out[a->instance_count++]=use;
    }
    return true;
}
