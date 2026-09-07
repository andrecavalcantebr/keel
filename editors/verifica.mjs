// editors/verifica.mjs — afirma que toda palavra de keel sai com escopo keel.
//
//   cd /tmp && mkdir -p tm && cd tm && npm init -y
//   npm install vscode-textmate vscode-oniguruma
//   curl -sSLO https://raw.githubusercontent.com/microsoft/vscode/main/extensions/cpp/syntaxes/c.tmLanguage.json
//   mv c.tmLanguage.json c.json
//   cp <repo>/editors/verifica.mjs .
//   node verifica.mjs <repo>/editors/vscode/syntaxes <repo>/golden/casos/*/*.k
//
// Sai com código 1 se alguma palavra ficar sem realce.
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

// as palavras vêm dos padrões da injeção — mesma fonte que a spec alimentou
const PAL = new Set();
for (const p of G['keel.injection'].patterns) {
  const m = /\\b\((.+?)\)\\b/.exec(p.match);
  if (m) for (const w of m[1].split('|')) PAL.add(w);
}

let faltas = 0, vistas = 0;
for (const arq of process.argv.slice(3)) {
  let regra = textmate.INITIAL, n = 0;
  for (const linha of fs.readFileSync(arq, 'utf8').split('\n')) {
    n++;
    const r = g.tokenizeLine(linha, regra); regra = r.ruleStack;
    for (const t of r.tokens) {
      const txt = linha.substring(t.startIndex, t.endIndex).trim();
      if (!PAL.has(txt)) continue;
      const esc = t.scopes.join(' ');
      if (/comment|string/.test(esc)) continue;      // dentro de comentário não conta
      vistas++;
      if (!/\.keel\b/.test(t.scopes[t.scopes.length - 1])) {
        console.log(`  SEM REALCE  ${arq}:${n}  '${txt}'  →  ${t.scopes[t.scopes.length-1]}`);
        faltas++;
      }
    }
  }
}
console.log(`\n${vistas} ocorrências de palavra keel, ${faltas} sem realce`);
process.exit(faltas ? 1 : 0);
