/* app/app_cond.c — generated from app/cond.k by cgen, C23 profile. */
#include "app/app_cond.h"
#line 1 "app/cond.k"









int app_cond_nested(int x) {
#ifdef A
#  ifdef B
    if (x) {
#  else
    if (!x) {
#  endif
#else
    if (x > 1) {
#endif
        return 1;
    }
    return 0;
}


int app_cond_chain(int x) {
#if X == 1
    if (x == 1) {
#elif X == 2
    if (x == 2) {
#elif defined(Z)
    if (x == 3) {
#elif !defined(W)
    if (x == 4) {
#  else
    if (x == 5) {
#endif
        x--;
    }
    return x;
}


int app_cond_across(int x) {
#if X
    if (x) {
#else
    while (x) {
#endif
        x--;
    }
    return x;
}


int app_cond_balanced(int x) {
#if X
    return (x
#else
    return ((x)
#endif
    );
}


int app_cond_deep(int x) {
#ifdef L0
#ifdef L1
#ifdef L2
#ifdef L3
#ifdef L4
#ifdef L5
#ifdef L6
#ifdef L7
    if (x) {
#else
    if (x > 7) {
#endif
#else
    if (x > 6) {
#endif
#else
    if (x > 5) {
#endif
#else
    if (x > 4) {
#endif
#else
    if (x > 3) {
#endif
#else
    if (x > 2) {
#endif
#else
    if (x > 1) {
#endif
#else
    if (x > 0) {
#endif
        x++;
    }
    return x;
}
