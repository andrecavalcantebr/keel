/* recs.type.h — generated from recs.k by cgen, C23 profile. */
#ifndef RECS_TYPE_H
#define RECS_TYPE_H

#include "keel.type.h"
#include "keel/keel_slice_recs_Person.type.h"
#include "keel/keel_slice_const_u8.type.h"
#include "keel/keel_buffer_i64.type.h"

#line 5 "recs.k"
typedef struct recs_Person { i32 age; } recs_Person;
typedef keel_slice_recs_Person recs_people;
typedef keel_slice_const_u8 recs_bytes;
typedef keel_buffer_i64 recs_wide;

#endif /* RECS_TYPE_H */
