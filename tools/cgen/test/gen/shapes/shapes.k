module shapes;
import_c <stddef.h>;

pub typedef struct {
    int x;     /* horizontal */
    int y;
} Point;

pub constexpr int LIMIT = 4;     // upper bound

pub int counter = 0;
priv int scale = 3;

// not exported
priv int weight(Point *p) {
    return p->x * scale;
}

pub int sum(Point *p) {
    return weight(p) + p->y + LIMIT;
}
