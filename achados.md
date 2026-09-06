Li os quatro documentos. **A proposta tem um núcleo coerente — contêineres, arenas, cleanup léxico e C gerado inspecionável —, mas a especificação ainda não está consistente o suficiente para orientar implementações independentes.** Há decisões antigas e novas coexistindo, além de alguns problemas de semântica que exigem mais que revisão editorial.

A análise abaixo distingue contradições explícitas de lacunas de implementação. Também conferi regras no rascunho do C23 e testei pequenos trechos do C proposto com GCC 13.3. Não alterei arquivos.

**1. Crítico: a justificativa das arenas sobre vetores de bytes inverte a regra de aliasing do C**

A [spec, em `from_array`](/home/andre/code/cprojects/keel/keel-spec.md:1628), e o [rationale](/home/andre/code/cprojects/keel/keel-rationale.md:1539) afirmam que um vetor de `u8` pode servir de armazenamento para `Particle` porque tipos de caractere podem aliasar qualquer objeto.

Essa permissão funciona no outro sentido: pode-se acessar a representação de um `Particle` por caracteres. Ela não autoriza tratar um vetor declarado de caracteres como um objeto arbitrário:

```c
_Alignas(int) unsigned char memoria[sizeof(int)];
int *p = (int *)memoria;
*p = 42;
```

Alinhamento suficiente não resolve o problema de tipo efetivo. O texto do C23 distingue objetos com tipo declarado de armazenamento alocado sem tipo declarado. [WG14 N3096, §6.5](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf).

Isso atinge **`from_array` e `from_stack`, dois caminhos centrais do projeto**. O mesmo erro aparece na [reinterpretação do `#embed` como `f32 *`](/home/andre/code/cprojects/keel/keel-rationale.md:480).

É necessário decidir entre restringir o modelo, usar armazenamento tipado ou assumir explicitamente um contrato de extensão do compilador. O lowering atual não sustenta a promessa de C estritamente padrão.

**2. Crítico: o contrato de `restrict` é mais fraco que o exigido pelo C gerado**

A [promessa da linguagem](/home/andre/code/cprojects/keel/keel-spec.md:1320) só proíbe sobreposição entre contêineres `restrict`. Entretanto, o [backend](/home/andre/code/cprojects/keel/keel-c-backend.md:735) usa o ponteiro hoistado para indexação e mantém outros acessos passando pelo descritor original.

Isso permite, sobre um buffer inicializado:

```keel
restrict buffer i32 b = buffer.of(v);
b[0] = 1;
buffer.set(b, 0, 2);
```

Há apenas um contêiner `restrict`, portanto a promessa escrita não foi violada. Mas os acessos podem ocorrer pelo ponteiro restrito e por outro caminho independente, derivado de `b.ptr`, violando o contrato do C.

**Capacidade fixa prova estabilidade da base; não prova exclusividade dos acessos.** É preciso uniformizar os caminhos de acesso ou fortalecer explicitamente o contrato.

**3. Crítico: `constexpr` por macro não preserva a semântica anunciada**

O [backend C11](/home/andre/code/cprojects/keel/keel-c-backend.md:1556) reconhece apenas a perda do diagnóstico de representabilidade. As diferenças são maiores:

- `&K` deixa de funcionar: a macro produz uma expressão, não um objeto.
- Uma macro local `N` também expande em `obj.N`.
- `#undef N` ao sair de um bloco não restaura uma definição externa sombreada.
- A conferência por `sizeof((T){INIT})` não exige que `INIT` seja constante.

Este último caso foi aceito pelo GCC:

```c
#define K ((int)f())
_Static_assert(sizeof((int){f()}) > 0, "K");
```

Usar `K + K` pode chamar `f()` duas vezes.

Isso contradiz a [equivalência entre perfis](/home/andre/code/cprojects/keel/keel-spec.md:3145). É preciso especificar quais operações `constexpr` admite e desenhar uma representação que as preserve; a macro atual não é uma substituição equivalente.

**4. Crítico: `tensor` e `view` exigem mecanismos que os genéricos proíbem**

A [regra de `dim`](/home/andre/code/cprojects/keel/keel-spec.md:2503) proíbe gerar declarações. Porém, a [stdlib multidimensional](/home/andre/code/cprojects/keel/keel-spec.md:2674) exige acessores de todas as aridades parciais até `N−1`.

Há três problemas distintos:

- Como declarar essa família para rank arbitrário sem geração de declarações?
- Como escrever `view(N-1) T` se o argumento precisa tornar-se um literal decimal, sem avaliação de expressão?
- Onde vive o descritor devolvido por `tensor.ptr(t,i)`, cujo retorno é `view(N-k) T *ref`?

O layout do tensor não contém esses descritores. Retornar endereço de variável local não serve; armazenamento estático compromete reentrância; armazenamento adicional ou temporário no chamador precisa ser especificado.

A indexação parcial necessita de uma decisão de representação e tempo de vida antes de poder ser considerada fechada.

**5. Alto: `parallel` promete determinismo que a interrupção impede**

A [spec](/home/andre/code/cprojects/keel/keel-spec.md:2080) afirma que `ok` e `failed` são determinísticos e que as escritas do corpo não dependem do escalonamento.

Considere `ANY`, com duas faixas:

```keel
interrupted;
if (w == 0) win;
fail;
```

Se a faixa 0 vencer antes do teste da faixa 1, esta pode terminar interrompida. Se a faixa 1 passar pelo teste antes disso, executa `fail`. **`failed(nome)` muda com o escalonamento**, sem corrida nos dados do usuário.

Da mesma forma, a interrupção muda quais elementos chegam a ser processados.

Execução serial pode ser uma execução permitida pelo contrato; isso não significa produzir o mesmo resultado que toda execução paralela. A promessa precisa ser formulada nesses termos.

**6. Alto: o lowering de `parallel` contradiz o fluxo permitido**

A [linguagem](/home/andre/code/cprojects/keel/keel-spec.md:2096) preserva `return` dentro do corpo. O [backend](/home/andre/code/cprojects/keel/keel-c-backend.md:1127) afirma que isso é erro 118 — mas esse número significa `else-multiplos-declaradores` na tabela atual.

O GCC rejeitou o retorno ao compilar a região OpenMP. Isso corresponde à proibição de sair do bloco estruturado por um salto. [OpenMP, restrições de blocos estruturados](https://www.openmp.org/spec-html/5.1/openmpse9.html).

Há ainda um erro na composição dos trechos de emissão: o [rótulo comum de término](/home/andre/code/cprojects/keel/keel-c-backend.md:1115) grava `SUCCESS`, enquanto `fail` e `interrupted` gravam seus status e saltam para esse rótulo. Composto literalmente, o código apaga falhas e interrupções.

São necessárias uma regra de linguagem para saídas e uma expansão completa, compilável, de cada política.

**7. Alto: `defer` não fecha sobre todos os caminhos de entrada no escopo**

A [spec](/home/andre/code/cprojects/keel/keel-spec.md:1941) promete registro seguido de execução na saída. O backend resolve isso por posição textual, sem estado de execução.

Mas `case` também pode pular o registro:

```keel
switch (op) {
case 0:
    defer cleanup();
case 1:
    break;
}
```

Quando `op == 1`, o `defer` não foi alcançado. A varredura textual encontra um cleanup anterior ao `break`. A regra de `goto` não cobre esse salto, e `defer` em `switch` recebe apenas warning.

Também falta preservar a ligação lexical sob sombreamento:

```keel
int x = 1;
defer usa(x);
{
    int x = 2;
    return;
}
```

Colar `usa(x)` no retorno chama com o `x` interno. O texto precisa definir como preserva o símbolo original, ou quais formas recusa.

**8. Alto: `corot` simultaneamente declara e não declara `failed`**

É uma contradição direta:

- [§4.8](/home/andre/code/cprojects/keel/keel-spec.md:2339): existe `corot.failed(r)`.
- [§4.10](/home/andre/code/cprojects/keel/keel-spec.md:2748): `corot` não declara `failed`, por isso fica fora de `else`.
- [Backend §5.14](/home/andre/code/cprojects/keel/keel-c-backend.md:1374): emite a função `keel_corot_i32_failed`.

Pelo protocolo publicado, `corot` é falível e aceita a cláusula. Para excluí-lo, é necessário renomear o predicado ou acrescentar uma distinção ao protocolo. A exclusão não decorre das regras atuais.

**9. Alto: há duas regras incompatíveis de instanciação**

A [spec](/home/andre/code/cprojects/keel/keel-spec.md:2434) diz que toda declaração pertence à instância, mencione `T` ou não. O [backend](/home/andre/code/cprojects/keel/keel-c-backend.md:505) diz que declarações sem menção ao parâmetro são emitidas uma vez no módulo.

O rationale defende as duas decisões em [seções consecutivas](/home/andre/code/cprojects/keel/keel-rationale.md:2636).

Além disso, a solução de buscar apenas o token `T` não basta:

```keel
pub inline size_t length(stack *s) {
    return s->len;
}
```

Essa função não menciona `T`, mas depende da instância de `stack`.

É preciso definir dependência por nomes de modificadores e tipos associados, ou limitar a exceção a uma categoria explícita, como constantes independentes.

**10. Alto: o despacho não explica chamadas centrais da própria base**

A [regra geral](/home/andre/code/cprojects/keel/keel-spec.md:2798) só examina o primeiro argumento. Entretanto:

```keel
buffer.clone(a, s)
slice.clone(a, s)
```

O primeiro argumento é uma arena. A instância depende de `s`, e o tipo produzido depende do qualificador.

Outras exceções também escapam ao inventário:

- `outcome.win/fail/none` usam o contexto de destino, incluindo retorno.
- `tensor.alloc` precisa obter tipo e rank.
- `string.of` distingue literal de ponteiro pelo token.

Apesar disso, [§4.11](/home/andre/code/cprojects/keel/keel-spec.md:2830) afirma que somente dois verbos não recebem contêiner como primeiro argumento e que `buffer.from` é o único caso decidido pelo alvo.

Falta uma regra completa para essas operações. Isso também enfraquece a afirmação de que a base inteira é expressável pelo protocolo comum.

**11. Alto: o prelúdio contradiz o backend e dispara uma proibição da própria linguagem**

A [spec](/home/andre/code/cprojects/keel/keel-spec.md:182) injeta toda a base. O [backend](/home/andre/code/cprojects/keel/keel-c-backend.md:387) e o [rationale](/home/andre/code/cprojects/keel/keel-rationale.md:501) afirmam que apenas a camada zero é implícita.

Há uma contradição ainda mais imediata:

```keel
import keel.buffer as buffer types;
```

Essa linha cria alias de módulo `buffer` e injeta o modificador `buffer`. A [regra 86](/home/andre/code/cprojects/keel/keel-spec.md:943) proíbe alias e nome de tipo com a mesma grafia, sem ressalva.

O padrão usado pelo próprio prelúdio precisa ser expressamente permitido.

**12. Alto: a invalidação incremental ignora entradas que alteram a tradução**

O [backend](/home/andre/code/cprojects/keel/keel-c-backend.md:1444) afirma que o `.c` de um módulo depende apenas de seu próprio fonte. Isso é falso quando a tradução consulta interfaces importadas.

Exemplo: `A` chama `b.consume(s)`. Se `B` mudar o parâmetro de `slice i32` para `slice i32 *`, a regra de adaptação de argumentos passa a exigir `&s` no C de `A`, mesmo sem edição em `A.k`.

O depfile transitivo pode reexecutar cgen, mas o [teste interno de timestamp](/home/andre/code/cprojects/keel/cgen-tool-spec.md:280) ainda permite pular `A`, mantendo o C antigo.

A correção precisa incluir as dependências relevantes na invalidação. A limitação documentada sobre troca de flags não cobre esse caso.

**13. Alto: a checagem global de colisões não cabe na ferramenta especificada**

A [linguagem](/home/andre/code/cprojects/keel/keel-spec.md:1017) exige verificar símbolos de todos os módulos do build antes de gerar qualquer coisa.

A [ferramenta](/home/andre/code/cprojects/keel/cgen-tool-spec.md:145) afirma que nunca vê o programa inteiro e que o link é repassado transparentemente.

Dois módulos compilados separadamente, sem import entre eles, podem produzir `net_http_get`. Nenhuma invocação conhece os dois conjuntos de símbolos.

É necessário limitar a garantia ao grafo conhecido, tornar o mangling não ambíguo ou introduzir uma etapa global. As três opções representam contratos diferentes.

**14. Alto: a EBNF não descreve várias formas declaradas válidas**

Não são apenas exemplos informais:

| Forma | Divergência |
|---|---|
| `module mat dim 2 type T;` | [§4.9 aceita](/home/andre/code/cprojects/keel/keel-spec.md:2409); `binder-dim` só aceita `IDENT`; o rationale diz que a forma foi removida. |
| Modificador com dois tipos | O módulo aceita `type K,V`, mas [a produção de uso](/home/andre/code/cprojects/keel/keel-spec.md:608) contém apenas um argumento. |
| `buffer _Atomic u32` e `buffer char const` | O backend aceita; `argumento` só prevê `const` antes do tipo. |
| `i32 *ref p` | O texto chama `ref` de `qual-c`, mas ele não aparece nessa produção. |
| `apply(T,c,fn,contexto)` | O backend e o rationale aceitam; [a EBNF](/home/andre/code/cprojects/keel/keel-spec.md:683) não. |
| Política com `constexpr` | A prosa aceita; `politica` só admite `ALL`, `ANY` ou `NUM`. |
| `alignas(...) array ...` | O backend depende de `prefixo-c`, produção ausente na gramática atual. |

A prioridade aqui é escolher uma versão normativa e derivar exemplos e gramática dela.

**Outras inconsistências concretas**

| Problema | Evidência e consequência |
|---|---|
| `buffer.of(v)` cheio ou vazio? | A [spec](/home/andre/code/cprojects/keel/keel-spec.md:1730) promete `len == cap`, mas chama `_as`; o [backend](/home/andre/code/cprojects/keel/keel-c-backend.md:646) define `_as` com `len = 0`. |
| Dois retornos incompatíveis para `at` | [Backend §5.4](/home/andre/code/cprojects/keel/keel-c-backend.md:905): `i32 *`; [§5.13](/home/andre/code/cprojects/keel/keel-c-backend.md:1327): `outcome i32`. |
| Falha da stdlib não segue o protocolo | [`string.clone`](/home/andre/code/cprojects/keel/keel-spec.md:2625) e [`tensor.alloc/clone`](/home/andre/code/cprojects/keel/keel-spec.md:2696) retornam descritores diretamente, contrariando a regra de falhar por `outcome`. |
| `alignof(objeto)` não é C padrão | O [backend](/home/andre/code/cprojects/keel/keel-c-backend.md:875) usa essa forma; a gramática padrão exige nome de tipo. GCC rejeitou com `-pedantic-errors`. [C23, §6.5.3](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf). |
| Usar `f16` exige também `bf16` | O [header compartilhado](/home/andre/code/cprojects/keel/keel-c-backend.md:292) verifica e declara ambos quando qualquer um é usado. |
| Tamanho não prova formato IEEE | Os [asserts do prelúdio](/home/andre/code/cprojects/keel/keel-c-backend.md:381) só verificam `sizeof`, embora prometam provar binary32/binary64. |
| Flags próprias de warning quebram GCC | A [ferramenta](/home/andre/code/cprojects/keel/cgen-tool-spec.md:361) repassa `-W<nome>` de keel. Testei `-Wopenmp-indisponivel`: GCC rejeita a opção. |
| A numeração não é única | A [tabela do backend](/home/andre/code/cprojects/keel/keel-c-backend.md:1471) diverge da tabela da linguagem, apesar de afirmar que a reproduz. |
| O modo pedântico não cobre seu próprio objetivo | O [backend](/home/andre/code/cprojects/keel/keel-c-backend.md:199) registra 31 caracteres externos garantidos, mas `--pedantic-names` limita a 63. |

Minha crítica de projeto é que **o documento frequentemente apresenta uma preferência como consequência inevitável da invariante**. “Não tipar C arbitrário” é uma fronteira útil; “não entender C” já não descreve precisamente um tradutor que reconhece declarações, campos, assinaturas, escopos e procedência. Esse conhecimento parcial pode ser perfeitamente aceitável, mas precisa ter limites operacionais claros.

Também não procede a afirmação de que um parser completo necessariamente tornaria `a && f()?` uma avaliação incondicional: isso é consequência de um lowering específico. Recusar o recurso pelo custo e pelo escopo seria uma justificativa suficiente, sem uma alegação de impossibilidade.

Eu priorizaria a correção nesta ordem: **modelo de memória e `restrict`; equivalência de `constexpr`; representação multidimensional; fluxo de `parallel` e `defer`; despacho e instanciação; invalidação do build**. Depois, usaria um conjunto pequeno de exemplos normativos completos, compilados nos dois perfis, para impedir que gramática, prosa e backend voltem a descrever versões diferentes da linguagem.
