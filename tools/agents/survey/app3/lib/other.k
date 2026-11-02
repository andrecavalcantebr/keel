module app3.lib.other;
import keel.tagged as tagged types;
pub tags Phase [ON, OFF];
pub typedef struct { tagged Phase void ph; } Gate;
pub constexpr size_t WORKERS = 4;
pub typedef struct { int v; } Item;
