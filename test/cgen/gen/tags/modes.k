module modes;

pub tags Mode [OFF, ON, AUTO];
pub tags Level [LOW = -1, MID = 0, HIGH = 10];

pub typedef struct { Mode mode; Level level; } Setting;

pub Mode mode_of(Setting *s) {
    return s->mode;
}
