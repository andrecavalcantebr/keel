module app.gfp_use;
import app.gfp as gfp types;

pub size_t outside(void) { return gfp.twice(21); }

pub size_t inside(void) {
    cell(7) u8 c = {0};
    return gfp.width2(c);
}
