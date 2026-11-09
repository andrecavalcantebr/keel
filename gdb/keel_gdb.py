#!/usr/bin/env python3
"""keel_gdb — GDB companion for keel's .keelsyms (cgen-tool-spec.md §4.11).

Shows the keel name of a symbol next to its mangled C name. Status:
skeleton (gdb/README.md) — load_keelsyms and `keel-load` are real; the
frame filter and `keel-break` are not written yet.
"""
import glob
import os

import gdb

mangled_to_pretty = {}
pretty_to_mangled = {}


def load_keelsyms(root):
    """Reads every *.keelsyms under root. Returns (mangled->pretty, pretty->mangled)."""
    m2p, p2m = {}, {}
    for path in glob.glob(os.path.join(root, "**", "*.keelsyms"), recursive=True):
        with open(path, encoding="utf-8") as f:
            for line in f:
                line = line.rstrip("\n")
                if not line or line.startswith("#"):
                    continue
                mangled, pretty, _kind = line.split("\t")
                m2p[mangled] = pretty
                p2m[pretty] = mangled
    return m2p, p2m


class KeelLoad(gdb.Command):
    """keel-load <dir> — reads every .keelsyms under <dir>, recursively."""

    def __init__(self):
        super().__init__("keel-load", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        global mangled_to_pretty, pretty_to_mangled
        root = arg.strip()
        mangled_to_pretty, pretty_to_mangled = load_keelsyms(root)
        print(f"keel: {len(mangled_to_pretty)} símbolos carregados de {root}")


KeelLoad()

# TODO (gdb/README.md "O que falta"): um gdb.FrameDecorator que anote
# bt/frame com os dois nomes, manglado e bonito, sem substituir um pelo
# outro.

# TODO: comando `keel-break <nome>`, que aceita o nome bonito ou o
# manglado, resolve pelo dicionário acima e chama gdb.Breakpoint sobre o
# símbolo manglado — o breakpoint real é sempre sobre ele.
