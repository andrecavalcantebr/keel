/* keel/keel_outcome_boxes_item.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_BOXES_ITEM_H
#define KEEL_KEEL_OUTCOME_BOXES_ITEM_H

#include "keel/keel_outcome_boxes_item.type.h"
#include "keel/keel_outcome.type.h"
#include "boxes.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_boxes_item_failed(keel_outcome_boxes_item e);
static inline bool keel_outcome_boxes_item_ok    (keel_outcome_boxes_item e);
static inline i32  keel_outcome_boxes_item_code  (keel_outcome_boxes_item e);
static inline boxes_item    keel_outcome_boxes_item_value (keel_outcome_boxes_item e);
static inline void keel_outcome_boxes_item_value1 (keel_outcome_boxes_item *r, boxes_item v);
#line 29 "keel/outcome.k"
static inline keel_outcome_boxes_item keel_outcome_boxes_item_win (keel_outcome_boxes_item *r);
static inline keel_outcome_boxes_item keel_outcome_boxes_item_win1 (keel_outcome_boxes_item *r, boxes_item v);
static inline keel_outcome_boxes_item keel_outcome_boxes_item_fail(keel_outcome_boxes_item *r, i32 c);
static inline keel_outcome_boxes_item keel_outcome_boxes_item_none(keel_outcome_boxes_item *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_boxes_item_failed(keel_outcome_boxes_item e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_boxes_item_ok    (keel_outcome_boxes_item e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_boxes_item_code  (keel_outcome_boxes_item e) { return e.code; }
static inline boxes_item    keel_outcome_boxes_item_value (keel_outcome_boxes_item e) { return e.value; }
static inline void keel_outcome_boxes_item_value1 (keel_outcome_boxes_item *r, boxes_item v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_boxes_item keel_outcome_boxes_item_win (keel_outcome_boxes_item *r)         { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_boxes_item keel_outcome_boxes_item_win1 (keel_outcome_boxes_item *r, boxes_item v)    { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_boxes_item keel_outcome_boxes_item_fail(keel_outcome_boxes_item *r, i32 c)  { r->code = c; return *r; }
static inline keel_outcome_boxes_item keel_outcome_boxes_item_none(keel_outcome_boxes_item *r)         { r->code = keel_outcome_NONE; return *r; }

#endif /* KEEL_KEEL_OUTCOME_BOXES_ITEM_H */
