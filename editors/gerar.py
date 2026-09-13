#!/usr/bin/env python3
"""Gera os realces de sintaxe a partir de keel-spec.md.

A lista de palavras é a do §2.2, lida do documento — não digitada aqui. Se a
spec ganhar ou perder uma palavra, rodar de novo é o que mantém os editores em
dia. Uso:  python3 editors/gerar.py
"""
import json, pathlib, re, sys

RAIZ = pathlib.Path(__file__).resolve().parent.parent
spec = (RAIZ / "keel-spec.md").read_text(encoding="utf-8")

def tabela(secao, fim):
    corpo = spec.split(secao, 1)[1].split(fim, 1)[0]
    return {g: re.findall(r'`([A-Za-z_][A-Za-z0-9_]*)`', p)
            for g, p in re.findall(r'^\| ([^|]+?) \| (.+?) \|$', corpo, re.M)}

g = tabela("#### Vocabulário do núcleo", "#### Palavras C reconhecidas")
PALAVRAS = {
    "unidade":     g["Unidade e visibilidade"],
    "tipo":        g["Tipos, marcadores e genéricos"],
    "fluxo":       g["Fluxo"],
    "etiquetado":  g["Valores etiquetados"],
}
# `else` é palavra do C: quem realça é a gramática do C, não nós.
PRIMITIVOS = ["i8","i16","i32","i64","u8","u16","u32","u64",
              "f16","bf16","f32","f64","size_t","ptrdiff_t","uintptr_t"]
# nomes injetados por `import … types`: não são palavras, mas lêem-se como tipo.
# `parallel` fica de fora: é palavra do núcleo, e já está no grupo Fluxo — o §2.2
# registra que ele é o único nome nas duas posições.
BASE = ["arena","buffer","slice","range","tagged","outcome","corot","routine"]
# Ficam de fora `cursor`, `slot` e `control`: `types` também os injeta, mas são
# palavras comuns em C, e realce por grafia é heurística — o falso positivo num
# identificador do usuário custa mais do que o realce que se ganharia.

def alt(xs): return "|".join(sorted(xs, key=lambda w: (-len(w), w)))
todas = sum(PALAVRAS.values(), [])

# ---------------------------------------------------------------- VSCode
#
# São DOIS arquivos, e a razão é medida: com um só, `"include": "source.c"`
# não alcança o interior das funções — a regra de bloco do C assume e só os
# padrões dela valem ali. `defer` saía como `meta.block.c` e `foreach` como
# `entity.name.function.c`, que é onde mora quase toda palavra de keel.
# A gramática base dá o C; a INJEÇÃO põe as palavras em todo escopo aninhado.
padroes = [
    {"name": "keyword.control.keel",        "match": r"\b(%s)\b" % alt(PALAVRAS["fluxo"] + PALAVRAS["etiquetado"])},
    {"name": "keyword.other.keel",          "match": r"\b(%s)\b" % alt(PALAVRAS["unidade"] + PALAVRAS["tipo"])},
    {"name": "support.type.primitive.keel", "match": r"\b(%s)\b" % alt(PRIMITIVOS)},
    {"name": "support.type.base.keel",      "match": r"\b(%s)\b" % alt(BASE)},
    # `0..4`: a regra numérica do C casa a partir do `0` e engoliria o `..`
    # como número malformado. Casando o operando esquerdo junto, a injeção
    # vence por posição — medido, e sem isso o intervalo não realça.
    {"match": r"(?<![\w.])(\d+)(\.\.)",
     "captures": {"1": {"name": "constant.numeric.decimal.c"},
                  "2": {"name": "keyword.operator.range.keel"}}},
    {"name": "keyword.operator.range.keel", "match": r"(?<!\.)\.\.(?!\.)"},
]
ESQ = "https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json"
base = {"$schema": ESQ, "name": "keel", "scopeName": "source.keel",
        "fileTypes": ["k"], "patterns": [{"include": "source.c"}]}
inj  = {"$schema": ESQ, "name": "keel (injeção)", "scopeName": "keel.injection",
        # L: dá precedência sobre as regras do C já existentes no escopo
        # `-comment -string`: sem isso, `defer` num comentário e `buffer` numa
        # string saíam realçados — medido.
        "injectionSelector": "L:source.keel -comment -string", "patterns": padroes}

vs = RAIZ / "editors/vscode"
(vs / "syntaxes").mkdir(parents=True, exist_ok=True)
(vs / "syntaxes/keel.tmLanguage.json").write_text(json.dumps(base, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
(vs / "syntaxes/keel-injection.tmLanguage.json").write_text(json.dumps(inj, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

# ---------------------------------------------------------------- Zed
zd = RAIZ / "editors/zed/languages/keel"
zd.mkdir(parents=True, exist_ok=True)
c_highlights = (RAIZ / "editors/c-highlights.scm").read_text(encoding="utf-8")
c_highlights = c_highlights.replace("__KEEL_WORDS__", alt(todas + PRIMITIVOS + BASE))
(zd / "highlights.scm").write_text(f"""; keel — realce sobre a gramática do C (tree-sitter-c).
; GERADO por editors/gerar.py: editors/c-highlights.scm + keel-spec.md §2.2.
; Não editar à mão.
;
; O parser é o do C, então as construções de keel caem em nós de erro ou viram
; identificadores. As regras abaixo casam por GRAFIA, que é o que funciona nos
; dois casos — e é a mesma disciplina do §2.2: palavra vale pela posição, e aqui
; se realça pela escrita.

{c_highlights}

; Vocabulário de keel, acrescentado às regras de C.

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

# ------------------------------------------------------- conferência
# Toda palavra lida da spec tem de aparecer nos dois artefatos. É o que pega
# uma seção renomeada: a tabela deixa de ser lida e o realce esvazia calado.
falta = []
vs_txt = (vs / "syntaxes/keel-injection.tmLanguage.json").read_text(encoding="utf-8")
zd_txt = (zd / "highlights.scm").read_text(encoding="utf-8")
for w in todas + PRIMITIVOS + BASE:
    if not re.search(r"\b%s\b" % re.escape(w), vs_txt): falta.append(("vscode", w))
    if not re.search(r"\b%s\b" % re.escape(w), zd_txt): falta.append(("zed", w))
if falta:
    for alvo, w in falta: print("FALTA %-7s %s" % (alvo, w), file=sys.stderr)
    sys.exit(1)

print("palavras do §2.2: %d" % len(todas))
for k, v in PALAVRAS.items():
    print("  %-12s %s" % (k, " ".join(v)))
print("gerado: editors/vscode/syntaxes/keel.tmLanguage.json")
print("gerado: editors/zed/languages/keel/highlights.scm")
