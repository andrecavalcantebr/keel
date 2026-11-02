module app3.fe05;
import keel.slice as slice types;
pub i32 f(slice i32 s)
{
    i32 t = 0;
    foreach (i32 x, size_t i : s)
    {
        t += x - (i32)i;
    }
    return t;
}
