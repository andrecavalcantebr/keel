module app3.a10;
pub typedef struct { array u8 m[2, 3]; } Img;
pub void fill(Img *p) {
    for (size_t i = 0; i < 2; i++)
        for (size_t j = 0; j < 3; j++)
            p->m[i, j] = (u8)(i * 3 + j);
}
