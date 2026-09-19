module app.cfg;

import keel.arena   as arena   types;
import keel.buffer  as buffer  types;
import keel.outcome as outcome types;
import_c <stdio.h>;

pub constexpr size_t MAX = 256;

/* Every exit is a `return` or the natural end → label ladder (backend §5.5.2). */
pub outcome i32 sum(arena *a, const char *path) {
    outcome i32 result = {0};
    FILE *ref fp = fopen(path, "rb");
    if (!fp) return outcome.fail(result, 1);
    defer [now FILE *fp] fclose(fp);

    buffer i32 xs = buffer.from(arena.alloc(a, i32, MAX), MAX);
    if (buffer.capacity(xs) == 0) return outcome.fail(result, 2);

    i32 v;
    while (fscanf(fp, "%d", &v) == 1)
        if (buffer.push(xs, v) == NULL) return outcome.fail(result, 3);

    i32 total = 0;
    foreach (i32 x, size_t i : xs) total += x;
    return outcome.win(result, total);
}

/* Arena in the frame: `from_stack` demands a compile-time constant size. */
pub outcome i32 sum_scratch(const char *path) {
    outcome i32 result = {0};
    arena t;
    if (!arena.from_stack(t, 4096)) return outcome.fail(result, 4);
    return sum(t, path);
}
