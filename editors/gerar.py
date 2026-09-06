#!/usr/bin/env python3
"""Gera os realces de sintaxe a partir de keel-spec.md.

A lista de palavras é a do §3.7, lida do documento — não digitada aqui. Se a
spec ganhar ou perder uma palavra, rodar de novo é o que mantém os editores em
dia. Uso:  python3 editors/gerar.py
"""
import json, pathlib, re, sys

RAIZ = pathlib.Path(__file__).resolve().parent.parent
spec = (RAIZ / "keel-spec.md").read_text(encoding="utf-8")

def tabela(secao, fim):
    corpo = spec.split(secao, 1)[1].split(fim, 1)[0]
    return {g: re.findall(r'`([A-Za-z_][A-Za-z0-9_]*)`', p)
            for g, p in re.findall(r'^\| \*\*(.+?)\*\* \| (.+?) \|$', corpo, re.M)}

g = tabela("### 3.7 Palavras do núcleo", "### 3.8")
PALAVRAS = {
    "unidade":     g["Unidade"],
    "tipo":        g["Tipo composto"],
    "fluxo":       g["Fluxo"],
    "cooperativo": g["Cooperativo"],
}
# `else` é palavra do C: quem realça é a gramática do C, não nós.
PRIMITIVOS = ["i8","i16","i32","i64","u8","u16","u32","u64",
              "f16","bf16","f32","f64","size_t","ptrdiff_t","uintptr_t"]
# nomes injetados por `import … types`: não são palavras, mas lêem-se como tipo
BASE = ["arena","buffer","slice","range","corot","outcome","string","strbuf"]

def alt(xs): return "|".join(sorted(xs, key=lambda w: (-len(w), w)))
todas = sum(PALAVRAS.values(), [])

# ---------------------------------------------------------------- VSCode
tm = {
  "$schema": "https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json",
  "name": "keel", "scopeName": "source.keel", "fileTypes": ["k"],
  "patterns": [{"include": "#keel"}, {"include": "source.c"}],
  "repository": {"keel": {"patterns": [
    {"name": "keyword.control.keel",        "match": r"\b(%s)\b" % alt(PALAVRAS["fluxo"] + PALAVRAS["cooperativo"])},
    {"name": "keyword.other.keel",          "match": r"\b(%s)\b" % alt(PALAVRAS["unidade"] + PALAVRAS["tipo"])},
    {"name": "support.type.primitive.keel", "match": r"\b(%s)\b" % alt(PRIMITIVOS)},
    {"name": "support.type.base.keel",      "match": r"\b(%s)\b" % alt(BASE)},
    {"name": "keyword.operator.range.keel", "match": r"\.\."},
  ]}}
}
vs = RAIZ / "editors/vscode"
(vs / "syntaxes").mkdir(parents=True, exist_ok=True)
(vs / "syntaxes/keel.tmLanguage.json").write_text(json.dumps(tm, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

# ---------------------------------------------------------------- Zed
zd = RAIZ / "editors/zed/languages/keel"
zd.mkdir(parents=True, exist_ok=True)
(zd / "highlights.scm").write_text(f"""; keel — realce sobre a gramática do C (tree-sitter-c).
; GERADO por editors/gerar.py a partir de keel-spec.md §3.7. Não editar à mão.
;
; O parser é o do C, então as construções de keel caem em nós de erro ou viram
; identificadores. As regras abaixo casam por GRAFIA, que é o que funciona nos
; dois casos — e é a mesma disciplina do §3.7: palavra vale pela posição, e aqui
; se realça pela escrita.

((identifier) @keyword
 (#match? @keyword "^({alt(todas)})$"))

((type_identifier) @keyword
 (#match? @keyword "^({alt(todas)})$"))

((identifier) @type.builtin
 (#match? @type.builtin "^({alt(PRIMITIVOS)})$"))

((type_identifier) @type.builtin
 (#match? @type.builtin "^({alt(PRIMITIVOS)})$"))

((identifier) @type
 (#match? @type "^({alt(BASE)})$"))

((type_identifier) @type
 (#match? @type "^({alt(BASE)})$"))
""", encoding="utf-8")

print("palavras do §3.7: %d" % len(todas))
for k, v in PALAVRAS.items():
    print("  %-12s %s" % (k, " ".join(v)))
print("gerado: editors/vscode/syntaxes/keel.tmLanguage.json")
print("gerado: editors/zed/languages/keel/highlights.scm")
