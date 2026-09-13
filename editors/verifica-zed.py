#!/usr/bin/env python3
"""Verifica as consultas do Zed com o parser real de C.

Dependências de teste: tree-sitter==0.25.2 e tree-sitter-c==0.24.2.
Uso: python3 editors/verifica-zed.py
"""
from pathlib import Path

from tree_sitter import Language, Parser, Query, QueryCursor
import tree_sitter_c

ROOT = Path(__file__).resolve().parent.parent
language = Language(tree_sitter_c.language())
parser = Parser(language)
query = Query(language, (ROOT / "editors/zed/languages/keel/highlights.scm").read_text())


def captures(source):
    tree = parser.parse(source)
    result = QueryCursor(query).captures(tree.root_node)
    return tree, result


def require(source, result, text, scope):
    expected = text.encode()
    assert any(source[n.start_byte:n.end_byte] == expected for n in result.get(scope, [])), (
        f"{text!r} sem captura {scope}"
    )


# Regressão do arquivo mostrado no editor: C e keel no mesmo módulo,
# mesmo quando as declarações modificadas produzem ERROR no parser de C.
source = (ROOT / "golden/casos/006-dim/caso.k").read_bytes()
_, result = captures(source)
for text, scope in [
    ("module", "keyword"), ("import", "keyword"), ("pub", "keyword"),
    ("array", "keyword"), ("constexpr", "keyword"),
    ("i8", "type.builtin"), ("f32", "type.builtin"),
    ("if", "keyword"), ("return", "keyword"),
    ("int", "type.builtin"), ("char", "type.builtin"),
    ("main", "function"), ("puts", "function"),
    ("1.5f", "number"), ('"ok"', "string"),
    ("dims", "property"), ("!=", "operator"),
]:
    require(source, result, text, scope)
assert result.get("comment"), "comentários do caso 006 sem realce"

# Cobertura de C fora das construções keel e exclusão de literais/comentários.
source = br'''
#include <stdio.h>
#define LIMITE 3
typedef struct Item { int valor; } Item;
static int soma(Item *p) {
    // module buffer defer i32
    const char *s = "module buffer defer i32\n";
    char c = 'x';
    for (int i = 0; i < LIMITE; ++i) {
        if (p->valor != 0) return p->valor + c;
    }
    return 0;
}
'''
tree, result = captures(source)
assert not tree.root_node.has_error, "amostra C inválida"
for text, scope in [
    ("#include", "preproc"), ("#define", "preproc"),
    ("static", "keyword"), ("for", "keyword"), ("const", "keyword"),
    ("Item", "type"), ("int", "type.builtin"), ("soma", "function"),
    ("valor", "property"), ("0", "number"), ("'x'", "string"),
    (r"\n", "string.escape"), ("++", "operator"),
]:
    require(source, result, text, scope)
protected = result["comment"] + result["string"]
for scope in ("keyword", "type", "type.builtin", "function", "number"):
    for node in result.get(scope, []):
        assert not any(p.start_byte <= node.start_byte < p.end_byte for p in protected), (
            f"captura {scope} dentro de comentário ou literal"
        )

# Todos os arquivos golden passam pela consulta sem exigir que keel seja C válido.
files = sorted((ROOT / "golden/casos").glob("**/*.k"))
for path in files:
    captures(path.read_bytes())
print(f"Zed: C + keel no caso 006, literais/comentários protegidos; {len(files)} arquivos consultados.")
