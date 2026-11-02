module pt;

priv tags Mode [OFF = 1, ON = 3, AUTO = 7];

/* The set is private: its enum is a line of the .c, and the function shows the
   path as a number. */
pub i32 trail(void) {
    Mode m = OFF;
    i32 n = 0;
    bool done = false;

    while (!done) {
        match (m) {
            OFF:
                n += 1;
                m = ON;
            ON:
                n += 10;
                m = AUTO;
            AUTO:
                n += 100;
                done = true;
        }
    }
    return n;
}
