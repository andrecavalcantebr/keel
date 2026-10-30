module reading;
import_c <stdio.h>;

pub int first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;
    defer fclose(file);
    return fgetc(file);
}
