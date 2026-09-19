#!/usr/bin/env python3
"""Checks the Zed queries against the real C parser.

Test dependencies: tree-sitter==0.25.2 and tree-sitter-c==0.24.2.
Usage: python3 editors/verify-zed.py
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
        f"{text!r} without capture {scope}"
    )


# Regression on a file shown in the editor: C and keel in the same module,
# even when the modified declarations produce ERROR in the C parser.
source = (ROOT / "golden/casos/006-dim/app/g.k").read_bytes()
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
assert result.get("comment"), "case 006 comments not highlighted"

# C coverage outside keel constructs, and exclusion of literals/comments.
source = br'''
#include <stdio.h>
#define LIMIT 3
typedef struct Item { int value; } Item;
static int sum(Item *p) {
    // module buffer defer i32
    const char *s = "module buffer defer i32\n";
    char c = 'x';
    for (int i = 0; i < LIMIT; ++i) {
        if (p->value != 0) return p->value + c;
    }
    return 0;
}
'''
tree, result = captures(source)
assert not tree.root_node.has_error, "invalid C sample"
for text, scope in [
    ("#include", "preproc"), ("#define", "preproc"),
    ("static", "keyword"), ("for", "keyword"), ("const", "keyword"),
    ("Item", "type"), ("int", "type.builtin"), ("sum", "function"),
    ("value", "property"), ("0", "number"), ("'x'", "string"),
    (r"\n", "string.escape"), ("++", "operator"),
]:
    require(source, result, text, scope)
protected = result["comment"] + result["string"]
for scope in ("keyword", "type", "type.builtin", "function", "number"):
    for node in result.get(scope, []):
        assert not any(p.start_byte <= node.start_byte < p.end_byte for p in protected), (
            f"{scope} capture inside a comment or literal"
        )

# Every golden file goes through the query without requiring keel to be valid C.
files = sorted((ROOT / "golden/casos").glob("**/*.k"))
for path in files:
    captures(path.read_bytes())
print(f"Zed: C + keel in case 006, literals/comments protected; {len(files)} files queried.")
