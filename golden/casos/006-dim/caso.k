module app.g;
import grid types;
import_c <stdio.h>;

constexpr i8 DIM = 3;

/* Os dois parâmetros são a MESMA instância: o argumento de `dim` substitui o
   valor, nunca a grafia (§4.9). Se fossem tipos distintos, o C não compilaria. */
pub f32 primeiro(grade(3) f32 *a, grade(DIM) f32 *b) {
    return *ptr(a, (size_t[3]){0,0,0}) + *ptr(b, (size_t[3]){0,0,0});
}

pub int main(int argc, char **argv) {
    array f32 dados[8];
    grade(DIM) f32 g;
    g.dims[0] = 2; g.dims[1] = 2; g.dims[2] = 2; g.ptr = dados;
    dados[0] = 1.5f;
    if (primeiro(g, g) != 3.0f) return 1;      /* o mesmo objeto nas duas posições */
    puts("ok");
    return 0;
}
