module app.ln;
import keel.buffer as buffer types;

/* Erro de TIPO deliberado, na linha 9. O compilador C tem de reclamar dele
   apontando ESTE arquivo e ESTA linha, com os nomes que estão escritos aqui —
   é o princípio 3, e quem o sustenta é o `#line` do §6. */
pub void erra(buffer i32 *b) {
    f32 *p = buffer.ptr(b);
    (void)p;
}
