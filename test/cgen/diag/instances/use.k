module use;
import gen types;
import inl types;

instance gen.box i32;
instance inl.cell i32;                      /* DIAG: redundant-instance */
instance i32;                               /* DIAG: instance-not-modifier */

pub void f(void) {
    instance gen.box f32;                   /* DIAG: instance-outside-file-scope */
}
