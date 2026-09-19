/* results.h — gerado de results.k pelo cgen, perfil C11. */
#ifndef RESULTS_H
#define RESULTS_H
#include "results.type.h"
#include "keel/keel_corot.type.h"
#include "keel/keel_outcome_void.type.h"

#line 5 "results.k"
keel_corot results_state(i32 code);
#line 12 "results.k"
bool results_is_fault(i32 code);
#line 17 "results.k"
keel_outcome_void results_final(bool failure, i32 code);
#line 23 "results.k"
i32 results_repair(i32 code);
#line 32 "results.k"
keel_corot results_evaluate_cleanup(i32 *counter);
#endif
