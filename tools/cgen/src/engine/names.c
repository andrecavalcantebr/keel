#include "engine/names.h"
#include <stdint.h>

size_t k_name_normalize(keel_slice_char source, keel_slice_char out) {
    KLexer lexer; k_lexer_init(&lexer,source,NULL); TKPpKind pp;
    bool identifier=true; size_t size=0;
    for(;;) {
        KToken token=k_lexer_next(&lexer,&pp);
        if(!token.len)break;
        if(identifier ? !k_token_is_ident(token) : !k_token_is_punct(token,"."))return SIZE_MAX;
        if(token.len>out.len-size)return SIZE_MAX;
        memcpy(out.ptr+size,token.ptr,token.len);size+=token.len;identifier=!identifier;
    }
    return identifier?SIZE_MAX:size;
}


const char *k_primitive_name(keel_slice_char name) {
    char compact[4096];
    size_t n=k_name_normalize(name,(keel_slice_char){sizeof compact,compact});
    if(n!=SIZE_MAX)name=(keel_slice_char){n,compact};
    static const char *const names[]={"i8","u8","i16","u16","i32","u32","i64","u64","f16","bf16","f32","f64","char","bool","void","size_t","ptrdiff_t","uintptr_t"};
    static const char *const c_names[]={"int8_t","uint8_t","int16_t","uint16_t","int32_t","uint32_t","int64_t","uint64_t"};
    if(name.len>5&&memcmp(name.ptr,"keel.",5)==0){name.ptr+=5;name.len-=5;}
    for(size_t i=0;i<sizeof names/sizeof *names;i++)
        if(k_symtab_same_name(name,k_diag_text(names[i])))return names[i];
    for(size_t i=0;i<sizeof c_names/sizeof *c_names;i++)
        if(k_symtab_same_name(name,k_diag_text(c_names[i])))return names[i];
    return NULL;
}
bool k_c_arith_word(KToken t) {
    static const char *const words[]={"signed","unsigned","short","long","int","float","double"};
    for(size_t i=0;i<sizeof words/sizeof *words;i++)if(k_token_is_c_word_named(t,words[i]))return true;
    return false;
}
/* `unsigned char`, `long long`, ... as the keel type they stand for (spec §4.2, rule 3). `long`, alone or
   with `double`, is target-dependent and has none: NULL. */
const char *k_c_words_primitive(const KToken *w,size_t n) {
    int sg=0,un=0,sh=0,lg=0,in=0,ch=0,fl=0,db=0;
    for(size_t i=0;i<n;i++) {
        if(k_token_is_c_word_named(w[i],"signed"))sg++;
        else if(k_token_is_c_word_named(w[i],"unsigned"))un++;
        else if(k_token_is_c_word_named(w[i],"short"))sh++;
        else if(k_token_is_c_word_named(w[i],"long"))lg++;
        else if(k_token_is_c_word_named(w[i],"int"))in++;
        else if(k_token_is_c_word_named(w[i],"char"))ch++;
        else if(k_token_is_c_word_named(w[i],"float"))fl++;
        else if(k_token_is_c_word_named(w[i],"double"))db++;
        else return NULL;
    }
    if(n==1&&ch)return "char";
    if(n==1&&fl)return "f32";
    if(n==1&&db)return "f64";
    if(fl||db||sg+un>1||in>1)return NULL;
    if(ch)return n==2&&sg?"i8":n==2&&un?"u8":NULL;
    if(sh)return lg?NULL:un?"u16":"i16";
    if(lg)return lg==2?(un?"u64":"i64"):NULL;
    return un?"u32":"i32";
}
const KSymbol *k_symbol_resolve(const KSymbolTable *table,keel_slice_char name) {
    char compact[4096];
    size_t n=k_name_normalize(name,(keel_slice_char){sizeof compact,compact});
    if(n==SIZE_MAX)return NULL;
    name=(keel_slice_char){n,compact};
    const KSymbol *plain=k_symtab_lookup(table,name);
    if(plain&&plain->kind!=K_SYM_MODULE)return plain;
    for(size_t i=0;i<table->len;i++) {
        const KSymbol *q=keel_buffer_KSymbol_ptr(table, i);
        if(q->kind!=K_SYM_MODULE||!q->origin)continue;
        char prefix[4096];
        size_t plen=k_name_normalize(q->name,(keel_slice_char){sizeof prefix,prefix});
        if(plen==SIZE_MAX||name.len<=plen+1||memcmp(name.ptr,prefix,plen)||name.ptr[plen]!='.')continue;
        keel_slice_char tail={name.len-plen-1,name.ptr+plen+1};
        for(size_t j=0;j<q->origin->symbol_count;j++)
            if(k_symtab_same_name(q->origin->symbols[j].name,tail))return &q->origin->symbols[j];
    }
    return plain;
}
size_t k_mangle_symbol(const KSymbol *s,keel_slice_char out) {
    keel_slice_char module=s->origin?s->origin->name:(keel_slice_char){0};
    bool primitive=module.len==4&&!memcmp(module.ptr,"keel",4);
    size_t last=0;
    for(size_t i=0;i<module.len;i++)if(module.ptr[i]=='.')last=i+1;
    bool shorten=(s->kind==K_SYM_TYPE||s->kind==K_SYM_MODIFIER)&&k_symtab_same_name((keel_slice_char){module.len-last,module.ptr?module.ptr+last:NULL},s->name);
    size_t n=0;
    if(!primitive)for(size_t i=0;i<module.len;i++){if(n<out.len)out.ptr[n]=module.ptr[i]=='.'?'_':module.ptr[i];n++;}
    if(!shorten||!module.len||primitive){
        if(n){if(n<out.len)out.ptr[n]='_';n++;}
        for(size_t i=0;i<s->name.len;i++){if(n<out.len)out.ptr[n]=s->name.ptr[i]=='.'?'_':s->name.ptr[i];n++;}
    }
    return n;
}
