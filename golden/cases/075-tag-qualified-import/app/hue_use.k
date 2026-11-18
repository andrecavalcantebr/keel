module app.hue_use;
import app.hue as hue types;

pub int green(void) { return hue.Hue.GREEN; }

pub hue.Hue next(hue.Hue h) {
    if (h == hue.Hue.RED) return hue.Hue.GREEN;
    return hue.Hue.BLUE;
}
