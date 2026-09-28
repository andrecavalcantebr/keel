#include <assert.h>
#include <stdio.h>
#include "engine/ast.h"
int main(void) {
    KSymbol symbols[1]; KSymbolTable table;
    k_symtab_init(&table,symbols,1);
    assert(k_symtab_insert(&table,(keel_slice_char){1,"x"},K_SYM_TYPE,0));
    assert(!k_symtab_insert(&table,(keel_slice_char){1,"y"},K_SYM_TYPE,0));
    assert(table.len==1 && table.cap==1);
    assert(keel_buffer_KSymbol_ptr(&table,0)==symbols);
    assert(k_symtab_lookup(&table,(keel_slice_char){1,"x"})==symbols);
    k_symtab_init(&table,NULL,8);
    assert(table.len==0 && table.cap==0);
    assert(!k_symtab_insert(&table,(keel_slice_char){1,"x"},K_SYM_TYPE,0));

    char text[]="module test; i32 x;";
    KLexeme tokens[16]; KAstNode nodes[2];
    size_t count=k_lexemes((keel_slice_char){sizeof text-1,text},tokens,16,NULL);
    KAst ast={.source={sizeof text-1,text},
        .tokens=keel_buffer_KLexeme_of(tokens,count),
        .nodes=keel_buffer_KAstNode_from(nodes,1)};
    assert(!k_parse_ast(&ast));
    assert(ast.nodes.len==1 && ast.nodes.cap==1);
    ast.nodes=keel_buffer_KAstNode_from(nodes,2);
    assert(k_parse_ast(&ast) && ast.nodes.len==2);
    assert(k_parse_ast(&ast) && ast.nodes.len==2); /* reset, not append */
    assert(keel_buffer_KAstNode_ptr(&ast.nodes,1)->kind==K_AST_VARIABLE);
    puts("ok");
}
