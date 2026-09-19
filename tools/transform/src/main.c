/*
 * DESC: simple code generation for keel files
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3
 */

/*
 * Gera o arquivo .h a partir de um arquivo .k de módulo
 * Faz apenas subtituição de strings
 *
 * SPECS:
 * 1. chamada:
 * transform -m qualified_module_name -i input_dir -d output_dir -t type -n dimension -e "[enum_list]"
 *
 * 2. variáveis globais reconhecidas:
 * 2.1. o tipo: via opção -t
 * 2.2. a dimensão: via opção -n
 * 2.3. a lista de enums: via opção -e
 *
 * 3. geração do .h
 * 3.1. o cabeçalho inicial é copiador verbatim
 *
 * 3.2. a linha module é detectada:
 *
 * 3.2.1. o nome qualificado do módulo é reconhecido
 * 3.2.2. o nome do arquivo de saída é obtido a partir do nome qualificado do módulo e acrescentando .h
 * 3.2.3. o nome do arquivo de entrada é obtido a partir do nome qualificado do módulo e acrescentando .k
 * 3.2.4. a linha inteira é copiada verbatim para o .h dentro de um comentário //
 *
 * 3.3. é inserido o define guard
 *
 * 3.4. os imports são transformados em #include <module_name_to_path_name.h>
 *
 * 3.5. todas as substituições das marcas são substituídas:
 * 3.5.1. $T é substituído pelo tipo do módulo
 * 3.5.2. $N é substituído pela dimensão do módulo
 * 3.5.3. $E é substituído pela lista de enums
 *
 * Detalhes:
 *  - nome qualifficado de módulo para nome de arquivo (module_name_to_path_name): subistitui o '.' por '/' e acrescenta a extensão
 *  - Cada marca é substituída no máximo uma vez; o texto produzido pela substituição não contém novas marcas semânticas para o gerador.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MEGA 1024*1024
char memory[3*MEGA];

//-------
// arena
//-------

typedef struct {
    size_t top, cap;
    char *ptr;
} arena;

void *arena_alloc(arena *a, size_t len) {
    if(a->top + len > a->cap) {
        return NULL;
    }
    void *ptr = a->ptr + a->top;
    a->top += len;
    return ptr;
}

// o primeiro MEGA e do strbuf de entrada; o resto e da arena
arena a = {.top = 0, .cap = 2*MEGA, .ptr = memory + MEGA};

//---------
// strings : strbuf e string
//---------

typedef struct {
    size_t len, cap;
    char *ptr;
} strbuf;

typedef struct {
    size_t len;
    char  *ptr;
} string;

void strbuf_append_char(strbuf *sb, char c) {
    assert(sb);
    assert(sb->ptr);
    if (sb->len + 1 > sb->cap) {
        return;
    }
    sb->ptr[sb->len++] = c;
}

void strbuf_append_pchar(strbuf *sb, const char *str) {
    assert(sb);
    assert(sb->ptr);
    size_t len = strlen(str);
    if (sb->len + len > sb->cap) {
        return;
    }
    memcpy(sb->ptr + sb->len, str, len);
    sb->len += len;
}

void strbuf_append_strbuf(strbuf *sb, strbuf *s) {
    assert(sb);
    assert(sb->ptr);
    if (sb->len + s->len > sb->cap) {
        return;
    }
    memcpy(sb->ptr + sb->len, s->ptr, s->len);
    sb->len += s->len;
}

void strbuf_append_string(strbuf *sb, string s) {
    assert(sb);
    assert(sb->ptr);
    if (sb->len + s.len > sb->cap) {
        return;
    }
    memcpy(sb->ptr + sb->len, s.ptr, s.len);
    sb->len += s.len;
}


void strbuf_to_pchar(strbuf *b, char *out, size_t out_size) {
    if (b->len + 1 > out_size) return;
    memcpy(out, b->ptr, b->len);
    out[b->len] = '\0';
}

strbuf strbuf_from_arena(arena *a, size_t cap) {
    char *ptr = arena_alloc(a, cap);
    strbuf sb = {.len = 0, .cap = ptr ? cap : 0, .ptr = ptr};
    return sb;
}

// NULL se nao coube: e aqui que o transbordo silencioso dos append vira visivel
char *strbuf_cstr(strbuf *sb) {
    if (!sb->ptr || sb->len + 1 > sb->cap) return NULL;
    sb->ptr[sb->len] = '\0';
    return sb->ptr;
}

string string_from(strbuf *sb, size_t start, size_t size) {
    string s = {.len = size, .ptr = sb->ptr + start};
    return s;
}

void string_to_pchar(string s, char *out, size_t out_size) {
    if (s.len + 1 > out_size) return;
    memcpy(out, s.ptr, s.len);
    out[s.len] = '\0';
}

//---------
// globals
//---------

char *module_name = NULL;
char *input_dir = NULL;
char *output_dir = NULL;
char *t_type = NULL;
char *e_list = NULL;
char *n_dim = NULL;
char *a_list = NULL;   // headers do argumento, separados por virgula

// as linhas de #include que $A produz, montadas uma vez em main
strbuf arg_includes = {0};

// o texto antes da primeira marca: comentario de cabecalho, que sai acima do
// include guard em cada um dos tres arquivos
strbuf preamble = {0};

strbuf input_buffer = {.len = 0, .cap = MEGA, .ptr = memory};

// as tres secoes de saida, na ordem das camadas do backend 4.3.2
enum { SEC_TYPE, SEC_DECL, SEC_IMPL, SEC_COUNT };

const char *section_mark[SEC_COUNT] = { "%type", "%h", "%impl" };
// %h e o L2 (.proto.h); %impl e o L3, o .h que se inclui para usar
const char *section_ext[SEC_COUNT]  = { ".type.h", ".proto.h", ".h" };
const char *section_guard[SEC_COUNT] = { "_TYPE_H", "_PROTO_H", "_H" };

//---------
// files and i/o
//---------


bool path_of(char *out, size_t out_size, const char *filename) {
    const char *slash = strrchr(filename, '/');
    if (!slash) {
        if (out_size == 0) return false;
        out[0] = '\0';
        return true;
    }

    size_t len = (size_t)(slash - filename);
    if (len + 1 > out_size) return false;

    memcpy(out, filename, len);
    out[len] = '\0';
    return true;
}

// "src" ou "src/" -> "src/";  "" -> ""
void strbuf_append_dir(strbuf *sb, const char *dir) {
    if (!dir || !dir[0]) return;
    strbuf_append_pchar(sb, dir);
    if (dir[strlen(dir) - 1] != '/') strbuf_append_char(sb, '/');
}

// "keel.buffer" -> "keel/buffer"
void strbuf_append_qualified_path(strbuf *sb, const char *name) {
    for (const char *p = name; *p; p++) {
        strbuf_append_char(sb, *p == '.' ? '/' : *p);
    }
}

// "keel.buffer" -> "keel/";  "keel" -> ""
void strbuf_append_qualified_dir(strbuf *sb, const char *name) {
    const char *last = strrchr(name, '.');
    if (!last) return;
    for (const char *p = name; p < last; p++) {
        strbuf_append_char(sb, *p == '.' ? '/' : *p);
    }
    strbuf_append_char(sb, '/');
}

// "keel.buffer" -> "keel_buffer"
void strbuf_append_qualified_type(strbuf *sb, const char *name) {
    for (const char *p = name; *p; p++) {
        strbuf_append_char(sb, *p == '.' ? '_' : *p);
    }
}

// "keel/keel_buffer_i32" -> "KEEL_KEEL_BUFFER_I32"
void strbuf_append_upper(strbuf *sb, const char *text) {
    for (const char *p = text; *p; p++) {
        char c = *p;
        bool alnum = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        strbuf_append_char(sb, alnum ? (char)toupper((unsigned char)c) : '_');
    }
}

// o fonte mora no caminho do modulo:  <dir>/keel/buffer.k
void build_source_name(strbuf *out, const char *dir, const char *module, const char *ext) {
    strbuf_append_dir(out, dir);
    strbuf_append_qualified_path(out, module);
    strbuf_append_pchar(out, ext);
}

// O nome do alvo e sempre o simbolo manglado do modulo, sob o caminho dele:
//
//   keel.arena              ->  <dir>/keel/keel_arena.h
//   keel.buffer   -t i32    ->  <dir>/keel/keel_buffer_i32.h
//   keel                    ->  <dir>/keel.h
//
// Sem -t muda so o sufixo do argumento, nao a regra: o arquivo leva o mesmo
// nome do tipo que ele declara, e e isso que torna o #include derivavel do
// nome do tipo, sem saber se ele veio de modulo ou de instancia.
void build_target_name(strbuf *out, const char *dir, const char *module,
                       const char *type, const char *ext) {
    strbuf_append_dir(out, dir);
    strbuf_append_qualified_dir(out, module);
    strbuf_append_qualified_type(out, module);
    if (type && type[0]) {
        strbuf_append_char(out, '_');
        strbuf_append_pchar(out, type);
    }
    strbuf_append_pchar(out, ext);
}

bool mkdir_p(const char *path) {
    char buffer[1024];
    size_t i = 0;

    for (; path[i] && i + 1 < sizeof buffer; i++) {
        buffer[i] = path[i];
    }
    if (path[i] != '\0') return false;  // caminho longo demais
    buffer[i] = '\0';

    for (char *p = buffer + 1; *p; p++) {
        if (*p != '/') continue;

        *p = '\0';
        if (mkdir(buffer, 0755) != 0 && errno != EEXIST) return false;
        *p = '/';
    }

    return mkdir(buffer, 0755) == 0 || errno == EEXIST;
}

void load_file(char *file, strbuf *sb) {
    FILE *f = fopen(file, "r");
    if (!f) {
        printf("Cannot open file. File %s not found\n", file);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    sb->len = ftell(f);
    fseek(f, 0, SEEK_SET);
    fread(sb->ptr, 1, sb->len, f);
    fclose(f);
}

void save_file3(char *file, strbuf *head, strbuf *body, strbuf *tail) {
    FILE *f = fopen(file, "w");
    if (!f) {
        printf("Cannot write file. File %s error\n", file);
        exit(1);
    }
    fwrite(head->ptr, 1, head->len, f);
    fwrite(body->ptr, 1, body->len, f);
    fwrite(tail->ptr, 1, tail->len, f);
    fclose(f);
}

void save_file(char *file, strbuf *sb) {
    FILE *f = fopen(file, "w");
    if (!f) {
        printf("Cannot write file. File %s error\n", file);
        exit(1);
    }
    fwrite(sb->ptr, 1, sb->len, f);
    fclose(f);
}


//---------
// parser and code generation
//---------

// emite no destino corrente; cur < 0 e o preambulo, que e comum aos tres
void emit_char(strbuf *out[SEC_COUNT], int cur, char c) {
    strbuf_append_char(cur < 0 ? &preamble : out[cur], c);
}

void emit_string(strbuf *out[SEC_COUNT], int cur, string v) {
    strbuf_append_string(cur < 0 ? &preamble : out[cur], v);
}

// a marca ocupa a linha inteira e so vale em coluna zero
int section_at(const char *prog, const char *end) {
    for (int s = 0; s < SEC_COUNT; s++) {
        size_t n = strlen(section_mark[s]);
        if ((size_t)(end - prog) < n) continue;
        if (memcmp(prog, section_mark[s], n) != 0) continue;
        char after = (prog + n < end) ? prog[n] : '\n';
        if (after == '\n' || after == '\r' || after == ' ' || after == '\t') return s;
    }
    return -1;
}

void transform(strbuf *input, strbuf *out[SEC_COUNT]) {
    puts("transforming...");

    string str_t_type = (string){.len = strlen(t_type), .ptr = t_type};
    string str_e_list = (string){.len = strlen(e_list), .ptr = e_list};
    string str_n_dim  = (string){.len = strlen(n_dim),  .ptr = n_dim};

    printf("$T = %.*s, $E = %.*s, $N = %.*s\n",
        (int)str_t_type.len, str_t_type.ptr,
        (int)str_e_list.len, str_e_list.ptr,
        (int)str_n_dim.len, str_n_dim.ptr);

    char *prog = input->ptr;
    char *end  = input->ptr + input->len;
    int  cur   = -1;         // antes da primeira marca: preambulo
    bool bol   = true;       // begin of line

    while (prog < end) {
        if (bol) {
            int s = section_at(prog, end);
            if (s >= 0) {
                cur = s;
                while (prog < end && *prog != '\n') prog++;   // a linha da marca nao sai
                if (prog < end) prog++;
                continue;
            }
        }

        if (*prog == '$' && prog + 1 < end) {
            if (prog[1] == 'A') {
                // sem -a a marca some, e leva junto a quebra de linha dela,
                // para nao deixar linha em branco solta no gerado
                emit_string(out, cur, (string){.len = arg_includes.len, .ptr = arg_includes.ptr});
                prog += 2;
                if (arg_includes.len == 0 && prog < end && *prog == '\n') prog++;
                bol = (arg_includes.len == 0);
                continue;
            }
            if (prog[1] == 'T') { emit_string(out, cur, str_t_type); prog += 2; bol = false; continue; }
            if (prog[1] == 'E') { emit_string(out, cur, str_e_list); prog += 2; bol = false; continue; }
            if (prog[1] == 'N') { emit_string(out, cur, str_n_dim);  prog += 2; bol = false; continue; }
        }

        emit_char(out, cur, *prog);
        bol = (*prog == '\n');
        prog++;
    }

    puts("transformed");
}

int main(int argc, char *argv[]) {
    puts("transform: simple code generation for keel files");
    for (int i = 0; i < argc; i++) {
        if(strcmp(argv[i], "-m") == 0) {
            module_name = argv[i + 1];
            i++;
        }
        if(strcmp(argv[i], "-i") == 0) {
            input_dir = argv[i + 1];
            i++;
        }
        if(strcmp(argv[i], "-d") == 0) {
            output_dir = argv[i + 1];
            i++;
        }
        if(strcmp(argv[i], "-t") == 0) {
            t_type = argv[i + 1];
            i++;
        }
        if(strcmp(argv[i], "-e") == 0) {
            e_list = argv[i + 1];
            i++;
        }
        if(strcmp(argv[i], "-n") == 0) {
            n_dim = argv[i + 1];
            i++;
        }
        if(strcmp(argv[i], "-a") == 0) {
            a_list = argv[i + 1];
            i++;
        }
    }

    if(t_type == NULL) {
        t_type = "";
    }

    if(e_list == NULL) {
        e_list = "";
    }

    if(n_dim == NULL) {
        n_dim = "";
    }

    if(a_list == NULL) {
        a_list = "";
    }

    if(module_name == NULL) {
        puts("-m module name is required");
        return 1;
    }

    if(input_dir == NULL) {
        input_dir = "./";
    }

    if(output_dir == NULL) {
        output_dir = "./";
    }

    printf("- Module Name: %s \n", module_name);
    printf("- Input Dir: %s \n", input_dir);
    printf("- Output Dir: %s \n", output_dir);
    printf("- Type: %s \n", t_type ? t_type : "not set");
    printf("- Enum List:  %s \n", e_list ? e_list : "not set");
    printf("- Dimension: %s \n", n_dim ? n_dim : "not set");
    printf("- Arg headers: %s \n", a_list[0] ? a_list : "not set");

    // uma linha de #include por entrada da lista; $A emite o bloco inteiro
    arg_includes = strbuf_from_arena(&a, 4096);
    for (const char *p = a_list; *p; ) {
        while (*p == ' ' || *p == ',') p++;
        if (!*p) break;
        strbuf_append_pchar(&arg_includes, "#include \"");
        while (*p && *p != ',') strbuf_append_char(&arg_includes, *p++);
        strbuf_append_pchar(&arg_includes, "\"\n");
    }

    strbuf source_name = strbuf_from_arena(&a, 512);
    build_source_name(&source_name, input_dir, module_name, ".k");

    char *source_file = strbuf_cstr(&source_name);
    if (!source_file) {
        puts("source name does not fit in the name buffer");
        return 1;
    }
    printf("- Input file: %s \n", source_file);

    preamble = strbuf_from_arena(&a, 8192);

    strbuf  section_buf[SEC_COUNT];
    strbuf *section[SEC_COUNT];
    for (int s = 0; s < SEC_COUNT; s++) {
        section_buf[s] = strbuf_from_arena(&a, 200*1024);
        section[s] = &section_buf[s];
    }

    load_file(source_file, &input_buffer);
    transform(&input_buffer, section);

    for (int s = 0; s < SEC_COUNT; s++) {
        // o caminho de escrita leva o --dest-dir; o de include, nao
        strbuf target = strbuf_from_arena(&a, 512);
        strbuf include = strbuf_from_arena(&a, 512);
        strbuf head = strbuf_from_arena(&a, 1024);
        strbuf tail = strbuf_from_arena(&a, 512);

        build_target_name(&target, output_dir, module_name, t_type, section_ext[s]);

        // o guard vem do nome do alvo sem diretorio de destino, em caixa alta
        build_target_name(&include, "", module_name, t_type, "");

        // o comentario de cabecalho vem primeiro; o guard, abaixo dele
        strbuf_append_strbuf(&head, &preamble);
        strbuf_append_pchar(&head, "#ifndef ");
        strbuf_append_upper(&head, strbuf_cstr(&include) ? include.ptr : "");
        strbuf_append_pchar(&head, section_guard[s]);
        strbuf_append_pchar(&head, "\n#define ");
        strbuf_append_upper(&head, include.ptr);
        strbuf_append_pchar(&head, section_guard[s]);
        strbuf_append_pchar(&head, "\n\n");

        // auto-include: o .proto.h puxa o .type.h, o .h puxa o .proto.h (backend 4.3.2)
        if (s != SEC_TYPE) {
            strbuf_append_pchar(&head, "#include \"");
            strbuf_append_pchar(&head, include.ptr);
            strbuf_append_pchar(&head, section_ext[s - 1]);
            strbuf_append_pchar(&head, "\"\n\n");
        }

        strbuf_append_pchar(&tail, "\n#endif // ");
        strbuf_append_upper(&tail, include.ptr);
        strbuf_append_pchar(&tail, section_guard[s]);
        strbuf_append_pchar(&tail, "\n");

        char *target_file = strbuf_cstr(&target);
        if (!target_file) {
            puts("target name does not fit in the name buffer");
            return 1;
        }

        char target_dir[512] = {0};
        if (path_of(target_dir, sizeof target_dir, target_file) && target_dir[0]) {
            mkdir_p(target_dir);
        }

        save_file3(target_file, &head, section[s], &tail);
        printf("- Output file: %s \n", target_file);
    }

    puts("");
    return 0;
}
