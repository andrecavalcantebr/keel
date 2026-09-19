// editors/verify.mjs — asserts that every keel word comes out with a keel scope.
//
//   cd /tmp && mkdir -p tm && cd tm && npm init -y
//   npm install vscode-textmate vscode-oniguruma
//   curl -sSLO https://raw.githubusercontent.com/microsoft/vscode/main/extensions/cpp/syntaxes/c.tmLanguage.json
//   mv c.tmLanguage.json c.json
//   cp <repo>/editors/verify.mjs .
//   node verify.mjs <repo>/editors/vscode/syntaxes <repo>/golden/casos/*/*.k
//
// Exits with code 1 if any word is left unhighlighted.
import fs from 'node:fs';
import onigPkg from 'vscode-oniguruma';
const oniguruma = onigPkg.loadWASM ? onigPkg : onigPkg.default;
import tmPkg from 'vscode-textmate';
const textmate = tmPkg.Registry ? tmPkg : tmPkg.default;

const wasm = fs.readFileSync(new URL('./node_modules/vscode-oniguruma/release/onig.wasm', import.meta.url));
await oniguruma.loadWASM(wasm.buffer);
const onig = Promise.resolve({
  createOnigScanner: s => new oniguruma.OnigScanner(s),
  createOnigString:  s => new oniguruma.OnigString(s),
});

const dir = process.argv[2];
const G = {
  'source.keel':    JSON.parse(fs.readFileSync(dir + '/keel.tmLanguage.json', 'utf8')),
  'keel.injection': JSON.parse(fs.readFileSync(dir + '/keel-injection.tmLanguage.json', 'utf8')),
  'source.c':       JSON.parse(fs.readFileSync('c.json', 'utf8')),
};
const registry = new textmate.Registry({
  onigLib: onig,
  loadGrammar: s => Promise.resolve(G[s] ?? null),
  getInjections: s => (s === 'source.keel' ? ['keel.injection'] : []),
});
const g = await registry.loadGrammar('source.keel');

// the words come from the injection's patterns — the same source the spec fed
const WORDS = new Set();
for (const p of G['keel.injection'].patterns) {
  const m = /\\b\((.+?)\)\\b/.exec(p.match);
  if (m) for (const w of m[1].split('|')) WORDS.add(w);
}

let missing = 0, seen = 0;
for (const file of process.argv.slice(3)) {
  let rule = textmate.INITIAL, n = 0;
  for (const line of fs.readFileSync(file, 'utf8').split('\n')) {
    n++;
    const r = g.tokenizeLine(line, rule); rule = r.ruleStack;
    for (const t of r.tokens) {
      const txt = line.substring(t.startIndex, t.endIndex).trim();
      if (!WORDS.has(txt)) continue;
      const scopes = t.scopes.join(' ');
      if (/comment|string/.test(scopes)) continue;      // inside a comment does not count
      seen++;
      if (!/\.keel\b/.test(t.scopes[t.scopes.length - 1])) {
        console.log(`  UNHIGHLIGHTED  ${file}:${n}  '${txt}'  →  ${t.scopes[t.scopes.length-1]}`);
        missing++;
      }
    }
  }
}
console.log(`\n${seen} keel word occurrences, ${missing} unhighlighted`);
process.exit(missing ? 1 : 0);
