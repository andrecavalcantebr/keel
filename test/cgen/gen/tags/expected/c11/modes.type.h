/* modes.type.h — generated from modes.k by cgen, C11 profile. */
#ifndef MODES_TYPE_H
#define MODES_TYPE_H

#include "keel.type.h"

#line 3 "modes.k"
typedef enum modes_Mode {modes_Mode_OFF, modes_Mode_ON, modes_Mode_AUTO} modes_Mode;
typedef enum modes_Level {
    modes_Level_LOW = -1,
    modes_Level_MID = 0,
    modes_Level_HIGH = 10
} modes_Level;
#line 10 "modes.k"
typedef struct modes_Setting { modes_Mode mode; modes_Level level; } modes_Setting;

#endif /* MODES_TYPE_H */
