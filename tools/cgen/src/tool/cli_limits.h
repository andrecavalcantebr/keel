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
_Static_assert(CGEN_CLI_ROOTS_CAP > 0, "CLI roots capacity must be positive");
_Static_assert(CGEN_CLI_ARGS_CAP > 0, "CLI arguments capacity must be positive");
#endif
