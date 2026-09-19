module app.reg;

import keel.slice   as slice   types;
import keel.outcome as outcome types;

/* Três campos, dois opcionais. Sem tipo de data, a data de nascimento fica
   sendo o que a libc oferece: um f64, com 0.0 para "não informada". */
pub typedef struct {
    slice const char name;
    slice const char address;
    f64 birth;
} Person;

pub array Person people[4];

priv const char *NAMES[3]   = { "ana", "bo", "cid" };
priv size_t      LENS[3]    = { 3, 2, 3 };
priv const char *ADDRS[3]   = { "rua um", "", "rua tres" };
priv size_t      ADDRLENS[3] = { 6, 0, 8 };
priv f64         BIRTHS[3]  = { 1.5, 2.25, 3.5 };

/* A fonte: cada leitura pode faltar, e cada uma falta por conta própria. */
priv outcome slice const char read_name(size_t i) {
    outcome slice const char r = {0};
    if (i >= 3) return outcome.none(r);
    return outcome.win(r, slice.from(const char, NAMES[i], LENS[i]));
}

priv outcome slice const char read_address(size_t i) {
    outcome slice const char r = {0};
    if (i >= 3 || ADDRLENS[i] == 0) return outcome.none(r);
    return outcome.win(r, slice.from(const char, ADDRS[i], ADDRLENS[i]));
}

priv outcome f64 read_birth(size_t i) {
    outcome f64 r = {0};
    if (i >= 3 || i == 2) return outcome.none(r);
    return outcome.win(r, BIRTHS[i]);
}

/* Os três resultados são locais e vivem FORA do laço: é a declaração deles que
   dá o tipo, e a cauda `else` na atribuição reusa esse tipo a cada volta
   (§4.10). Escrever `people[i].address = read_address(i) else …` seria
   `else-on-complex-target`: o alvo não é identificador declarado. */
pub size_t load(size_t n) {
    outcome slice const char name = {0};
    outcome slice const char address = {0};
    outcome f64 birth = {0};
    size_t k = 0;

    for (size_t i = 0; i < n && i < 4; i++) {
        name = read_name(i) else break;
        address = read_address(i) else slice.from(const char, "", 0);
        birth = read_birth(i) else 0.0;

        people[i].name    = outcome.value(name);
        people[i].address = outcome.value(address);
        people[i].birth   = outcome.value(birth);
        k++;
    }
    return k;
}
