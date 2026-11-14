/* Build provenance printed by `--cgen-version` (cgen-tool-spec §4.1).
   The values come from the build (tools/cgen/Makefile, tools/dist/build.sh)
   as -D macros; a build that does not pass them says "unknown". This file is
   the only one that carries them, so a stale value cannot hide in another
   object. */

#ifndef CGEN_BUILD_COMMIT
#define CGEN_BUILD_COMMIT "unknown"
#endif
#ifndef CGEN_BUILD_DATE
#define CGEN_BUILD_DATE "unknown"
#endif
#ifndef CGEN_BUILD_CC
#define CGEN_BUILD_CC "unknown"
#endif

const char cgen_build_commit[] = CGEN_BUILD_COMMIT;
const char cgen_build_date[] = CGEN_BUILD_DATE;
const char cgen_build_cc[] = CGEN_BUILD_CC;
