#ifndef CGEN_CLI_LIMITS_H
#define CGEN_CLI_LIMITS_H
/* Build-time limits. Search storage also reserves the implicit base root. */
#ifndef CGEN_CLI_ROOTS_CAP
#define CGEN_CLI_ROOTS_CAP 128
#endif
#ifndef CGEN_CLI_ARGS_CAP
#define CGEN_CLI_ARGS_CAP 256
#endif
#define CGEN_SEARCH_ROOTS_CAP (CGEN_CLI_ROOTS_CAP + 1)
/* Detail text of the parse dump's island lines: bytes per token of the file,
   plus a fixed base. */
#ifndef CGEN_ISLAND_TEXT_PER_TOKEN
#define CGEN_ISLAND_TEXT_PER_TOKEN 32
#endif
/* the closure's dump lines: one per added instance and per lost verb */
#ifndef CGEN_CLOSURE_TEXT
#define CGEN_CLOSURE_TEXT 65536
#endif
/* entries of the closure of instances, and verbs they lose, per module */
#ifndef CGEN_CLOSURE_MAX
#define CGEN_CLOSURE_MAX 128
#endif
#ifndef CGEN_UNAVAILABLE_MAX
#define CGEN_UNAVAILABLE_MAX 512
#endif
#ifndef CGEN_ISLAND_TEXT_BASE
#define CGEN_ISLAND_TEXT_BASE 1024
#endif
_Static_assert(CGEN_CLI_ROOTS_CAP > 0, "CLI roots capacity must be positive");
_Static_assert(CGEN_CLI_ARGS_CAP > 0, "CLI arguments capacity must be positive");
#endif
