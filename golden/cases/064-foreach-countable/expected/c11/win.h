/* win.h — generated from win.k by cgen, C11 profile. */
#ifndef WIN_H
#define WIN_H

#include "win.type.h"

#line 8 "win.k"
static inline size_t win_first(win_window *w);
static inline size_t win_limit(win_window *w);
#line 11 "win.k"
size_t win_sum_param(win_window *w);
#line 19 "win.k"
size_t win_sum_local(void);

#line 8 "win.k"
static inline size_t win_first(win_window *w) { return w->lo; }
static inline size_t win_limit(win_window *w) { return w->hi; }

#endif /* WIN_H */
