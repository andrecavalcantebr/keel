# transform - pequena ferramenta de apoio ao parser

O parser é cgen (vide cgen-tool-spec.md)

Esta é uma pequena ferramenta de apoio ao parser, que transforma o código fonte em base ou outros arquivos em pseudo keel em .c/.h verdadeiro que é capaz de utilizado no cgen/parser de keel já com as técnicas e tipos keel.

A ideia é desenvolver o transpilador keel com a própria linguagem keel, sendo esta uma aplicação não trivial a validar a própria linguagem e seu stack de desenvolvimento. No entanto, precisamos de um bootstrap inicial. Para facilitar, alguns tipos básicos de keel já são "feitos à mão" diretamente em C e disponíveis para uso no transpilador.

## SPECS:

### 1. chamada:

```bash
transform -m qualified_module_name -i input_dir -d output_dir -t type -n dimension -e "[enum_list]"
```

### 2. variáveis globais reconhecidas:

### 2.1. o tipo: via opção -t
### 2.2. a dimensão: via opção -n
### 2.3. a lista de enums: via opção -e
 
### 3. geração do .h

### 3.1. o cabeçalho inicial é copiador verbatim
### 3.2. a linha module é detectada:

#### 3.2.1. o nome qualificado do módulo é reconhecido
#### 3.2.2. o nome do arquivo de saída é obtido a partir do nome qualificado do módulo e acrescentando .h
#### 3.2.3. o nome do arquivo de entrada é obtido a partir do nome qualificado do módulo e acrescentando .k
#### 3.2.4. a linha inteira é copiada verbatim para o .h dentro de um comentário //

### 3.3. é inserido o define guard

### 3.4. os imports são transformados em #include <module_name_to_path_name.h>

### 3.5. todas as substituições das marcas são substituídas:
#### 3.5.1. $T é substituído pelo tipo do módulo
#### 3.5.2. $N é substituído pela dimensão do módulo
#### 3.5.3. $E é substituído pela lista de enums

### 3.6. marcas de geração para .type.h, .impl.h e o .h:
#### 3.6.1. %type - leva o texto para .type.h
#### 3.6.2. %impl - leva o texto para .impl.h
#### 3.6.3. %h - leva o texto para o .h: é gerado pelo import  do .type.h + o que for de tipo próprio necessário
#### 3.6.4. $A - gera o conjunto de #includes

## Detalhes:

- Nome qualifficado de módulo para nome de arquivo do módulo: subistitui o '.' por '/'; o último nome é substituíido pelo nome do tipo
- Nome qualificado de módulo para nome do tipo: subistitui o '.' por '_'
- Nomes dos arquivos: caminho da pasta de entrada/saída + nome do arquivo do módulo + nome do tipo + extensão apropriada
- Cada marca é substituída no máximo uma vez; o texto produzido pela substituição não contém novas marcas semâ1nticas para o gerador.
