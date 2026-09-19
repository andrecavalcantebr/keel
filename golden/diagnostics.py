#!/usr/bin/env python3
"""Asks Zed's clangd what it thinks of a file, over LSP.

`clangd --check` runs neither the IncludeCleaner nor hints, so it reports
"0 errors" for a file the editor underlines. This shows what the editor shows.

    python3 golden/diagnostics.py golden/cases/002-from-array/expected/c11/app/app_pool.h
"""
import json, subprocess, sys, os, glob
CL = glob.glob(os.path.expanduser("~/.local/share/zed/languages/clangd/*/bin/clangd"))[0]
file = os.path.abspath(sys.argv[1])
root = os.path.abspath(".")
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
     "processId":None,"rootUri":"file://"+root,
     "capabilities":{"textDocument":{"publishDiagnostics":{"tagSupport":{"valueSet":[1,2]}}}}}})
while True:
    m = rec()
    if m.get("id") == 1: break
env({"jsonrpc":"2.0","method":"initialized","params":{}})
env({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{
     "uri":"file://"+file,"languageId":"c","version":1,
     "text":open(file).read()}}})
for _ in range(40):
    m = rec()
    if m.get("method") == "textDocument/publishDiagnostics" and m["params"]["uri"].endswith(os.path.basename(file)):
        ds = m["params"]["diagnostics"]
        if not ds: print("  (no diagnostics)")
        for d in ds:
            print("  line %d  [%s]  tags=%s  %s" % (
                d["range"]["start"]["line"]+1, d.get("source","?"),
                d.get("tags"), d["message"].replace("\n"," ")[:90]))
        break
p.kill()
