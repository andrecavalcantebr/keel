module app.lib.other;
import keel.tagged as tagged types;

pub tags Phase [ON, OFF];
pub typedef struct { tagged Phase void ph; } Gate;
