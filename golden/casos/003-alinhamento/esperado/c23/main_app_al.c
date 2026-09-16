/* gen/main_app_al.c — unidade de ponto de entrada, gerada por `--main app.al`.
   Ela NÃO vai dentro do .c do módulo: o conteúdo gerado passaria a depender da
   flag de invocação, e o critério de timestamp deixaria de significar o que
   significa (backend §5.8). */

#include "keel/prelude.h"
#include "app/al.impl.h"
int main(int argc, char **argv) { return app_al_main(argc, argv); }
