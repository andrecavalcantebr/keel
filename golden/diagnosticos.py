#!/usr/bin/env python3
"""Pergunta ao clangd do Zed o que ele acha de um arquivo, via LSP.

`clangd --check` não roda o IncludeCleaner nem reporta hints, então ele diz
"0 errors" para arquivo que o editor sublinha. Isto mostra o que o editor mostra.

    python3 golden/diagnosticos.py golden/casos/002-from-array/esperado/c11/app/pool.h
"""
import json, subprocess, sys, os, glob
CL = glob.glob(os.path.expanduser("~/.local/share/zed/languages/clangd/*/bin/clangd"))[0]
arq = os.path.abspath(sys.argv[1])
raiz = os.path.abspath(".")
p = subprocess.Popen([CL, "--background-index=false"], stdin=subprocess.PIPE,
                     stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
def env(m):
    b = json.dumps(m).encode()
    p.stdin.write(b"Content-Length: %d\r\n\r\n" % len(b) + b); p.stdin.flush()
def rec():
    n = 0
    while True:
        l = p.stdout.readline()
        if l == b"\r\n": break
        if l.lower().startswith(b"content-length:"): n = int(l.split(b":")[1])
    return json.loads(p.stdout.read(n))

env({"jsonrpc":"2.0","id":1,"method":"initialize","params":{
     "processId":None,"rootUri":"file://"+raiz,
     "capabilities":{"textDocument":{"publishDiagnostics":{"tagSupport":{"valueSet":[1,2]}}}}}})
while True:
    m = rec()
    if m.get("id") == 1: break
env({"jsonrpc":"2.0","method":"initialized","params":{}})
env({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{
     "uri":"file://"+arq,"languageId":"c","version":1,
     "text":open(arq).read()}}})
for _ in range(40):
    m = rec()
    if m.get("method") == "textDocument/publishDiagnostics" and m["params"]["uri"].endswith(os.path.basename(arq)):
        ds = m["params"]["diagnostics"]
        if not ds: print("  (sem diagnóstico)")
        for d in ds:
            print("  linha %d  [%s]  tags=%s  %s" % (
                d["range"]["start"]["line"]+1, d.get("source","?"),
                d.get("tags"), d["message"].replace("\n"," ")[:90]))
        break
p.kill()
