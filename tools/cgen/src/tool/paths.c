/* tool/paths.c — caminho ↔ módulo (cgen-tool §2.7, §3.3). Só texto:
   nenhuma destas funções abre arquivo nem consulta o sistema. É por isso
   que elas ficam separadas do `tool.c`, que faz as duas coisas. */
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* Normaliza lexicalmente, como o §3.3 item 4 manda: remove `./`, colapsa
   `//`, resolve `..`. Lexical de propósito — não segue symlink e não
   toca no disco, então `a/b/../c` é `a/c` mesmo que `a/b` não exista.
   Um `..` que sobe além do começo de um caminho relativo é preservado,
   porque `../x` é um caminho legítimo; num caminho absoluto ele é
   descartado, porque `/..` é `/`. Devolve false se não couber. */
bool cgen_path_normalize(const char *path, char *out, size_t cap) {
    if (path == NULL || out == NULL || cap == 0) return false;
    bool absolute = path[0] == '/';
    size_t len = 0;
    if (absolute) {
        if (cap < 2) return false;
        out[len++] = '/';
    }
    size_t kept = len;   /* onde começa a parte que `..` pode comer */

    const char *p = path;
    while (*p) {
        while (*p == '/') p++;
        const char *seg = p;
        while (*p && *p != '/') p++;
        size_t seg_len = (size_t)(p - seg);
        if (seg_len == 0) continue;
        if (seg_len == 1 && seg[0] == '.') continue;
        if (seg_len == 2 && seg[0] == '.' && seg[1] == '.') {
            if (len > kept) {
                /* volta um segmento */
                size_t back = len - 1;
                if (out[back] == '/') back--;
                while (back > kept && out[back - 1] != '/') back--;
                len = back;
                if (len > kept && out[len - 1] == '/') len--;
                continue;
            }
            if (absolute) continue;   /* `/..` é `/` */
            /* relativo: `..` acima da raiz do caminho é parte do caminho */
        }
        if (len > 0 && out[len - 1] != '/') {
            if (len + 1 >= cap) return false;
            out[len++] = '/';
        }
        if (len + seg_len >= cap) return false;
        memcpy(out + len, seg, seg_len);
        len += seg_len;
        if (seg_len == 2 && seg[0] == '.' && seg[1] == '.') kept = len;
    }
    if (len == 0) {
        if (cap < 2) return false;
        out[len++] = '.';
    }
    out[len] = '\0';
    return true;
}

/* O nome de módulo esperado de um fonte: o caminho relativo à raiz, sem
   o `.k`, com `/` virando `.` (§3.3 item 4). `source` e `root` já
   normalizados. Devolve false se o fonte não estiver sob a raiz, se não
   terminar em `.k`, ou se não couber. */
bool cgen_module_name_of(const char *source, const char *root,
                         char *out, size_t cap) {
    if (source == NULL || root == NULL || out == NULL) return false;
    size_t slen = strlen(source);
    if (slen < 2 || strcmp(source + slen - 2, ".k") != 0) return false;
    slen -= 2;

    size_t skip = 0;
    if (strcmp(root, ".") != 0) {
        size_t rlen = strlen(root);
        if (strncmp(source, root, rlen) != 0) return false;
        if (source[rlen] != '/') return false;
        skip = rlen + 1;
    }
    if (skip >= slen) return false;

    size_t n = slen - skip;
    if (n + 1 > cap) return false;
    for (size_t i = 0; i < n; i++) {
        char c = source[skip + i];
        out[i] = c == '/' ? '.' : c;
    }
    out[n] = '\0';
    return true;
}

/* O caminho candidato de um módulo sob uma raiz: `<raiz>/<a>/<b>.k`,
   com `.` virando `/` (§3.3 item 6). Quem percorre as raízes na ordem,
   base por último, e decide qual existe, é o `tool.c`. */
bool cgen_module_path(const char *module_name, size_t name_len,
                      const char *root, char *out, size_t cap) {
    if (module_name == NULL || root == NULL || out == NULL) return false;
    size_t len = 0;
    if (strcmp(root, ".") != 0) {
        size_t rlen = strlen(root);
        if (rlen + 1 >= cap) return false;
        memcpy(out, root, rlen);
        len = rlen;
        out[len++] = '/';
    }
    if (len + name_len + 3 > cap) return false;
    for (size_t i = 0; i < name_len; i++) {
        char c = module_name[i];
        out[len + i] = c == '.' ? '/' : c;
    }
    len += name_len;
    memcpy(out + len, ".k", 3);
    return true;
}
