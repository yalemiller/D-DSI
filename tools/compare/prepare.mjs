// Prepares the pixel comparison between the native renderer and the browser
// prototype. It writes tools/compare/www/ (git-ignored) with:
//   - index.html (the comparison page)
//   - pocket-sheet-sim-v2.js, patched so frames are stepped by hand (tick(n))
//     instead of requestAnimationFrame, so both sides run the same frames
//   - shots/: the CI screenshots (the "screenshots" artifact, unzipped)
//
// Usage: node tools/compare/prepare.mjs PATH/TO/UNZIPPED/SHOTS
// Then serve tools/compare/www/ (e.g. py -m http.server 8771 --directory tools/compare/www),
// open it and press "Run all". Every row should report 0 differing pixels.
import { cpSync, mkdirSync, readFileSync, readdirSync, rmSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, '..', '..');
const shots = process.argv[2];
if (!shots) {
  console.error('usage: node tools/compare/prepare.mjs PATH/TO/UNZIPPED/SHOTS');
  process.exit(1);
}
const www = join(here, 'www');
rmSync(www, { recursive: true, force: true });
mkdirSync(join(www, 'shots'), { recursive: true });

let sim = readFileSync(join(root, 'docs/handoff/prototype/pocket-sheet-sim-v2.js'), 'utf8');
const loop = '  raf = requestAnimationFrame(loop);\n';
const destroy = '    destroy() { cancelAnimationFrame(raf); },';
if (!sim.includes(loop) || !sim.includes(destroy)) throw new Error('prototype changed: update the patch');
sim = sim.replace(loop, '  // (compare harness: frames are stepped manually with tick())\n')
  .replace(destroy, destroy + '\n    tick(n) { for (let i = 0; i < n; i++) step(); render(); },');
writeFileSync(join(www, 'pocket-sheet-sim-v2.js'), sim);
cpSync(join(here, 'index.html'), join(www, 'index.html'));

const pngs = readdirSync(shots).filter(f => f.endsWith('.png')).sort();
for (const f of pngs) cpSync(join(shots, f), join(www, 'shots', f));
writeFileSync(join(www, 'shots', 'index.json'), JSON.stringify(pngs));
console.log(`wrote ${www} with ${pngs.length} screenshots`);
