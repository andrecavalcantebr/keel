#include "tool/memory.h"

/* The one shared copy: memory.h's inline functions are duplicated per
   translation unit (as any `static inline` is), but the counters they touch
   must not be — each .c including memory.h would otherwise tally into its
   own private, mostly-empty table. */
CgenStat cgen_stats[CGEN_STAT_CAP];
size_t cgen_stats_n;
