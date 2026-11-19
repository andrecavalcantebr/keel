module app.hue2_use;
import app.hue2 types;

pub int level(void) { return Mode.LOW; }

pub Mode step(Mode m) {
    if (m == Mode.OFF) return Mode.LOW;
    return Mode.HIGH;
}
