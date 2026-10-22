// Pocket Sheet v0.2: browser SIMULATION of the native DS-mode app.
// It mirrors the native module split: font/draw, data+rules, save format, storage, input, screens.
// The native build re-implements this in C (BlocksDS/libnds). This file is a reference, not the app.

const W = 256, H = 192;
// Color schemes. Every channel is a multiple of 8, so each maps exactly to DS RGB555.
// Roles: gold = focus/accent, blue = spells/edit/temp HP, red = damage/errors, green = heal/saved, ink = text on accent fills.
export const THEMES = {
  torchlight: { bg:'#181010', panel:'#282018', panel2:'#383028', line:'#585040', text:'#f0e0c0', dim:'#a89878', gold:'#f0a830', red:'#d84830', green:'#78a848', blue:'#7898d0', ink:'#181010' },
  grimoire:   { bg:'#200810', panel:'#381020', panel2:'#501830', line:'#703048', text:'#f8e8c8', dim:'#c8a090', gold:'#e8c050', red:'#f06048', green:'#80c068', blue:'#88a8e8', ink:'#200810' },
  elven:      { bg:'#101810', panel:'#182818', panel2:'#283828', line:'#405840', text:'#e8f0d0', dim:'#98b090', gold:'#d8b048', red:'#e06048', green:'#88d068', blue:'#78b0e0', ink:'#101810' },
  parchment:  { bg:'#e8d8b0', panel:'#d8c898', panel2:'#c8b080', line:'#a08860', text:'#281810', dim:'#584830', gold:'#903010', red:'#a82818', green:'#386820', blue:'#284880', ink:'#f8f0d8' },
  night:      { bg:'#181020', panel:'#282038', panel2:'#383050', line:'#504868', text:'#f8f0d8', dim:'#a098a8', gold:'#f8b840', red:'#e04848', green:'#58c068', blue:'#5890e8', ink:'#181020' },
};
export const PAL = { ...THEMES.torchlight };

// ---------- 5x7 bitmap font (original, CC0) — 6px advance ----------
export const FONT = {
' ':'..... ..... ..... ..... ..... ..... .....',
'A':'.###. #...# #...# ##### #...# #...# #...#',
'B':'####. #...# #...# ####. #...# #...# ####.',
'C':'.###. #...# #.... #.... #.... #...# .###.',
'D':'####. #...# #...# #...# #...# #...# ####.',
'E':'##### #.... #.... ####. #.... #.... #####',
'F':'##### #.... #.... ####. #.... #.... #....',
'G':'.###. #...# #.... #.### #...# #...# .####',
'H':'#...# #...# #...# ##### #...# #...# #...#',
'I':'.###. ..#.. ..#.. ..#.. ..#.. ..#.. .###.',
'J':'..### ...#. ...#. ...#. ...#. #..#. .##..',
'K':'#...# #..#. #.#.. ##... #.#.. #..#. #...#',
'L':'#.... #.... #.... #.... #.... #.... #####',
'M':'#...# ##.## #.#.# #.#.# #...# #...# #...#',
'N':'#...# #...# ##..# #.#.# #..## #...# #...#',
'O':'.###. #...# #...# #...# #...# #...# .###.',
'P':'####. #...# #...# ####. #.... #.... #....',
'Q':'.###. #...# #...# #...# #.#.# #..#. .##.#',
'R':'####. #...# #...# ####. #.#.. #..#. #...#',
'S':'.#### #.... #.... .###. ....# ....# ####.',
'T':'##### ..#.. ..#.. ..#.. ..#.. ..#.. ..#..',
'U':'#...# #...# #...# #...# #...# #...# .###.',
'V':'#...# #...# #...# #...# #...# .#.#. ..#..',
'W':'#...# #...# #...# #.#.# #.#.# #.#.# .#.#.',
'X':'#...# #...# .#.#. ..#.. .#.#. #...# #...#',
'Y':'#...# #...# .#.#. ..#.. ..#.. ..#.. ..#..',
'Z':'##### ....# ...#. ..#.. .#... #.... #####',
'0':'.###. #...# #..## #.#.# ##..# #...# .###.',
'1':'..#.. .##.. ..#.. ..#.. ..#.. ..#.. .###.',
'2':'.###. #...# ....# ...#. ..#.. .#... #####',
'3':'####. ....# ....# .###. ....# ....# ####.',
'4':'...#. ..##. .#.#. #..#. ##### ...#. ...#.',
'5':'##### #.... ####. ....# ....# #...# .###.',
'6':'..##. .#... #.... ####. #...# #...# .###.',
'7':'##### ....# ...#. ..#.. .#... .#... .#...',
'8':'.###. #...# #...# .###. #...# #...# .###.',
'9':'.###. #...# #...# .#### ....# ...#. .##..',
'!':'..#.. ..#.. ..#.. ..#.. ..#.. ..... ..#..',
'?':'.###. #...# ....# ...#. ..#.. ..... ..#..',
'.':'..... ..... ..... ..... ..... ..... ..#..',
',':'..... ..... ..... ..... ..... ..#.. .#...',
':':'..... ..#.. ..... ..... ..... ..#.. .....',
"'":'..#.. ..#.. .#... ..... ..... ..... .....',
'-':'..... ..... ..... ##### ..... ..... .....',
'+':'..... ..#.. ..#.. ##### ..#.. ..#.. .....',
'=':'..... ..... ##### ..... ##### ..... .....',
'/':'....# ....# ...#. ..#.. .#... #.... #....',
'(':'...#. ..#.. .#... .#... .#... ..#.. ...#.',
')':'.#... ..#.. ...#. ...#. ...#. ..#.. .#...',
'&':'.##.. #..#. #.#.. .#... #.#.# #..#. .##.#',
'<':'...#. ..#.. .#... #.... .#... ..#.. ...#.',
'>':'.#... ..#.. ...#. ....# ...#. ..#.. .#...',
'_':'..... ..... ..... ..... ..... ..... #####',
'♥':'..... .#.#. ##### ##### .###. ..#.. .....',
'✓':'..... ....# ...## #.##. ###.. .#... .....',
'✗':'..... #...# .#.#. ..#.. .#.#. #...# .....',
'▶':'#.... ##... ###.. ####. ###.. ##... #....',
'◀':'....# ...## ..### .#### ..### ...## ....#',
'▲':'..... ..#.. .###. ##### ..... ..... .....',
'▼':'..... ..... ##### .###. ..#.. ..... .....',
'●':'..... .###. ##### ##### ##### .###. .....',
'○':'..... .###. #...# #...# #...# .###. .....',
'◆':'..... ..#.. .###. ##### .###. ..#.. .....',
'→':'..... ..#.. ...#. ##### ...#. ..#.. .....',
};
const GL = Object.keys(FONT), GI = {}; GL.forEach((c, i) => GI[c] = i);
const atlasCache = {};
function atlas(col) {
  if (atlasCache[col]) return atlasCache[col];
  const cv = document.createElement('canvas'); cv.width = GL.length * 5; cv.height = 7;
  const c = cv.getContext('2d'); c.fillStyle = col;
  GL.forEach((ch, i) => FONT[ch].split(' ').forEach((row, y) => { for (let x = 0; x < 5; x++) if (row[x] === '#') c.fillRect(i * 5 + x, y, 1, 1); }));
  return atlasCache[col] = cv;
}
function tx(c, s, x, y, col, sc = 1) {
  s = String(s).toUpperCase(); const a = atlas(col);
  for (let i = 0; i < s.length; i++) { const ch = s[i]; if (ch === ' ') continue; const gi = GI[ch] ?? GI['?']; c.drawImage(a, gi * 5, 0, 5, 7, x + i * 6 * sc, y, 5 * sc, 7 * sc); }
}
const tw = (s, sc = 1) => { s = String(s); return s.length ? (s.length * 6 - 1) * sc : 0; };
const txc = (c, s, x, w, y, col, sc = 1) => tx(c, s, x + Math.floor((w - tw(s, sc)) / 2), y, col, sc);
const txr = (c, s, r, y, col, sc = 1) => tx(c, s, r - tw(s, sc), y, col, sc);
const R = (c, x, y, w, h, col) => { c.fillStyle = col; c.fillRect(x, y, w, h); };
const BOX = (c, x, y, w, h, col, t = 1) => { R(c, x, y, w, t, col); R(c, x, y + h - t, w, t, col); R(c, x, y, t, h, col); R(c, x + w - t, y, t, h, col); };
function wrap(s, n) { const out = []; let line = ''; for (const w of String(s).split(' ')) { if (!line) line = w; else if ((line + ' ' + w).length <= n) line += ' ' + w; else { out.push(line); line = w; } } if (line) out.push(line); return out; }
// ---------- icons (original pixel art, CC0). '#' = color 0, '+' = color 1, '=' = color 2 ----------
export const ICONS = {
  sword: ['..........#', '.........##', '........##.', '.......##..', '......##...', '.#...##....', '..#.##.....', '...##......', '..#.#......', '.#...#.....', '#..........'],
  arrow: ['........###', '.........##', '........#.#', '.......#...', '......#....', '.....#.....', '....#......', '...#.......', '#.#........', '.##........', '###........'],
  heart: ['.###...###.', '#####.#####', '###########', '#####+#####', '####+++####', '.####+####.', '..#######..', '...#####...', '....###....', '.....#.....'],
  shield: ['#########', '##+######', '##+######', '##+######', '#########', '.#######.', '.#######.', '..#####..', '...###...', '....#....'],
  skull: ['..#####..', '.#######.', '#########', '#..###..#', '#..###..#', '#########', '.###.###.', '..#####..', '..#.#.#..'],
  fire: ['.....#.....', '.....##....', '....###.#..', '...####.#..', '...##+###..', '..##+++##..', '..##+++##..', '...#####...', '==.......==', '..===.===..', '.....=.....'],
  fire2: ['....#......', '....##.....', '...###.#...', '...####.#..', '...##+###..', '..##+++##..', '..##+++##..', '...#####...', '==.......==', '..===.===..', '.....=.....'],
  moon: ['...####....', '.####....+.', '.###....+++', '###......+.', '###........', '###........', '###........', '.###.....#.', '.####...##.', '..#######..', '....###....'],
  star: ['.....#.....', '.....#.....', '....###....', '###########', '.#########.', '..#######..', '...#####...', '..###.###..', '..##...##..', '.##.....##.', '.#.......#.'],
  bolt: ['.....###', '....###.', '...###..', '..######', '.....##.', '....##..', '...##...', '..##....', '.#......'],
  boot: ['..#####..', '..#####..', '..#####..', '..#####..', '..######.', '..#######', '#########', '#########', '+++++++++'],
  eye: ['...#####...', '.##.....##.', '#...+++...#', '#..+++++..#', '#...+++...#', '.##.....##.', '...#####...'],
  mpage: ['#####..', '#...##.', '#.....#', '#.###.#', '#.....#', '#.###.#', '#######'],
  mheart: ['.##.##.', '#######', '#######', '.#####.', '..###..', '...#...'],
  msword: ['......#', '.....#.', '....#..', '.#.#...', '..#....', '.#.#...', '#......'],
  mspark: ['...#...', '...#...', '..###..', '#######', '..###..', '...#...', '...#...'],
  mbook: ['###.###', '#..#..#', '#..#..#', '#..#..#', '#..#..#', '###.###', '.##.##.'],
  mquill: ['.....##', '....###', '...###.', '..###..', '.###...', '##.....', '#......'],
  mskull: ['.#####.', '#######', '#..#..#', '#######', '.##.##.', '.#####.', '.#.#.#.'],
  mstar: ['...#...', '..###..', '#######', '.#####.', '..###..', '.##.##.', '.#...#.'],
  mshield: ['#######', '#######', '#######', '.#####.', '.#####.', '..###..', '...#...'],
};
const iconCache = {};
const isz = n => [ICONS[n][0].length, ICONS[n].length];
function icon(c, n, x, y, cols, sc = 1) {
  const key = n + '|' + cols.join(','); let cv = iconCache[key];
  if (!cv) {
    const [w, h] = isz(n); cv = document.createElement('canvas'); cv.width = w; cv.height = h; const g = cv.getContext('2d');
    ICONS[n].forEach((row, yy) => [...row].forEach((p, xx) => { const i = '#+='.indexOf(p); if (i >= 0 && cols[i] != null) { g.fillStyle = cols[i]; g.fillRect(xx, yy, 1, 1); } }));
    iconCache[key] = cv;
  }
  c.drawImage(cv, x, y, cv.width * sc, cv.height * sc);
}
const focusBox = (c, b) => { BOX(c, b.x, b.y, b.w, b.h, PAL.gold, 2); BOX(c, b.x + 2, b.y + 2, b.w - 4, b.h - 4, PAL.ink, 1); };
function iconBtn(b, ic, lines, o = {}) {
  b.draw = (c, st) => {
    let bg = o.bg || PAL.panel2, fg = o.fg || PAL.text, ic0 = o.ic || fg;
    if (st.p) { bg = PAL.text; fg = PAL.ink; ic0 = PAL.ink; }
    R(c, b.x, b.y, b.w, b.h, bg);
    const n = typeof ic === 'function' ? ic() : ic, [iw, ih] = isz(n), sc = o.sc || 1, lh = 7 * sc;
    const lw = Math.max(...lines.map(l => tw(l, sc))), x0 = b.x + ((b.w - iw - 5 - lw) >> 1);
    icon(c, n, x0, b.y + ((b.h - ih) >> 1), [ic0, o.ic1 === 'bg' ? bg : (st.p ? PAL.ink : o.ic1 || ic0), st.p ? PAL.ink : o.ic2 || ic0]);
    const th = lines.length * lh + (lines.length - 1) * 2, ty = b.y + ((b.h - th) >> 1);
    lines.forEach((l, i) => tx(c, l, x0 + iw + 5, ty + i * (lh + 2), fg, sc));
    if (st.f) focusBox(c, b);
  }; return b;
}
function plain(b, label, o = {}) {
  b.draw = (c, st) => {
    let bg = o.bg || PAL.panel2, fg = o.fg || PAL.text; if (st.p) { bg = PAL.text; fg = PAL.ink; }
    R(c, b.x, b.y, b.w, b.h, bg); const sc = o.sc || 1;
    txc(c, label, b.x, b.w, b.y + ((b.h - 7 * sc) >> 1), fg, sc); if (st.f) focusBox(c, b);
  }; return b;
}

// ---------- data + rules ----------
const AB = ['STR', 'DEX', 'CON', 'INT', 'WIS', 'CHA'];
const ABF = ['STRENGTH', 'DEXTERITY', 'CONSTITUTION', 'INTELLIGENCE', 'WISDOM', 'CHARISMA'];
const ORD = ['1ST', '2ND', '3RD', '4TH', '5TH', '6TH', '7TH', '8TH', '9TH'];
const SK = [['ACROBATICS', 1], ['ANIMAL HANDLING', 4], ['ARCANA', 3], ['ATHLETICS', 0], ['DECEPTION', 5], ['HISTORY', 3], ['INSIGHT', 4], ['INTIMIDATION', 5], ['INVESTIGATION', 3],
  ['MEDICINE', 4], ['NATURE', 3], ['PERCEPTION', 4], ['PERFORMANCE', 5], ['PERSUASION', 5], ['RELIGION', 3], ['SLEIGHT OF HAND', 1], ['STEALTH', 1], ['SURVIVAL', 4]];
const mod = s => Math.floor((s - 10) / 2);
const sg = n => (n >= 0 ? '+' : '-') + Math.abs(n);
const pb = k => 2 + Math.floor((k.level - 1) / 4);
const skMod = (k, i) => mod(k.ab[SK[i][1]]) + k.skill[i] * pb(k);
const svMod = (k, i) => mod(k.ab[i]) + (((k.saveProf >> i) & 1) ? pb(k) : 0);
const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
const NW = 4, WAB = ['STR', 'DEX', 'FINESSE', 'SPELL'], DIE = [4, 6, 8, 10, 12];
const DTYPE = ['SLASHING', 'PIERCING', 'BLUDGEONING', 'FIRE', 'COLD', 'LIGHTNING', 'THUNDER', 'ACID', 'POISON', 'NECROTIC', 'RADIANT', 'FORCE', 'PSYCHIC'];
const wAbIdx = (k, w) => w.ab === 0 ? 0 : w.ab === 1 ? 1 : w.ab === 2 ? (mod(k.ab[1]) > mod(k.ab[0]) ? 1 : 0) : (k.spellAb === 255 ? -1 : k.spellAb);
const wMod = (k, w) => { const i = wAbIdx(k, w); return i < 0 ? 0 : mod(k.ab[i]); };
const wAtk = (k, w) => wMod(k, w) + (w.prof ? pb(k) : 0) + w.bonus;
const wDmg = (k, w) => `${w.n}D${w.die}` + (w.dmg ? sg(w.dmg) : '');
const newWeapon = name => ({ name, ab: 0, prof: 1, bonus: 0, dmg: 0, n: 1, die: 8, type: 0, range: '5 FT', mastery: 0, props: '' });
// Weapon mastery properties (SRD 5.2). 0 = none. Effect text is an original short paraphrase that fits the 42-char strip.
const MASTERY = ['NONE', 'CLEAVE', 'GRAZE', 'NICK', 'PUSH', 'SAP', 'SLOW', 'TOPPLE', 'VEX'];
const masteryTag = (k, w) => w.mastery === 7 ? `TOPPLE DC ${8 + wMod(k, w) + pb(k)}` : w.mastery === 2 ? `GRAZE ${Math.max(0, wMod(k, w))}` : MASTERY[w.mastery];
const masteryText = (k, w) => [null, 'CLEAVE: ON HIT, ATTACK A 2ND FOE NEAR IT', `GRAZE: ON A MISS, STILL DEAL ${Math.max(0, wMod(k, w))} DAMAGE`, 'NICK: LIGHT EXTRA ATTACK, NO BONUS ACTION',
  'PUSH: ON HIT, SHOVE LARGE OR SMALLER 10 FT', 'SAP: ON HIT, TARGET HAS DIS ON NEXT ATTACK', 'SLOW: ON HIT, TARGET SPEED -10 FT', `TOPPLE: ON HIT, CON SAVE DC ${8 + wMod(k, w) + pb(k)} OR PRONE`, 'VEX: ON HIT, ADV ON YOUR NEXT ATTACK'][w.mastery];
const masteryDesc = (k, w) => {
  const m = wMod(k, w), dc = 8 + m + pb(k);
  return [null,
    'IF YOU HIT A CREATURE WITH A MELEE ATTACK, YOU CAN MAKE ONE MORE MELEE ATTACK AGAINST A SECOND CREATURE WITHIN 5 FEET OF THE FIRST AND WITHIN YOUR REACH. ON A HIT IT TAKES THE WEAPON DAMAGE WITHOUT YOUR ABILITY MODIFIER (UNLESS IT IS NEGATIVE).\nONCE PER TURN.',
    'IF YOUR ATTACK ROLL MISSES, THE TARGET STILL TAKES DAMAGE EQUAL TO YOUR ABILITY MODIFIER: ' + Math.max(0, m) + ' DAMAGE OF THE WEAPON\'S TYPE.\nONLY YOUR ABILITY MODIFIER CAN RAISE THIS DAMAGE.',
    'WHEN YOU MAKE THE EXTRA ATTACK FROM THE LIGHT PROPERTY, YOU CAN MAKE IT AS PART OF THE ATTACK ACTION INSTEAD OF USING A BONUS ACTION.\nONCE PER TURN.',
    'ON A HIT, YOU CAN PUSH THE CREATURE UP TO 10 FEET STRAIGHT AWAY FROM YOU IF IT IS LARGE OR SMALLER.',
    'ON A HIT, THE CREATURE HAS DISADVANTAGE ON ITS NEXT ATTACK ROLL BEFORE THE START OF YOUR NEXT TURN.',
    'ON A HIT THAT DEALS DAMAGE, YOU CAN REDUCE THE CREATURE\'S SPEED BY 10 FEET UNTIL THE START OF YOUR NEXT TURN.\nHITTING IT AGAIN DOESN\'T REDUCE IT BY MORE THAN 10 FEET.',
    'ON A HIT, YOU CAN FORCE THE CREATURE TO MAKE A CON SAVE, DC ' + dc + ' (8 + ABILITY MOD + PROF). ON A FAILURE IT FALLS PRONE.',
    'ON A HIT THAT DEALS DAMAGE, YOU HAVE ADVANTAGE ON YOUR NEXT ATTACK ROLL AGAINST THAT CREATURE BEFORE THE END OF YOUR NEXT TURN.'][w.mastery];
};
function newChar(name) { return { weapons: [], spells: [], feats: [], species: '', background: '', hitDie: 8, hdUsed: 0, name, cls: 'ADVENTURER', sub: '', level: 1, ab: [10, 10, 10, 10, 10, 10], saveProf: 0, skill: Array(18).fill(0), hp: 10, hpMax: 10, temp: 0, ac: 10, speed: 30, ds: 0, df: 0, slotMax: Array(9).fill(0), slotUsed: Array(9).fill(0), spellAb: 255, insp: 0 }; }
function sample() {
  const k = newChar('MAREN OAKHOLT'); Object.assign(k, { cls: 'CLERIC', sub: 'LIFE DOMAIN', level: 5, ab: [14, 10, 14, 10, 17, 12], saveProf: 48, hp: 31, hpMax: 38, ac: 18, spellAb: 4 });
  [6, 9, 13, 14].forEach(i => k.skill[i] = 1); k.slotMax.splice(0, 3, 4, 3, 2); k.slotUsed[0] = 1;
  k.weapons = [{ ...newWeapon('MACE'), die: 6, dmg: 2, type: 2, mastery: 5 }, { ...newWeapon('LT CROSSBOW'), ab: 1, type: 1, range: '80/320', mastery: 6, props: 'AMMUNITION, LOADING' }, { ...newWeapon('WARHAMMER'), die: 8, dmg: 2, type: 2, prof: 0, mastery: 4, props: 'VERSATILE (1D10)' }];
  k.feats = [{ name: 'CHANNEL DIVINITY', used: 0 }];
  k.hdUsed = 1;
  k.spells = ['SACRED FLAME', 'GUIDANCE', 'SPARE THE DYING', 'THAUMATURGY', 'BLESS*', 'CURE WOUNDS*', 'GUIDING BOLT*', 'HEALING WORD*', 'SHIELD OF FAITH', 'DETECT MAGIC',
    'AID*', 'LESSER RESTORATION*', 'SPIRITUAL WEAPON*', 'HOLD PERSON', 'MASS HEALING WORD', 'REVIVIFY*', 'SPIRIT GUARDIANS*'].map(n => ({ name: n.replace('*', ''), prep: n.endsWith('*') ? 1 : 0 }));
  return k;
}
function jedediah() {
  const k = newChar('JEDEDIAH GRAVES');
  Object.assign(k, { cls: 'RANGER', species: 'HUMAN (HALF-ELF)', background: 'WAYFARER', level: 2, ab: [10, 16, 14, 8, 16, 10], saveProf: 1 | 16, hp: 20, hpMax: 20, ac: 15, speed: 30, spellAb: 4, hitDie: 10 });
  [3, 6, 16, 17].forEach(i => k.skill[i] = 1); k.skill[15] = 2;
  k.slotMax[0] = 2;
  k.weapons = [{ ...newWeapon('LUCINDA'), ab: 1, bonus: 2, dmg: 3, die: 6, type: 1, range: '', mastery: 8, props: 'BARREL 6, BLARING, FAN FIRE' },
    { ...newWeapon('WHIP'), ab: 2, dmg: 3, die: 4, type: 0, range: '10 FT', mastery: 6, props: 'FINESSE, REACH' }];
  k.spells = ["HUNTER'S MARK", 'GOODBERRY', 'ENSNARING STRIKE', 'SILVERED SHELL', 'FIND FAMILIAR'].map(name => ({ name, prep: 1 }));
  k.feats = ['FAVORED ENEMY', 'DEFT EXPLORER', 'WEAPON MASTERY', 'ARCHERY', 'LUCKY', 'FAMILIAR FRIEND', 'RESOURCEFUL', 'BARREL', 'BLARING', 'FAN FIRE', 'MISFIRE', 'LANGUAGES', 'TOOLS'].map(name => ({ name, used: 0 }));
  return k;
}
const demoSlots = () => [jedediah(), sample(), null];
// Jedediah's portrait (user-supplied art) as a 48x48 24-bit BMP, written to portrait1.bmp on first run in the simulation only
const DEMO_PORTRAIT1 = 'Qk02GwAAAAAAADYAAAAoAAAAMAAAADAAAAABABgAAAAAAAAbAAAAAAAAAAAAAAAAAAAAAAAARlNaQktHOklBOk5LNUpHOUlCLTouIiseISogGiYiEyAaP1ZgYIieTWqGk8HjjMLlkMHojr/mcKHaB0anBUq0BlG8Bli+D021AE+1BFG7AVbADFi+DVOtDFi9CFm+EVi1IEuTZIu2h6zLZoeaYXmIcIubNUVQPUlJMzouNTcsKDAtRVBXOkZLSVdXSVdcLjw/ZnyIXmxsOkQ+OEU+MUM+Jzo0ITIqKzcsJy8fIisgDBoaJzZAKEFUfaK8lMHjksLjkcHmj8HmdaXdDUejBUi0AU28Cle+DkevAEy1BE+5AE65AFG5E1WvDVKzCla7DFK4CzmQNV6Wj7vcWXuRLkNReZKjTlxlO0A2Oj8uOkA0LzkyJTAxQUxOQ0tKR1RWTV1jYnuIL0FELDg4JjQ2ITIxIzg3ITQ2IzIvLDQtKjItDxwdGyUpKT5JgqnDjLvekL/iksHmlMHkgrHjKFyuBEiyAEy4BlC2DUKoAku0BUezA0WxBUu2BU+3AkenDVKxB1G1BzmaF0KHjLndkrXQKz9LPlRhQFNXLTYsOT0uR05BQ0o9KzIrNT8/UVtbVV5ZUFlWWW57Kj9EHTY4GjM0ITM1HzI4GS46HC40IzIxIjEwFCQkIi4xJTY+ID5QXISgiLTSkLzhlcDlj7rpR3jBBE2xAEu1Bk2xCkWpDFK5A0m0DEm1Bkm1ClS+DlW6D0yrDkylCDuiDz6KgbDak7rZLEBMFyQtKzY2PkM6ODsvO0E1TlZKPklCIjI0UWBkQUlDXGJXKjdAHC82Hzg8HTY8GC4+HTtcHEBpCiVFCyIyHC84HC02ER8kMT4/NkdFIzg9OlZohKrKjbvgkr/lcqHfDEWoAEu0CEywBESnCFm9Ble/BlG6Bk24AE25BFG+Dk+vBkWWDE2uHlarbaHTXomtgZiqOkZOFiInJC0rNz04OkA8P0tMVGNqQ1JaS1VaR01ILjQmChUXCBcfEiQzFTBPUoCxL26wCUGJDi1cGzBCHzNKDyA4FyMuJTEsJjUtMD07NUhSlbnVkb/nlMDofq/bFECUA0WmAj+kBkWnCVrBCVa4AEqqB1e/BVfCClXCEFO2FlGlBkagFVOtRXu3JVSCd5i3kam4Y3aKIC48HyowN0RJKDY+JjA6LzY3MjYtPkI1Pj8yCRAaCBUyECtfIFueL4PIF1CaAy5xESlVEyg5KT9TGiw+ER0gLTQtGiEcFSEniKa2lcPhj8DmkL/kjrzeMVOPCCyBFla0CE6sC1i+AkenAUGaCFa8AFnFA1nFBlvFEWDAEFewFlW0JWKwKGSmIEyIfqPHmL3fkLHOXHSIICw8GiQvND8/P0hAMzw1JiwfKS0dCA4gDCZTEEGSDVK1EF69CVuwFk6ZCithCiI3HSo6Aw4UAwcEFhcVDQ4SZHiHlcDhjcLmkMHjkL/kkcHedJjHG1WlDV2/B02vEFO2CkadADmMAle8AFnFAVjHAVnHAWDGDGG/CVSwG2W/MHvSDkahPmakkbvkkLngiqnNfo+tW2R3S09OWVtNZWpiS1JKKC8lDQ4dCyJOBT2TBVS1D2DGBlO2ClGxGUyYEzRcBw8jAgIHAwEDBgUFKi84nLnRj77ljb/mksDhkMDlirrcgq3TH06ZADuaBzmPCziHCixwAS53DVG6AVjHCljFFVe9IGC/HGO+F2G5E2G5IG/JFVW1Dzd8fpvBfpzBfZrCgpu/ipe2iIiUcGpfb2dZbGhfamtgwMLIRVqFDUWfC1a4AWHDBkuuBE+tCUqrFVqtID52BgkfBgMHCgkLZnaEj7PQh7DYf6fRjLHUjbHUgqjLfKTBIj51Ci2DEi91ABhLAxQ6ABhLC0OZClS6M2PATGm3VW6xUHK3R3K/Nma5HGK7Dl++FTx/bn+ehJm6gprAgpu+hZi9hZW1iomVeGdgdmZPdWZM7fH2X3ilCEelBla6D1/CAEamBEOXDTeTDVzBH2W/HUB+DxMmEhYfbYymd6bNc6LNeJjAeZCwWG6LeZKygqLELUVbDhtLFChcHkF6JE6OIkaIES91O1WdW2quZW6pZnCqZXCranOsWXG4JGK4El66MlWVZHGJhpi1g5u8gpu+g5q9g5a9jJS0iY6ZcGxicWJOuszdWH20DE6xA1e6D1q9A0GfADGFE0qjClq+FG7MDlmzJj1pU15yeZm7bZ7NcprKc4qyeJCySF54FSo/hKLBHC89CR5CI1ilGFy1GmXHFlu5PVyiWVqGYmefS2exM2G0QGevcHWsXHGzJF+1EFiySmSgZWZ6gZGpgpq5gpm8gpi7hpm/g5W8hpq6ipOhbGRdsMfgQWqoBVG0DVu9CEurC0WiDUWgCVu8C1W5AFOzCFq4HUOFcoOceY6wdJbEdZ/Nco61epi/fp6/T2FwLT5ULzxdEzt6DVGwBE+wEmDHLmbCWHCwTGmlJkKOEU6qDFzCK161ZnCuZXGxVW+4U22zaG6fa2N0hYidhZi6gJi/hZi8h5m8hJa5gpa7hZm6h46ip8DeJk+SAE2uBVGuDEOgBUCbB1CvD1/AAUepBkGnBlGzF0WRcIOifYemdouuf5rCe4qxfZzDhKfIPklTBgweEDVmBjR9EkObLFiuI2C1RWyxUWyyK2a9G1u3BkqtD1K6MF+0Ym6qaHOma3msbnWoaGSQaGF3eIGShJ7AiajRgpm+hZi6gJa9hJvEhqHMiqDGf5vDGUiZE1i7BUKWDDB/ByyADEOhBlOvD02rCkmnBVe3H06hrL7YtsDTmK3GiKTFh5mzjanBXnSICxEjEitWEzl7FDmGDz+PJ1meRnKpRmeSKVeWEmfLDWnND1/DAkyxJFm0SGWtYnGwanKtZmyrVVaQVFh8RFlxiaTKfpvDf5i9gZrCh57Fh53Gi6HKiaHDWHimEEqiA1W3Bz2PByJeAR1bACVyAzqLCT+QAzmKFGG6G1alwNPo6/X7yt3rqLvSuc3blqarIi42DiNHFlKxFFK0C0ywElW4HFu7G1uvJWOyHGrHEmjLInjXEmvIFWfIFFm4JV24L2G4NV+4JEykGjyLXHyrNUhldomxfImyfY62epG6fZG4eoyufIqliJKgbIWkFEmYC1m6E1WzFk+pFVOtDk6vClOrFUqUBylsA1CiGFeoj6jE7/P5vsTN6/H57fX3wNDVkajGIkiOAjqcBTGJE0WkCUevBUq1CVjDBV/JC2fOFGnOGXPUEGzOGm7SIWvMF1CvDlGyFVu/BD6XET6VaZLDY3ybe4ywg4mmjpWpjpajm5ecqZ2cqp+cpqirv83fMlWYAUWqCla+ElW5DlKyDUyoBUigD0agFT+VIFarCkONVHae5evym6Ck3uXo4+jpl6u9H0d1ED6RBy2UCDiQDFy8FV/IGVXADFnECV3IEGbNFGnQE2/QCGjKCWPLDWTJFlW3DUutEFq6CTd+JVGScJvKdpK6ipOnnpubp5+XrqOUsqSUtqeUtaeZrbS87fP7qb3ZH0KPCUCZD1mzDFiwF1iwFFi2ClK1CFSzCE+iCjN7O2CYvc/cc3V7sLa5sLW5LEVpEjuDATCUByuPBDSPCVm3AVu7AV2+BV7DBV3EAGHFB2fOD23PCGbKB2DIAV7IEFzAEEyvHF+5DDRoJUJogqvIkq/KlqGwo6Kcq6SVsKWYsaWXs6WYtaWYramo2eDp6e/1YXScDDyPB06yBU6uA0emAUWmBEaqB0+2DEqoFk+kIVSdLkBYHyYmSVFSRktNECxTDjOIASyZAC2PBzaQFEqdBD6RDFGpAEqtAFC3AlrBAFvGCWfNBWbLC2HLAFrGDFvBB0mtJmHACiVPQFFoscrV4/T6tsfZnaWvqqOcraSZsaWWs6SYtaWbtqOauNDi2+Drz9jlPFqPCkGeE1K1DVy8BVy9B1i/B1nCCFm4CFO6D0uUEyo/HSEaGyIZGyEXECYzBydZAyx/BTCEDjBsBS1ZDkF7BUWXB0ScAkmkAVO2AFfCAFvEAV3FCFvDCF/FDFe8BUmuLWG3CB0+W26BwNTc6/T13Or1nLfInKaxqqSfsKaZtaaXsqabtqeatM7nwdHm6uz0sr3TFD2ICVWzB1u/BlW8Ak65AEm4AlC1E1O7L1CWHiQrHiIUHiQVHCMTHCcZFyUtCyJFCSNQCBcvHTFNFTZuBkabBkihC0+rC1a7BFS6AVe9BFO8CVK7D2DCCFCrFU6sMFylCRkwOVJjkqez9/r69vv+3u76ucresbnBtbaztrCns6+lubWuwtDnu9DmytTp2d7lIj1xCzuTC0CaEFWuEF7BDFnEGVq7I1miRWeIIy0pGiIUHicZHysdHS4iHzAjGSghGigmGSkoEyIrETRiA0GPCkORBkSYDFCtB1ezCVq/DFe/D1a/FVe3CDJ8KFWdIkuEEiY6Fio2mKay+f39/f769v378vn98fX57PPy5+7t6O3t7fDvvtDmvtLlwdHm8vT7r7bHKUaDIV20IGK6IV2lJUp7JjlTIi0yICoiJCUbHSITGSIVHSodHC4nHTMrGDEpGzEsGzIsGTUtDSg9EzRdDi5ZJEV3ETl2EFOcE2XEEGC9DVGvF1ChCSRSJEh+Nm6yLFeNDycxxM7P+/39+/77+//7/f/8///+/////v7//P7/+v/9v9ftu9DlxNXm9/v69Pb2maW3eZSxNE1lNUNOICYkHCIbHCIUHyQPHyIUJy0eHSESHSMTGiQYHzAjFy0jDiIYFjArGzg2GDY2FzA1DiIpECMtFyo+EzFbKFidHFKYJ1mlEESJChwvGTlcIF2nGFedIFByzt7m+P3//v79+/79/P/+/v///////v///v7//f79x9vry9rm3ur0+v3++vz8+ff5x8/TNT8/KjElIygYGyMUGiIUGiEVHCEUHCIUHiEXHSAWHCEXGiQZGSUZFiETDB4UEismGzcxGDgxHjo1Fy4pEB0fGzBAHTtiGTxkHD5yDi1jDBIeFz1aCUJ5EkGFKmaeuNvo3fTz+fz9/f39/f/+/////v7+/v7+/f7///7+6/T79Pn7+Pv8/v/8+/7+9fv4oKGhOz80Skw7NDsqICYZHSITICEWIiETICUZJy0pLzk6Mjs6MDs3IS4kGiQXGiIVDyERDCQeGjg7GDs2HTszJzQxDh8aEiIuEiMxBhctFidMCQgdFTdeK1+ODTB2FVWifb3QwfHt8f39/f39/v7+/////////v///f7//v7+/f37/v7+/////v/9/P7+9Pf5bXJxNDo0P0Q7PkQ1PUM2HyMVHCEVHygpQlRcQFNgNk5dPVhkTWZ1XHaBPlZZNUREGiojFSMYEiQeFzkyFjkzHTYzGCchCBsgFyo3BQsaCRcbCgwOFS1SEzhoDTB7ElipR6nByefs+fz79/3+/v7//////////////v///P/////+/////////v/9/P7+8fT1UFVULzcxMzsxRk1ANzwvGyQQHSUhM0BPM0hXDyUwQlhhLkVHQlZcOVFbMlNhR2R1SF5lGSckFiQdGzYwHz48GDM2HCwhGiceEyAbBw8OChEPCBAPEh0uBxY5EUCBFmOiVpm16vP8/f38/f7+//7//////////////v///f///////////////v/+/f/97/L0PUREJi8sNTw0Qko+IicZHycYQlBPL0BNITVAQVVcNkhKGyoiIi4nJTg6RWhyJUtSO1ZiPFBaKjk7ESorHD4+ID9BHzYuGiohGikfFCcfCxkVDxkXCRQYBRowKVmLBTZZgJyp9fj99f78/v7+///////+/////////v///v///////////////////v7+7fDxJjAyFCAfMjo1O0I3ISUaHyYlUmBiFiclDiQiIzU5QE1MGyQbGSIWFikiIUFBHkQ/GztAKUFQSWFvPFdlG0BHIENDKkxIJUM9JD85JUE9FzIvDB4dCBgXBRgfECc4DiMth7i8qtjfz+js9fz7+v/9/P7//f7+/P7+/P/+//////////////////////7/8vT1OEBDDBgaHCciOUE4EhsTFSMpQlRXFycdPE1LLz9CMT04HiUcHCYaGi0kGDc0Gj04Gzc1GzM0JT5HWHeHWHmIKUtXIUVEKkxMLkxILUtGKUlFJD4+FSopCR4cBhoXCBgfMV5sW6Gqt93k8fr8+v39+//8/P/9/f7+/v///////////v///////v////7+9Pb3dH9/Aw8PBhIPDRkVEhwYJzpGKjxBHisgO0hFO0hJHSYgHSUcHSYaHS8nGjo3Gj06GDUvFzInHDYwHTc9X3yLZIiYLk9XHz9BL0tKL05KLU1JLUlHLklGLkdCKEE6FykrHERNO4GGV66tl9rZ3PX39fz9+/3//P7//v///////v7+//7///7///////7++vv8xczNCRMVBA8PAxAODRgXITM8GycsJCojHy4pPUdHHCEZHSIXGyMXEyUcGTgyGD06GTw4FzYwGDEnHjMsHzg5NlhgRmZwMU9YJkFCKUdEKUtHLExIK0pHKklGKEpDLEhDJ0pIIEZEHkNEI1FUUpSWnNvd4vn7+v39/f7+/v/+8fn4/P39//39/////v7+/P3+7/P1LDQ3AxAQAQ8QBA8QBhMWGh8eJDAnHy0qLzY0HSAXHSIQGyMSGCIVFykYHD02FT0/Gjw7FjQrFjMrFjErHTo8H0BIPlpnQFliHzxBKUNDLEZDK0VDKUREKEREK0VEJUJDIkBAKUFDKklGEjQ3KGlwhtHU4/r4+/37+P791eLr+/z+//3//////////v7+9fr5kZaYERkaBhAQBRITBxIVGR8aJi4jICsjJy4mHSIUHiMSHiMSHSAUHiQYGiwmHT06HT06FTIqHjsyKEQ/HDc4Hjs5HT09NldeQmNuK0tTJT9ALERDLEVEJ0JBKEJBKENBJ0JBJkA/JkJBJEA8EjAtHmdijtHR5/r7/fz9ztzk+vz+/v3//////////v/++fz79vn43OHgeYB/RlFRND0+HiIeHSEWGyIWHSAWHiMRHSIQJisbHCIWICQZIywiIzoxJEA4FjEpHTgyLUpJIj07GjErGTYuGjk5UW94XnuJRmVvKEFJLkVEK0Q/Jz87K0M+LkZCKkI+KkVAKUE7IT0wFC4lMHFxq+Xk9fr98ff7/f3//v7//v///v///////f39/P39+Pn59Pj36vDwiY+RICgjHSMYHyMYHSIUHSIRGiESGiQXHCgfHisiKjctIjMpJDgwHTQpGTQuKEVEHjgyGjEpGTIoGjIsHTUzK0VJUWpzTGJoIjQ0KTs2LT45KUA7KkM+K0Q/KkM+KEE9HTovHCkeCjUveMC87/v+/f7+///9/////////////////////v7+/v7+//7+9/f5hIuNGSMjDxcTFh0WGiQZDxoOCxUOCxYRDxsYHCcmGCQiIC4rHywpIDMsHTQyKT9BGTEpGTAnGDAnGjIpHDInHC4lHygoFB4eGyIcHiIYGiMXHCsgIDIpJjgwKDsyITUrGS8hHCUfJUVCls/O8vv9///+///+/////////////////////////////v7/+Pf6paitGB4nBBEXBRMTBxYSDRoXChUVCBESBxAPCBEQCBMTCBIVCRQWBxYXBhgcDR4gECUiGjAsGS8pGDAnITUpHiYcChUTDxoYHSIZHiMZGyMYHiUaHCMXHCMYHSUZHyQXLTgtc4J+xdja7Pj5/Pz9/////////////////////////////////////v7++vr68fHxsrrCKzhBBRcYBhMRBxMRBxIRBhQRDRkTCxcQBxIRBhIRBBERBhQTBRMRAhEPAhETDiIfGC8rGS8nGS4iFB0XBhwccZaSrbiyhZKNbX13dHt3RklGPUA9TU5Ll56a5Ojl8ff29/z5+/39/P7+/////////////////////////////////////v7+/Pz8+/z88vT1y9XXd4iJJDI0BhMWBRESAxEQAxIQCRUTExwXExsVDxkRDRcPBxMNBxIRBBAUBBUTFSolGi0lGSYdCxwWPHNxyenp9fj38/f28PX08/X08fLy7/Dv8/Pz9/n4+vr8/v3+/v/8/v/+/v/+/////////////////////////////////////////////v7+/fz++fj67PDxzdLTa3N0MDg5GyYmDBkaChMVEBsTFyIXHCIWGyIWGSEWHCQbEx0VDRkSGCcdIC4lECIYHEg/stfZ5/v6/f39/v3+/v3//f7//v///v/////9///+///////////+/////////////////////////////////////////////////////////////v7+/f7++vr77u7v19nbxMnLmZ+hV1pbJi0nJywlLjEnIigcJC4kIy8mITAmGy4hJzIpHS8oEjApYZyV4/X1+v3++/39/////////////////////////////////////////////////////////////////////////////////////////////////////////////f39/v39/Pv79fj38PPy6urq0dLQqKmngoKBZWlkVl5aRVVQOldQI1FGIEc/GEpEPoN8w+rm9Pv5/f38/P7+//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////7+/v7+/P39+/v7+vr6+Pj49vb28PPx8PDw6e7t1enlxODgutnZudraxOXl7vv5/fz8/P79/v///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////v7+/v7+/f39/f39/v7+/f39/P39/fz8+Pz7+fv79vr69fv79Pv8/P79//39/f7+//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////7+/f7+/P79/P7+/v7+/v7+/v3+/v7+/f/9/P/+/P///f//////////////////////////////////////////////////////';

// ---------- save format v5 (little-endian, 3760 bytes); v1-v4 are read and migrated ----------
const HDR = 16, NS = 3, VER = 5, RECV = { 1: 96, 2: 112, 3: 224, 4: 752, 5: 1248 }, MAXSP = 24, SPREC = 22, MAXF = 16, FREC = 22, HITDIE = [6, 8, 10, 12];
const LAYOUT = v => v >= 5 ? { WOFF: 112, WREC: 56, SPOFF: 336, FOFF: 864, IDOFF: 1216 } : { WOFF: 112, WREC: 28, SPOFF: 224 };
const SIZE = HDR + RECV[VER] * NS;
const DIR = '/data/pocketsheet/', SAV = DIR + 'sheet.sav', BAK = DIR + 'sheet.bak', TMP = DIR + 'sheet.tmp', BAD = DIR + 'sheet.bad';
const PORTRAIT = i => DIR + `portrait${i + 1}.bmp`;

// ---------- spell library: built-in starter set + fat:/data/pocketsheet/spells.txt (user-edited on a PC) ----------
// Descriptions are original short paraphrases (2024-style numbers). Players should check their own rulebook.
const SPELLS_TXT = DIR + 'spells.txt';
const STARTER = [
  ['FIRE BOLT', 0, 'EVOCATION', '1 ACTION', '120 FT', 'V, S', 'INSTANT', '', 'Hurl a mote of fire at a creature or object. Make a ranged spell attack. On a hit it takes 1d10 fire damage, and an unattended flammable object ignites.\nDamage rises by 1d10 at character levels 5, 11 and 17.'],
  ['SACRED FLAME', 0, 'EVOCATION', '1 ACTION', '60 FT', 'V, S', 'INSTANT', '', 'Radiance descends on a creature you can see. It makes a DEX save or takes 1d8 radiant damage. Cover gives no bonus to this save.\nDamage rises by 1d8 at character levels 5, 11 and 17.'],
  ['GUIDANCE', 0, 'DIVINATION', '1 ACTION', 'TOUCH', 'V, S', '1 MINUTE', 'C', 'Touch a willing creature and choose a skill. Until the spell ends, it adds 1d4 to ability checks that use that skill.'],
  ['LIGHT', 0, 'EVOCATION', '1 ACTION', 'TOUCH', 'V, M', '1 HOUR', '', 'An object you touch sheds bright light in a 20-foot radius and dim light for another 20 feet. Covering it with something opaque blocks the light. Casting again ends the earlier light.'],
  ['MAGE HAND', 0, 'CONJURATION', '1 ACTION', '30 FT', 'V, S', '1 MINUTE', '', 'A spectral floating hand appears. As a magic action you can move it up to 30 feet and use it to handle an object, open an unlocked door or container, or stow or retrieve an item. It cannot attack, activate magic items or carry more than 10 pounds.'],
  ['SPARE THE DYING', 0, 'NECROMANCY', '1 ACTION', '15 FT', 'V, S', 'INSTANT', '', 'Choose a creature at 0 hit points that is not dead. It becomes stable.\nThe range doubles at character levels 5, 11 and 17.'],
  ['THAUMATURGY', 0, 'TRANSMUTATION', '1 ACTION', '30 FT', 'V', '1 MINUTE', '', 'Create a minor wonder: your voice booms, flames flicker or change color, the ground trembles harmlessly, a sound or a door bursts open, or your eyes change. Up to three one-minute effects can be active at once.'],
  ['PRESTIDIGITATION', 0, 'TRANSMUTATION', '1 ACTION', '10 FT', 'V, S', '1 HOUR', '', 'A small magic trick: a harmless sensory effect, light or snuff a small flame, clean or soil a small object, chill, warm or flavor food, make a small mark appear, or create a trinket-like illusion that lasts until the end of your next turn.'],
  ['ELDRITCH BLAST', 0, 'EVOCATION', '1 ACTION', '120 FT', 'V, S', 'INSTANT', '', 'A beam of crackling energy streaks at a creature or object. Make a ranged spell attack. On a hit the target takes 1d10 force damage.\nYou fire 2 beams at character level 5, 3 at level 11 and 4 at level 17, each with its own attack roll.'],
  ['RAY OF FROST', 0, 'EVOCATION', '1 ACTION', '60 FT', 'V, S', 'INSTANT', '', 'A frigid beam strikes a creature. Make a ranged spell attack. On a hit it takes 1d8 cold damage and its speed drops by 10 feet until the start of your next turn.\nDamage rises by 1d8 at character levels 5, 11 and 17.'],
  ['BLESS', 1, 'ENCHANTMENT', '1 ACTION', '30 FT', 'V, S, M', '1 MINUTE', 'C', 'Up to three creatures you choose add 1d4 to their attack rolls and saving throws while the spell lasts.\nHigher slots: one more creature for each slot level above 1st.'],
  ['CURE WOUNDS', 1, 'ABJURATION', '1 ACTION', 'TOUCH', 'V, S', 'INSTANT', '', 'A creature you touch regains 2d8 + your spellcasting modifier hit points.\nHigher slots: +2d8 for each slot level above 1st.'],
  ['DETECT MAGIC', 1, 'DIVINATION', '1 ACTION', 'SELF', 'V, S', '10 MINUTES', 'CR', 'You sense magic within 30 feet of you. If you sense it, you can take an action to see a faint aura around any visible creature or object that bears magic and learn its school of magic.\nBlocked by 1 foot of stone, dirt or wood, 1 inch of metal, or a thin sheet of lead.'],
  ['GUIDING BOLT', 1, 'EVOCATION', '1 ACTION', '120 FT', 'V, S', '1 ROUND', '', 'A flash of light streaks toward a creature. Make a ranged spell attack. On a hit it takes 4d6 radiant damage, and the next attack roll against it before the end of your next turn has advantage.\nHigher slots: +1d6 for each slot level above 1st.'],
  ['HEALING WORD', 1, 'ABJURATION', '1 BONUS ACTION', '60 FT', 'V', 'INSTANT', '', 'A creature you can see regains 2d4 + your spellcasting modifier hit points.\nHigher slots: +2d4 for each slot level above 1st.'],
  ['MAGIC MISSILE', 1, 'EVOCATION', '1 ACTION', '120 FT', 'V, S', 'INSTANT', '', 'Three glowing darts strike creatures you can see in range, without an attack roll. Each dart deals 1d4 + 1 force damage. Aim them at one target or several.\nHigher slots: one more dart for each slot level above 1st.'],
  ['SHIELD', 1, 'ABJURATION', '1 REACTION', 'SELF', 'V, S', '1 ROUND', '', 'Cast when you are hit by an attack roll or targeted by Magic Missile. Until the start of your next turn you gain +5 AC, including against the triggering attack, and take no damage from Magic Missile.'],
  ['SHIELD OF FAITH', 1, 'ABJURATION', '1 BONUS ACTION', '60 FT', 'V, S, M', '10 MINUTES', 'C', 'A shimmering field surrounds a creature you choose, giving it +2 AC while the spell lasts.'],
  ['SLEEP', 1, 'ENCHANTMENT', '1 ACTION', '60 FT', 'V, S, M', '1 MINUTE', 'C', 'Each creature you choose in a 5-foot-radius sphere makes a WIS save. On a failure it is incapacitated until the end of its next turn, then repeats the save. If it fails again it is unconscious for the duration, or until it takes damage or someone uses an action to shake it awake. Creatures that do not sleep or are immune to exhaustion are unaffected.'],
  ['THUNDERWAVE', 1, 'EVOCATION', '1 ACTION', 'SELF', 'V, S', 'INSTANT', '', 'Each creature in a 15-foot cube from you makes a CON save. On a failure it takes 2d8 thunder damage and is pushed 10 feet away. On a success it takes half damage and is not pushed. The boom is audible 300 feet away.\nHigher slots: +1d8 for each slot level above 1st.'],
  ['MAGE ARMOR', 1, 'ABJURATION', '1 ACTION', 'TOUCH', 'V, S, M', '8 HOURS', '', 'A willing creature not wearing armor gets a base AC of 13 + its DEX modifier. The spell ends early if it dons armor or you dismiss it.'],
  ['BURNING HANDS', 1, 'EVOCATION', '1 ACTION', 'SELF', 'V, S', 'INSTANT', '', 'Flames shoot from your hands. Each creature in a 15-foot cone makes a DEX save, taking 3d6 fire damage on a failure or half on a success. Unattended flammable objects in the cone ignite.\nHigher slots: +1d6 for each slot level above 1st.'],
  ['CHARM PERSON', 1, 'ENCHANTMENT', '1 ACTION', '30 FT', 'V, S', '1 HOUR', '', 'A humanoid you can see makes a WIS save, with advantage if you or your allies are fighting it. On a failure it is charmed by you until the spell ends or you or your allies harm it. It knows it was charmed when the spell ends.\nHigher slots: one more creature for each slot level above 1st.'],
  ['AID', 2, 'ABJURATION', '1 ACTION', '30 FT', 'V, S, M', '8 HOURS', '', 'Up to three creatures each increase their hit point maximum and current hit points by 5 for the duration.\nHigher slots: +5 for each slot level above 2nd.'],
  ['HOLD PERSON', 2, 'ENCHANTMENT', '1 ACTION', '60 FT', 'V, S, M', '1 MINUTE', 'C', 'A humanoid you can see makes a WIS save or is paralyzed for the duration. At the end of each of its turns it repeats the save, ending the spell on itself on a success.\nHigher slots: one more humanoid for each slot level above 2nd.'],
  ['LESSER RESTORATION', 2, 'ABJURATION', '1 BONUS ACTION', 'TOUCH', 'V, S', 'INSTANT', '', 'Touch a creature to end one condition on it: blinded, deafened, paralyzed or poisoned.'],
  ['MISTY STEP', 2, 'CONJURATION', '1 BONUS ACTION', 'SELF', 'V', 'INSTANT', '', 'Briefly wrapped in silvery mist, you teleport up to 30 feet to an unoccupied space you can see.'],
  ['SPIRITUAL WEAPON', 2, 'EVOCATION', '1 BONUS ACTION', '60 FT', 'V, S', '1 MINUTE', 'C', 'A floating spectral force appears within range. When you cast it, make a melee spell attack against a creature within 5 feet of it, dealing 1d8 + your spellcasting modifier force damage on a hit. On later turns, as a bonus action, you can move it up to 20 feet and repeat the attack.\nHigher slots: +1d8 for each slot level above 2nd.'],
  ['INVISIBILITY', 2, 'ILLUSION', '1 ACTION', 'TOUCH', 'V, S, M', '1 HOUR', 'C', 'A creature you touch becomes invisible, along with what it wears and carries, until the spell ends. The spell ends early for the target if it attacks, deals damage or casts a spell.\nHigher slots: one more creature for each slot level above 2nd.'],
  ['SCORCHING RAY', 2, 'EVOCATION', '1 ACTION', '120 FT', 'V, S', 'INSTANT', '', 'You hurl three fiery rays at targets in range, one or several. Make a ranged spell attack for each ray. Each hit deals 2d6 fire damage.\nHigher slots: one more ray for each slot level above 2nd.'],
  ['COUNTERSPELL', 3, 'ABJURATION', '1 REACTION', '60 FT', 'S', 'INSTANT', '', 'Cast when you see a creature within 60 feet casting a spell with components. It makes a CON save. On a failure the spell dissipates with no effect, and the action and slot it used are wasted. (Check your rules version: older rules use an ability check instead.)'],
  ['DISPEL MAGIC', 3, 'ABJURATION', '1 ACTION', '120 FT', 'V, S', 'INSTANT', '', 'Choose a creature, object or magical effect. Any ongoing spell of 3rd level or lower on it ends. For each higher-level spell on it, make an ability check with your spellcasting ability, DC 10 + that spell\'s level. On a success, that spell ends.\nHigher slots: automatically end spells of the slot\'s level or lower.'],
  ['FIREBALL', 3, 'EVOCATION', '1 ACTION', '150 FT', 'V, S, M', 'INSTANT', '', 'A bright streak flashes to a point you choose and blossoms into an explosion. Each creature in a 20-foot-radius sphere makes a DEX save, taking 8d6 fire damage on a failure or half on a success. Unattended flammable objects in the area ignite.\nHigher slots: +1d6 for each slot level above 3rd.'],
  ['MASS HEALING WORD', 3, 'ABJURATION', '1 BONUS ACTION', '60 FT', 'V', 'INSTANT', '', 'Up to six creatures you can see each regain 2d4 + your spellcasting modifier hit points.\nHigher slots: +1d4 for each slot level above 3rd.'],
  ['REVIVIFY', 3, 'NECROMANCY', '1 ACTION', 'TOUCH', 'V, S, M', 'INSTANT', '', 'Touch a creature that has died within the last minute. It returns to life with 1 hit point. This cannot revive a creature that died of old age.\nMaterial: a diamond worth 300+ gp, which the spell consumes.'],
  ['SPIRIT GUARDIANS', 3, 'CONJURATION', '1 ACTION', 'SELF', 'V, S, M', '10 MINUTES', 'C', 'Protective spirits fill a 15-foot emanation around you. Creatures you choose are unaffected. Others have their speed halved in it. When a creature enters the area for the first time on a turn, or ends its turn there, it makes a WIS save, taking 3d8 radiant or necrotic damage (your choice when casting) on a failure or half on a success.\nHigher slots: +1d8 for each slot level above 3rd.'],
  ['FLY', 3, 'TRANSMUTATION', '1 ACTION', 'TOUCH', 'V, S, M', '10 MINUTES', 'C', 'A willing creature gains a flying speed of 60 feet and can hover. If the spell ends while it is aloft, it falls unless it can stop the fall.\nHigher slots: one more creature for each slot level above 3rd.'],
  ['BANISHMENT', 4, 'ABJURATION', '1 ACTION', '30 FT', 'V, S, M', '1 MINUTE', 'C', 'A creature you can see makes a CHA save or is sent to a harmless demiplane, where it is incapacitated. It returns to the space it left when the spell ends. If it is not native to your plane and the spell lasts the full minute, it goes home instead.\nHigher slots: one more creature for each slot level above 4th.'],
  ['DEATH WARD', 4, 'ABJURATION', '1 ACTION', 'TOUCH', 'V, S', '8 HOURS', '', 'The first time the target would drop to 0 hit points, it drops to 1 hit point instead and the spell ends.'],
  ['MASS CURE WOUNDS', 5, 'ABJURATION', '1 ACTION', '60 FT', 'V, S', 'INSTANT', '', 'Up to six creatures in a 30-foot-radius sphere each regain 5d8 + your spellcasting modifier hit points.\nHigher slots: +1d8 for each slot level above 5th.'],
  ['CONE OF COLD', 5, 'EVOCATION', '1 ACTION', 'SELF', 'V, S, M', 'INSTANT', '', 'Frigid air blasts from your hands. Each creature in a 60-foot cone makes a CON save, taking 8d8 cold damage on a failure or half on a success. A creature killed by this spell becomes a frozen statue until it thaws.\nHigher slots: +1d8 for each slot level above 5th.'],
  ["HUNTER'S MARK", 1, 'DIVINATION', '1 BONUS ACTION', '90 FT', 'V', '1 HOUR', 'C', 'You mark a creature you can see as your quarry. Whenever you hit it with an attack roll, it takes an extra 1d6 force damage. You also have advantage on WIS (Perception or Survival) checks to find it.\nIf it drops to 0 HP before the spell ends, you can use a bonus action on a later turn to move the mark to a new creature you can see.\nHigher slots: concentration lasts up to 8 hours with a 3rd or 4th level slot, 24 hours with 5th or higher.'],
  ['GOODBERRY', 1, 'CONJURATION', '1 ACTION', 'SELF', 'V, S, M', '24 HOURS', '', 'Ten magic berries appear in your hand. A creature can use a bonus action to eat one, regaining 1 hit point. One berry is a full day of nourishment.\nUneaten berries disappear when the spell ends.'],
  ['ENSNARING STRIKE', 1, 'CONJURATION', '1 BONUS ACTION', 'SELF', 'V', '1 MINUTE', 'C', 'Cast right after you hit a creature with a weapon. Grasping vines appear and it makes a STR save (Large or larger creatures have advantage). On a failure it is restrained until the spell ends. On a success the vines wither.\nWhile restrained it takes 1d6 piercing damage at the start of each of its turns. It or a creature within reach can use an action to try a STR (Athletics) check against your spell save DC, ending the spell on a success.\nHigher slots: +1d6 for each slot level above 1st.'],
  ['FIND FAMILIAR', 1, 'CONJURATION', '1 HOUR', '10 FT', 'V, S, M', 'INSTANT', 'R', 'You summon a spirit that takes an animal form you choose, such as a bat, cat, frog, hawk, lizard, octopus, owl, rat, raven, spider or weasel. It acts on its own initiative, obeys your commands, and cannot attack, but it can take other actions.\nWithin 100 feet you can communicate telepathically, and as a bonus action you can see and hear through its senses. When you cast a touch spell, the familiar can deliver it.\nIf it drops to 0 HP it vanishes. You can dismiss it to a pocket dimension and recall it. Only one familiar at a time.\nMaterial: incense worth 10+ gp, consumed.'],
];
// ---------- feature library: built-in starter set + fat:/data/pocketsheet/features.txt ----------
const FEATURES_TXT = DIR + 'features.txt';
const FCAT = ['CLASS', 'SUBCLASS', 'SPECIES', 'BACKGROUND', 'FEAT', 'ITEM', 'OTHER'], FCAT3 = ['CLS', 'SUB', 'SPC', 'BKG', 'FT', 'ITM', 'OTH'];
const RESETS = { LONG: 'LONG REST', SHORT: 'SHORT REST', SHORT1: '1 PER SHORT', NONE: 'MANUAL' };
const FSTARTER = [
  ['FAVORED ENEMY', 'CLASS', '2', 'LONG', '', "You always have Hunter's Mark prepared. You can cast it without a spell slot a number of times set by your Ranger level (2 at levels 1-4, rising later), and you regain all uses on a long rest."],
  ['DEFT EXPLORER', 'CLASS', '', '', '', 'Choose one skill you are proficient in: you gain expertise in it. You also learn two languages of your choice.'],
  ['WEAPON MASTERY', 'CLASS', '', '', '', 'You can use the mastery property of a set number of weapon kinds you are proficient with. When you finish a long rest you can swap one choice.'],
  ['ARCHERY', 'FEAT', '', '', '', 'Fighting style. You gain a +2 bonus to attack rolls you make with ranged weapons.\nPocket Sheet does not add this automatically: put it in the weapon HIT BONUS.'],
  ['LUCKY', 'FEAT', 'PB', 'LONG', '', 'You have Luck Points equal to your proficiency bonus, regained on a long rest.\nAdvantage: when you roll a d20 test, spend 1 to give yourself advantage on it.\nDisadvantage: when a creature rolls a d20 to attack you, spend 1 to give it disadvantage.'],
  ['RESOURCEFUL', 'SPECIES', '', '', 'INSPIRE', 'You gain Heroic Inspiration whenever you finish a long rest. Pocket Sheet sets the inspiration star for you on a long rest.'],
  ['SKILLFUL', 'SPECIES', '', '', '', 'You gain proficiency in one skill of your choice.'],
  ['CHANNEL DIVINITY', 'CLASS', '2', 'SHORT1', '', 'Channel divine energy to fuel effects such as Divine Spark and Turn Undead. You regain one use on a short rest and all uses on a long rest.'],
  ['SECOND WIND', 'CLASS', '2', 'SHORT1', '', 'As a bonus action, regain hit points equal to 1d10 + your Fighter level. You regain one use on a short rest and all uses on a long rest.'],
  ['RAGE', 'CLASS', '2', 'SHORT1', '', 'As a bonus action, enter a rage: resistance to bludgeoning, piercing and slashing damage, a bonus to STR damage, and advantage on STR checks and saves. You regain one use on a short rest and all uses on a long rest.'],
  ['BARDIC INSPIRATION', 'CLASS', 'CHA', 'LONG', '', 'As a bonus action, give a creature within 60 feet a Bardic Inspiration die it can add to one d20 test in the next hour. Uses equal your CHA modifier (minimum 1).'],
];
const mkFeat = ([name, cat, uses, reset, tags, desc], src) => ({ name, cat: Math.max(0, FCAT.indexOf(cat)), uses: uses || '', reset: reset || (uses ? 'LONG' : ''), inspire: /INSPIRE/.test(tags), desc: cleanText(desc), src: src || 'BUILT-IN' });
const featMax = (k, f) => !f || !f.uses ? 0 : f.uses === 'PB' ? pb(k) : AB.includes(f.uses) ? Math.max(1, mod(k.ab[AB.indexOf(f.uses)])) : Math.min(20, +f.uses || 0);
const FEATURES_TEMPLATE = ['# POCKET SHEET FEATURE LIBRARY', '# Class features, feats, species traits, item properties and notes. Edit on a PC.', '# Entries here are added to the built-in list. One with the same name as a built-in entry replaces it.',
  '# Each entry starts with @NAME (up to 20 characters), then optional fields, then the description.', '# category: class, subclass, species, background, feat, item or other', '# uses: a number (1-20), PB (proficiency bonus) or an ability (STR, DEX, CON, INT, WIS, CHA). Leave out for no tracker.',
  '# reset: long, short, short1 (one use back per short rest, all on a long rest) or none (you reset it yourself)', '# tags: inspire (a long rest grants Heroic Inspiration)', '# Weapon properties: name an entry after a property (e.g. @REACH) and weapons with it show the text.', ''].join('\n');
const DEMO_FEATURES = ['', '# ---- Jedediah Graves (copied from his paper sheet) ----',
  '@FAMILIAR FRIEND', 'category: feat',
  "You always have Find Familiar prepared. You can cast it without a slot or material components, and regain that use on a long rest. You can also cast it with your spell slots. Your familiar's HP max and current HP rise by twice your character level.", '',
  'When you make an ability check with a skill you are proficient in while your familiar is within 5 feet, you gain advantage. You can do this a number of times equal to your proficiency bonus per long rest.', '',
  '@BARREL', 'category: item', 'uses: 6', 'reset: none',
  'Lucinda holds 6 shots. Reloading takes an action, or one attack. Spend a pip each shot. Y gives one back, RELOAD in the details refills it.', '',
  '@BLARING', 'category: item',
  'The shot can be heard up to 100 feet away. You have disadvantage on stealth checks until the start of your next turn, and creatures in range have advantage on perception checks to find you.', '',
  '@FAN FIRE', 'category: item',
  'Fire a number of times equal to your DEX modifier, with a cumulative -3 penalty on each attack. Your speed becomes 0. Only add your damage modifier to the first attack.', '',
  '@MISFIRE', 'category: item',
  "If you roll a 1 on an attack roll, the weapon jams and can't be used until repaired. Use an action for a DC 15 DEX (Sleight of Hand) check to repair it, or spend 1 hour mending it to succeed automatically.", '',
  "If the check fails, the weapon is broken and must be mended at a specialist's shop for a quarter of the firearm's cost.", '',
  '@LANGUAGES', 'category: other', "Common, Elvish, Celestial, Thieves' Cant.", '',
  '@TOOLS', 'category: other', "Thieves' Tools.", '',
  '@REACH', 'category: item', 'This weapon adds 5 feet to your reach when you attack with it, and when you work out your reach for opportunity attacks.', '',
  '@FINESSE', 'category: item', 'Use your choice of STR or DEX for the attack and damage rolls. Pocket Sheet picks the higher modifier for the to-hit.', ''].join('\n');
const DEMO_SPELLS = ['', '# ---- Jedediah Graves ----', '@SILVERED SHELL', 'level: 1', 'school: transmutation', 'time: 1 bonus action', 'range: self', 'components: V, S, M', 'duration: 1 minute',
  'You touch a weapon and coat it in otherworldly bile. For the duration it becomes moongilded, and while you hold it, it deals an extra 1d4 force damage on a hit.', '',
  'Higher slots: the duration grows to 10 minutes with a 2nd level slot, 1 hour with 3rd, 8 hours with 4th and 24 hours with 5th or higher.', '', 'Material: an empty phantasm shell.', ''].join('\n');
function parseFeaturesTxt(text) {
  const out = []; let cur = null, skipped = 0;
  const finish = () => {
    if (!cur) return; const desc = cur.lines.join(' ').replace(/ *\n */g, '\n').replace(/ +/g, ' ').trim().slice(0, 1200);
    if (cur.name && cur.name.length <= 20 && NAME_OK.test(cur.name) && !cur.bad) out.push({ name: cur.name, cat: cur.cat, uses: cur.uses, reset: cur.reset || (cur.uses ? 'LONG' : ''), inspire: cur.inspire, desc: desc || 'NO DESCRIPTION.', src: 'FEATURES.TXT' }); else skipped++; cur = null;
  };
  for (const raw of text.split(/\r?\n/)) {
    const line = raw.trim(); if (line.startsWith('#')) continue;
    if (line.startsWith('@')) { finish(); cur = { name: cleanText(line.slice(1)).replace(/\n/g, '').trim(), cat: 6, uses: '', reset: '', inspire: false, lines: [], body: false, bad: false }; continue; }
    if (!cur) continue;
    const m = !cur.body && /^([a-z0-9]+)\s*:\s*(.*)$/i.exec(line), key = m && m[1].toLowerCase();
    if (m && ['category', 'uses', 'reset', 'tags'].includes(key)) {
      const v = cleanText(m[2]).trim();
      if (key === 'category') { const i = FCAT.findIndex(c => c.startsWith(v.slice(0, 3))); if (i < 0) cur.bad = true; else cur.cat = i; }
      else if (key === 'uses') { if (/^([1-9]|1[0-9]|20)$/.test(v) || v === 'PB' || AB.includes(v)) cur.uses = v; else cur.bad = true; }
      else if (key === 'reset') { const r = v.replace(/\s/g, ''); if (RESETS[r]) cur.reset = r; else cur.bad = true; }
      else cur.inspire = /INSPIRE/.test(v);
      continue;
    }
    if (!line) { if (cur.body) cur.lines.push('\n'); continue; }
    cur.body = true; cur.lines.push(cleanText(line));
  }
  finish(); return { feats: out.slice(0, 200), skipped };
}
const cleanText = s => String(s).toUpperCase().replace(/[\u2018\u2019"\u201C\u201D]/g, "'").replace(/[\u2013\u2014]/g, '-').replace(/;/g, ',').replace(/[^A-Z0-9 '.,:\-+\/()!?&=<>\n]/g, '');
const mkSpell = ([name, level, school, time, range, comp, dur, tags, desc]) => ({ name, level, school, time, range, comp, dur, conc: tags.includes('C'), ritual: tags.includes('R'), desc: cleanText(desc), src: 'BUILT-IN' });
const SPELLS_TEMPLATE = ['# POCKET SHEET SPELL LIBRARY', '# Edit this file on a PC, then put the card back in the flashcart.', '# Spells here are added to the built-in starter list. One with the same name as a built-in spell replaces it.',
  '# Each spell starts with a line @NAME (up to 20 characters). Then optional fields, then the description.', '# Fields: level (C for cantrip, or 1-9, required), school, time, range, components, duration, tags (concentration ritual).',
  '# A blank line in the description starts a new paragraph. Use plain ASCII. Everything shows in capitals.', '#', '# Example (remove the # signs to use it):', '# @MY HOMEBREW BOLT', '# level: 2', '# school: evocation', '# time: 1 action', '# range: 90 ft', '# components: V, S',
  '# duration: instant', '# tags:', '# A crackling bolt hits one creature you can see for 3d6 lightning damage.', ''].join('\n');
function parseSpellsTxt(text) {
  const out = []; let cur = null, skipped = 0;
  const KEYS = { level: 'level', school: 'school', time: 'time', range: 'range', components: 'comp', duration: 'dur', tags: 'tags' };
  const finish = () => {
    if (!cur) return; const desc = cur.lines.join(' ').replace(/ *\n */g, '\n').replace(/ +/g, ' ').trim().slice(0, 1500);
    if (cur.name && cur.name.length <= 20 && NAME_OK.test(cur.name) && cur.level >= 0) out.push({ name: cur.name, level: cur.level, school: cur.school, time: cur.time, range: cur.range, comp: cur.comp, dur: cur.dur, conc: cur.conc, ritual: cur.ritual, desc: desc || 'NO DESCRIPTION.', src: 'SPELLS.TXT' });
    else skipped++; cur = null;
  };
  for (const raw of text.split(/\r?\n/)) {
    const line = raw.trim(); if (line.startsWith('#')) continue;
    if (line.startsWith('@')) { finish(); cur = { name: cleanText(line.slice(1)).replace(/\n/g, '').trim(), level: -1, school: '', time: '', range: '', comp: '', dur: '', conc: false, ritual: false, lines: [], body: false }; continue; }
    if (!cur) continue;
    const m = !cur.body && /^([a-z]+)\s*:\s*(.*)$/i.exec(line);
    if (m && KEYS[m[1].toLowerCase()]) {
      const key = KEYS[m[1].toLowerCase()], v = cleanText(m[2]).trim();
      if (key === 'level') cur.level = /^(C|CANTRIP|0)$/.test(v) ? 0 : /^[1-9]$/.test(v) ? +v : -1;
      else if (key === 'tags') { cur.conc = /CONC/.test(v); cur.ritual = /RITUAL/.test(v); }
      else cur[key] = v.slice(0, 20);
      continue;
    }
    if (!line) { if (cur.body) cur.lines.push('\n'); continue; }
    cur.body = true; cur.lines.push(cleanText(line));
  }
  finish(); return { spells: out.slice(0, 300), skipped };
}
const decodeText = b => new TextDecoder('utf-8').decode(b);
const NAME_OK = /^[A-Z0-9 '.\-&\/()]*$/, TEXT_OK = /^[A-Z0-9 '.,\-&\/()+!?:]*$/;
function crc32(b) { let crc = 0xFFFFFFFF; for (let i = 0; i < b.length; i++) { let c = (crc ^ b[i]) & 0xFF; for (let k = 0; k < 8; k++) c = c & 1 ? (c >>> 1) ^ 0xEDB88320 : c >>> 1; crc = (crc >>> 8) ^ c; } return (crc ^ 0xFFFFFFFF) >>> 0; }
const putStr = (b, o, s, n) => { for (let i = 0; i < n; i++) b[o + i] = i < s.length ? s.charCodeAt(i) : 0; };
const getStr = (b, o, n) => { let s = ''; for (let i = 0; i < n && b[o + i]; i++) s += String.fromCharCode(b[o + i]); return s; };
function serialize(slots, last) {
  const REC = RECV[VER], { WOFF, WREC, SPOFF, FOFF, IDOFF } = LAYOUT(VER), b = new Uint8Array(SIZE), d = new DataView(b.buffer);
  b.set([80, 75, 83, 72], 0); d.setUint16(4, VER, true); b[6] = NS; b[7] = last;
  slots.forEach((k, i) => {
    if (!k) return; const o = HDR + i * REC; b[o] = 1;
    putStr(b, o + 1, k.name, 16); putStr(b, o + 17, k.cls, 12); b[o + 29] = k.level;
    k.ab.forEach((v, j) => b[o + 30 + j] = v); b[o + 36] = k.saveProf; k.skill.forEach((v, j) => b[o + 37 + j] = v);
    d.setInt16(o + 55, k.hp, true); d.setUint16(o + 57, k.hpMax, true); d.setUint16(o + 59, k.temp, true);
    b[o + 61] = k.ac; b[o + 62] = k.speed; b[o + 63] = k.ds; b[o + 64] = k.df;
    k.slotMax.forEach((v, j) => b[o + 65 + j] = v); k.slotUsed.forEach((v, j) => b[o + 74 + j] = v);
    b[o + 83] = k.spellAb; b[o + 84] = k.insp; putStr(b, o + 85, k.sub, 16); b[o + 101] = k.hitDie; b[o + 102] = k.hdUsed;
    k.spells.slice(0, MAXSP).forEach((s, j) => { const q = o + SPOFF + j * SPREC; putStr(b, q, s.name, 20); b[q + 20] = 1 | (s.prep ? 2 : 0); });
    k.weapons.slice(0, NW).forEach((w, j) => {
      const q = o + WOFF + j * WREC; b[q] = 1; putStr(b, q + 1, w.name, 12); b[q + 13] = w.ab | (w.prof ? 4 : 0);
      d.setInt8(q + 14, w.bonus); b[q + 15] = w.n; b[q + 16] = w.die; b[q + 17] = w.type; putStr(b, q + 18, w.range, 7); b[q + 25] = w.mastery; d.setInt8(q + 26, w.dmg); putStr(b, q + 27, w.props, 28);
    });
    k.feats.slice(0, MAXF).forEach((f, j) => { const q = o + FOFF + j * FREC; putStr(b, q, f.name, 20); b[q + 20] = f.used; b[q + 21] = 1; });
    putStr(b, o + IDOFF, k.species, 16); putStr(b, o + IDOFF + 16, k.background, 16);
  });
  d.setUint32(8, crc32(b), true); return b;
}
function valid(k) {
  return k.name.length > 0 && NAME_OK.test(k.name) && NAME_OK.test(k.cls) && NAME_OK.test(k.sub) && k.level >= 1 && k.level <= 20 && k.ab.every(v => v >= 1 && v <= 30) &&
    k.saveProf < 64 && k.skill.every(v => v <= 2) && k.hpMax >= 1 && k.hpMax <= 999 && k.hp >= 0 && k.hp <= k.hpMax && k.temp <= 999 &&
    k.ac <= 40 && k.speed <= 120 && k.ds <= 3 && k.df <= 3 && k.slotMax.every(v => v <= 4) && k.slotUsed.every((v, j) => v <= k.slotMax[j]) &&
    (k.spellAb <= 5 || k.spellAb === 255) && k.insp <= 1 && HITDIE.includes(k.hitDie) && k.hdUsed <= k.level && new Set(k.spells.map(s => s.name)).size === k.spells.length &&
    NAME_OK.test(k.species) && NAME_OK.test(k.background) && new Set(k.feats.map(f => f.name)).size === k.feats.length;
}
function parse(b) {
  if (!b) return { ok: false, reason: 'MISSING' };
  if (b.length < HDR || String.fromCharCode(b[0], b[1], b[2], b[3]) !== 'PKSH') return { ok: false, reason: 'MAGIC' };
  const d = new DataView(b.buffer, b.byteOffset, b.length), ver = d.getUint16(4, true), REC = RECV[ver], { WOFF, WREC, SPOFF, FOFF, IDOFF } = LAYOUT(ver);
  if (!REC) return { ok: false, reason: 'VERSION' };
  if (b.length !== HDR + REC * NS) return { ok: false, reason: 'SIZE' };
  if (b[6] !== NS) return { ok: false, reason: 'SLOTS' };
  const z = b.slice(); z[8] = z[9] = z[10] = z[11] = 0;
  if (crc32(z) !== d.getUint32(8, true)) return { ok: false, reason: 'CHECKSUM' };
  const slots = [];
  for (let i = 0; i < NS; i++) {
    const o = HDR + i * REC; if (b[o] === 0) { slots.push(null); continue; } if (b[o] !== 1) return { ok: false, reason: 'RANGE' };
    const k = { name: getStr(b, o + 1, 16), cls: getStr(b, o + 17, 12), sub: ver >= 2 ? getStr(b, o + 85, 16) : '', level: b[o + 29], ab: [...b.slice(o + 30, o + 36)], saveProf: b[o + 36], skill: [...b.slice(o + 37, o + 55)],
      hp: d.getInt16(o + 55, true), hpMax: d.getUint16(o + 57, true), temp: d.getUint16(o + 59, true), ac: b[o + 61], speed: b[o + 62], ds: b[o + 63], df: b[o + 64],
      slotMax: [...b.slice(o + 65, o + 74)], slotUsed: [...b.slice(o + 74, o + 83)], spellAb: b[o + 83], insp: b[o + 84], weapons: [], hitDie: ver >= 3 && b[o + 101] ? b[o + 101] : 8, hdUsed: ver >= 3 ? b[o + 102] : 0, spells: [], feats: [], species: ver >= 5 ? getStr(b, o + IDOFF, 16) : '', background: ver >= 5 ? getStr(b, o + IDOFF + 16, 16) : '' };
    if (ver >= 3) for (let j = 0; j < NW; j++) {
      const q = o + WOFF + j * WREC; if (b[q] === 0) continue; if (b[q] !== 1 || b[q + 13] > 7) return { ok: false, reason: 'RANGE' };
      const w = { name: getStr(b, q + 1, 12), ab: b[q + 13] & 3, prof: (b[q + 13] >> 2) & 1, bonus: d.getInt8(q + 14), n: b[q + 15], die: b[q + 16], type: b[q + 17], range: getStr(b, q + 18, 7), mastery: b[q + 25], dmg: ver >= 5 ? d.getInt8(q + 26) : d.getInt8(q + 14), props: ver >= 5 ? getStr(b, q + 27, 28) : '' };
      if (!w.name || !NAME_OK.test(w.name) || !NAME_OK.test(w.range) || !TEXT_OK.test(w.props) || w.dmg < -10 || w.dmg > 30 || w.bonus < -5 || w.bonus > 10 || w.n < 1 || w.n > 9 || !DIE.includes(w.die) || w.type >= DTYPE.length || w.mastery >= MASTERY.length) return { ok: false, reason: 'RANGE' };
      k.weapons.push(w);
    }
    if (ver >= 4) for (let j = 0; j < MAXSP; j++) {
      const q = o + SPOFF + j * SPREC, fl = b[q + 20]; if (fl === 0) continue; if (fl > 3 || !(fl & 1)) return { ok: false, reason: 'RANGE' };
      const name = getStr(b, q, 20); if (!name || !NAME_OK.test(name)) return { ok: false, reason: 'RANGE' }; k.spells.push({ name, prep: (fl >> 1) & 1 });
    }
    if (ver >= 5) for (let j = 0; j < MAXF; j++) {
      const q = o + FOFF + j * FREC; if (b[q + 21] === 0) continue; if (b[q + 21] !== 1 || b[q + 20] > 20) return { ok: false, reason: 'RANGE' };
      const name = getStr(b, q, 20); if (!name || !NAME_OK.test(name)) return { ok: false, reason: 'RANGE' }; k.feats.push({ name, used: b[q + 20] });
    }
    if (!valid(k)) return { ok: false, reason: 'RANGE' }; slots.push(k);
  }
  return { ok: true, slots, last: Math.min(b[7], NS - 1), migrated: ver < VER };
}

// ---------- portrait: 48x48 uncompressed BMP, 24-bit or 8-bit indexed; shown as RGB555 ----------
const PW = 48;
function parseBMP(b) {
  if (!b || b.length < 54 || b[0] !== 66 || b[1] !== 77) return { ok: false, reason: 'NOT A BMP' };
  const d = new DataView(b.buffer, b.byteOffset, b.length);
  const off = d.getUint32(10, true), dib = d.getUint32(14, true), w = d.getInt32(18, true), h = d.getInt32(22, true), bpp = d.getUint16(28, true), comp = d.getUint32(30, true);
  if (dib < 40 || d.getUint16(26, true) !== 1) return { ok: false, reason: 'HEADER' };
  if (w !== PW || Math.abs(h) !== PW) return { ok: false, reason: 'NOT 48X48' };
  if ((bpp !== 24 && bpp !== 8) || comp !== 0) return { ok: false, reason: 'USE 24 OR 8 BIT' };
  const stride = ((w * bpp + 31) >> 5) * 4; if (off + stride * PW > b.length) return { ok: false, reason: 'TRUNCATED' };
  const pal = 14 + dib, ncol = bpp === 8 ? (d.getUint32(46, true) || 256) : 0; if (bpp === 8 && pal + ncol * 4 > off) return { ok: false, reason: 'PALETTE' };
  const px = new Uint8ClampedArray(PW * PW * 4), q = v => { const v5 = v >> 3; return (v5 << 3) | (v5 >> 2); };
  for (let y = 0; y < PW; y++) {
    const row = off + (h > 0 ? PW - 1 - y : y) * stride;
    for (let x = 0; x < PW; x++) {
      let r, g, bl; if (bpp === 24) { const p = row + x * 3; bl = b[p]; g = b[p + 1]; r = b[p + 2]; } else { const ix = b[row + x]; if (ix >= ncol) return { ok: false, reason: 'PALETTE' }; const p = pal + ix * 4; bl = b[p]; g = b[p + 1]; r = b[p + 2]; }
      const o = (y * PW + x) * 4; px[o] = q(r); px[o + 1] = q(g); px[o + 2] = q(bl); px[o + 3] = 255;
    }
  }
  return { ok: true, px };
}
export function makeBMP24(rgba) {
  const stride = PW * 3, size = 54 + stride * PW, b = new Uint8Array(size), d = new DataView(b.buffer);
  b[0] = 66; b[1] = 77; d.setUint32(2, size, true); d.setUint32(10, 54, true); d.setUint32(14, 40, true); d.setInt32(18, PW, true); d.setInt32(22, PW, true);
  d.setUint16(26, 1, true); d.setUint16(28, 24, true); d.setUint32(34, stride * PW, true);
  for (let y = 0; y < PW; y++) for (let x = 0; x < PW; x++) { const s = (y * PW + x) * 4, t = 54 + (PW - 1 - y) * stride + x * 3; b[t] = rgba[s + 2]; b[t + 1] = rgba[s + 1]; b[t + 2] = rgba[s]; }
  return b;
}

// ---------- simulated flashcart FAT (browser localStorage) ----------
const LS = 'pocketsheet.sim.sd.v6';
const enc = b => { let s = ''; for (let i = 0; i < b.length; i += 4096) s += String.fromCharCode(...b.subarray(i, i + 4096)); return btoa(s); };
const dec = s => Uint8Array.from(atob(s), c => c.charCodeAt(0));
function makeFS() {
  const f = {
    mode: 'ok', mounted: false,
    _get() { try { return JSON.parse(localStorage.getItem(LS) || '{}'); } catch (e) { return {}; } },
    _put(o) { try { localStorage.setItem(LS, JSON.stringify(o)); return true; } catch (e) { return false; } },
    mount() { f.mounted = f.mode !== 'missing'; return f.mounted; },
    ok() { return f.mounted && f.mode !== 'missing'; },
    canWrite() { return f.ok() && f.mode === 'ok'; },
    read(p) { if (!f.ok()) return null; const o = f._get(); return o[p] != null ? dec(o[p]) : null; },
    write(p, b) { if (!f.canWrite()) return false; const o = f._get(); o[p] = enc(b); return f._put(o); },
    exists(p) { return f.ok() && p in f._get(); },
    remove(p) { if (!f.canWrite()) return false; const o = f._get(); delete o[p]; return f._put(o); },
    rename(a, b) { if (!f.canWrite()) return false; const o = f._get(); if (!(a in o) || (b in o)) return false; o[b] = o[a]; delete o[a]; return f._put(o); },
  };
  return f;
}

// ---------- app ----------
const KEYS = ['A', 'B', 'X', 'Y', 'L', 'R', 'START', 'SELECT', 'UP', 'DOWN', 'LEFT', 'RIGHT'];
const REPEAT_DELAY = 15, REPEAT_RATE = 4; // frames, same as keysSetRepeat(15, 4)

export function createSim(topCv, botCv) {
  const T = topCv.getContext('2d'), Bc = botCv.getContext('2d');
  T.imageSmoothingEnabled = false; Bc.imageSmoothingEnabled = false;
  const fs = makeFS();
  const hw = {}, latch = {}, prev = {}, rep = {};
  let tq = [], uid = 0, S, pcache = {};
  function fresh() { S = { view: 'boot', t: 0, session: 'sd', slots: [null, null, null], cur: 0, sel: 0, tab: 0, prevTab: 0, editPage: 0, slotEdit: false, fx: [], scroll: {}, lib: new Map(), libInfo: { state: 'none', added: 0, skipped: 0 }, flib: new Map(), flibInfo: { state: 'none', added: 0, skipped: 0 }, focus: {}, amt: '', dirty: false, toast: null, ov: [], lid: false, lidSave: null, press: null, frame: 0 }; fs.mounted = false; tq = []; pcache = {}; }
  fresh();
  const ch = () => S.slots[S.cur];
  const toast = (text, col = PAL.green) => { S.toast = { text, col, t: 110 }; };
  const addFx = f => { S.fx = S.fx.filter(x => x.k !== f.k); f.T = f.t; f.seed = (S.frame * 7919) % 997; S.fx.push(f); };
  const pushOv = o => { o.uid = ++uid; S.ov.push(o); S.press = null; };
  const popOv = () => { S.ov.pop(); S.press = null; };
  const pushNotice = n => pushOv({ type: 'notice', ...n });

  function scan() {
    const k = { down: {}, held: {}, rep: {} };
    for (const n of KEYS) {
      const h = !!(hw[n] || latch[n]); latch[n] = false; k.held[n] = h; k.down[n] = h && !prev[n];
      if (k.down[n]) rep[n] = 0; else if (h) rep[n] = (rep[n] || 0) + 1;
      k.rep[n] = k.down[n] || (h && rep[n] >= REPEAT_DELAY && (rep[n] - REPEAT_DELAY) % REPEAT_RATE === 0); prev[n] = h;
    }
    return k;
  }

  // ---------- portraits (loaded from the SD on demand, cached until reboot or change) ----------
  function portrait(i) {
    if (pcache[i]) return pcache[i];
    const b = fs.read(PORTRAIT(i)); let r;
    if (!b) r = { state: 'none' };
    else { const p = parseBMP(b); if (!p.ok) r = { state: 'bad', reason: p.reason }; else { const cv = document.createElement('canvas'); cv.width = cv.height = PW; cv.getContext('2d').putImageData(new ImageData(p.px, PW, PW), 0, 0); r = { state: 'ok', img: cv }; } }
    return pcache[i] = r;
  }
  function drawPortrait(c, i, x, y) {
    const p = portrait(i), k = S.slots[i];
    if (p.state === 'ok') { c.drawImage(p.img, x, y); BOX(c, x - 1, y - 1, PW + 2, PW + 2, PAL.line, 1); return; }
    R(c, x, y, PW, PW, PAL.panel2); BOX(c, x - 1, y - 1, PW + 2, PW + 2, p.state === 'bad' ? PAL.red : PAL.line, 1);
    if (p.state === 'bad') { txc(c, 'BAD', x, PW, y + 14, PAL.red); txc(c, 'FILE', x, PW, y + 26, PAL.red); return; }
    const ini = k ? k.name.split(' ').filter(Boolean).slice(0, 2).map(w => w[0]).join('') : '?';
    txc(c, ini, x, PW, y + 17, PAL.dim, 2);
  }

  // ---------- storage ----------
  function saveAll() {
    if (S.session === 'temp') return { ok: false, code: 'TEMP' };
    const bytes = serialize(S.slots, S.cur);
    if (!fs.write(TMP, bytes)) return { ok: false, code: 'WRITE' };
    const back = fs.read(TMP);
    if (!back || back.length !== bytes.length || back.some((v, i) => v !== bytes[i])) { fs.remove(TMP); return { ok: false, code: 'VERIFY' }; }
    if (fs.exists(SAV)) { fs.remove(BAK); if (!fs.rename(SAV, BAK)) return { ok: false, code: 'RENAME' }; }
    if (!fs.rename(TMP, SAV)) return { ok: false, code: 'RENAME' };
    S.dirty = false; return { ok: true };
  }
  function saveFailNotice(r) {
    if (r.code === 'TEMP') return { title: 'SAVING IS OFF', sub: 'TEMPORARY SESSION', lines: ['THIS IS A TEMPORARY SESSION. CHANGES LAST UNTIL YOU POWER OFF.', 'RESTART WITH A WORKING SD CARD TO SAVE.'], btns: [{ label: 'OK' }] };
    return { title: 'SAVE FAILED', tone: 'err', sub: 'NOTHING WAS OVERWRITTEN', lines: [`THE SD CARD REFUSED THE WRITE (${r.code}).`, 'YOUR LAST GOOD SAVE WAS NOT CHANGED.', 'YOUR EDITS ARE STILL HERE IN MEMORY. YOU CAN KEEP GOING AND RETRY LATER.'], btns: [{ label: 'RETRY', primary: true, act: () => { popOv(); doSave(); } }, { label: 'OK' }], cancel: 1 };
  }
  function doSave() { const r = saveAll(); if (r.ok) toast('SAVED TO SD CARD ✓'); else pushNotice(saveFailNotice(r)); return r; }
  function loadAll() {
    const sav = fs.read(SAV), bak = fs.read(BAK);
    if (!sav && !bak) {
      S.slots = demoSlots(); S.cur = 0; S.dirty = true; const r = saveAll();
      if (r.ok && !fs.exists(PORTRAIT(0))) { fs.write(PORTRAIT(0), dec(DEMO_PORTRAIT1)); pcache = {}; }
      if (r.ok) toast('CREATED /DATA/POCKETSHEET/'); else return saveFailNotice(r);
      return null;
    }
    const a = parse(sav);
    if (a.ok) { S.slots = a.slots; S.cur = a.last; if (a.migrated) { S.dirty = true; toast('OLDER SAVE UPGRADED. SAVE TO KEEP IT.', PAL.blue); } return null; }
    const b = parse(bak);
    if (sav) { fs.remove(BAD); fs.rename(SAV, BAD); }
    if (b.ok) {
      S.slots = b.slots; S.cur = b.last; S.dirty = true;
      return { title: 'BACKUP RESTORED', sub: 'SAVE FILE WAS DAMAGED', lines: [sav ? `SHEET.SAV FAILED ITS CHECK (${a.reason}).` : 'SHEET.SAV WAS MISSING.', 'LOADED SHEET.BAK, THE SAVE BEFORE IT. YOUR MOST RECENT CHANGES MAY BE MISSING.', sav ? 'THE DAMAGED FILE WAS KEPT AS SHEET.BAD.' : ''].filter(Boolean), btns: [{ label: 'OK', primary: true }] };
    }
    S.slots = [null, null, null]; S.cur = 0;
    return { title: "CAN'T READ SAVE", tone: 'err', sub: 'SAVE AND BACKUP DAMAGED', lines: [`SHEET.SAV: ${a.reason}. SHEET.BAK: ${b.reason}.`, 'THE DAMAGED FILE WAS KEPT AS SHEET.BAD.', 'START FRESH WRITES A NEW SAVE. TEMP SESSION SAVES NOTHING.'],
      btns: [{ label: 'START FRESH', primary: true }, { label: 'TEMP SESSION', act: () => { S.session = 'temp'; S.slots = demoSlots(); popOv(); } }] };
  }
  function loadLibrary() {
    S.lib = new Map(STARTER.map(a => { const s = mkSpell(a); return [s.name, s]; })); S.libInfo = { state: 'none', added: 0, skipped: 0 };
    S.flib = new Map(FSTARTER.map(a => { const f = mkFeat(a); return [f.name, f]; })); S.flibInfo = { state: 'none', added: 0, skipped: 0 };
    if (!fs.ok()) { parseSpellsTxt(DEMO_SPELLS).spells.forEach(s => S.lib.set(s.name, s)); parseFeaturesTxt(DEMO_FEATURES).feats.forEach(f => S.flib.set(f.name, f)); return; }
    let b = fs.read(SPELLS_TXT);
    if (!b && fs.canWrite()) { fs.write(SPELLS_TXT, new TextEncoder().encode(SPELLS_TEMPLATE + DEMO_SPELLS)); b = fs.read(SPELLS_TXT); }
    let fb = fs.read(FEATURES_TXT);
    if (!fb && fs.canWrite()) { fs.write(FEATURES_TXT, new TextEncoder().encode(FEATURES_TEMPLATE + DEMO_FEATURES)); fb = fs.read(FEATURES_TXT); }
    let skipped = 0;
    if (b) { const r = parseSpellsTxt(decodeText(b)); r.spells.forEach(s => S.lib.set(s.name, s)); S.libInfo = { state: 'ok', added: r.spells.length, skipped: r.skipped }; skipped += r.skipped; }
    if (fb) { const r = parseFeaturesTxt(decodeText(fb)); r.feats.forEach(f => S.flib.set(f.name, f)); S.flibInfo = { state: 'ok', added: r.feats.length, skipped: r.skipped }; skipped += r.skipped; }
    if (skipped) toast('LIBRARY FILES: ' + skipped + ' ENTR' + (skipped > 1 ? 'IES' : 'Y') + ' SKIPPED', PAL.gold);
  }
  function startup() {
    S.view = 'select';
    if (!fs.mount()) {
      loadLibrary();
      const n = { title: 'NO SD CARD FOUND', tone: 'err', sub: 'FAT/DLDI STARTUP FAILED', lines: ["THE APP CAN'T OPEN THE FLASHCART'S MICROSD, SO IT CAN'T LOAD OR SAVE CHARACTERS.", 'CHECK THE CARD IS SEATED AND THAT YOU STARTED THE APP FROM THE CART MENU.'] };
      n.btns = [{ label: 'TRY AGAIN', primary: true, act: () => { if (fs.mount()) { popOv(); pcache = {}; loadLibrary(); const m = loadAll(); if (m) pushNotice(m); S.focus.select = 'slot' + S.cur; } else n.err = 'STILL NO SD CARD.'; } },
        { label: 'NO-SAVE MODE', act: () => { S.session = 'temp'; S.slots = demoSlots(); popOv(); toast('TEMPORARY SESSION', PAL.gold); } }];
      pushNotice(n); return;
    }
    loadLibrary(); const n = loadAll(); S.focus.select = 'slot' + S.cur; S.sel = S.cur; if (n) pushNotice(n);
  }

  // ---------- actions ----------
  function openSlot(i) { S.cur = i; S.sel = i; S.tab = 0; S.prevTab = 0; S.editPage = 0; S.slotEdit = false; S.fx = []; S.fpeek = null; S.scroll = {}; S.amt = ''; S.view = 'sheet'; S.focus = { select: 'slot' + i }; }
  function leave() {
    let r = null; if (S.dirty && S.session === 'sd') r = saveAll();
    S.view = 'select'; S.focus.select = 'slot' + S.cur; S.sel = S.cur;
    if (r) { if (r.ok) toast('SAVED TO SD CARD ✓'); else pushNotice(saveFailNotice(r)); }
  }
  function stepper(o) { pushOv({ type: 'stepper', step: 1, big: 5, ...o, val: o.get() }); }
  function kbd(o) { pushOv({ type: 'kbd', val: '', max: 16, ...o }); }
  function askDelete(i) {
    const k = S.slots[i]; if (!k) return;
    pushNotice({ title: `DELETE ${k.name}?`, tone: 'err', sub: "THIS CAN'T BE UNDONE", lines: ['THIS REMOVES THE CHARACTER FROM THE SAVE FILE. ITS PORTRAIT FILE STAYS ON THE SD CARD.', 'THE PREVIOUS SAVE STAYS IN SHEET.BAK UNTIL THE NEXT SAVE.'],
      btns: [{ label: 'KEEP' }, { label: 'DELETE', danger: true, act: () => { S.slots[i] = null; S.dirty = true; popOv(); if (S.session === 'sd') doSave(); else toast('DELETED', PAL.gold); } }], def: 0, cancel: 0 });
  }
  function newCharFlow(i) {
    kbd({ title: 'NEW CHARACTER NAME', field: 'NAME', onOk: v => { popOv(); S.slots[i] = newChar(v); openSlot(i); S.tab = 5; S.dirty = true; toast('NEW CHARACTER: FILL IN YOUR SHEET', PAL.blue); } });
  }
  function applyDmg(k, n) {
    if (!n) return toast('ENTER AN AMOUNT FIRST', PAL.gold);
    const t = Math.min(k.temp, n); k.temp -= t; const before = k.hp; k.hp = Math.max(0, k.hp - (n - t));
    addFx({ k: 'shake', t: 12 }); addFx({ k: 'flash', t: 14, col: PAL.red }); addFx({ k: 'count', t: 18, from: before, to: k.hp }); addFx({ k: 'float', t: 40, text: '-' + n, col: PAL.red });
    toast(k.hp === 0 ? k.name.split(' ')[0] + ' IS DOWN!' : 'OUCH! -' + n + (t ? ' (' + t + ' TEMP)' : '') + '  HP ' + k.hp + '/' + k.hpMax, PAL.red); S.amt = ''; S.dirty = true;
  }
  function applyHeal(k, n) {
    if (!n) return toast('ENTER AN AMOUNT FIRST', PAL.gold);
    const before = k.hp; k.hp = Math.min(k.hpMax, k.hp + n); if (before === 0 && k.hp > 0) { k.ds = 0; k.df = 0; }
    addFx({ k: 'flash', t: 14, col: PAL.green }); addFx({ k: 'count', t: 18, from: before, to: k.hp }); addFx({ k: 'float', t: 40, text: '+' + (k.hp - before), col: PAL.green }); addFx({ k: 'sparkle', t: 36, rect: [4, 54, 114, 38], col: PAL.green });
    toast(before === 0 ? 'BACK ON YOUR FEET!  HP ' + k.hp : k.hp === k.hpMax ? '+' + (k.hp - before) + ' HP  GOOD AS NEW!' : '+' + n + ' HP  NOW ' + k.hp + '/' + k.hpMax, PAL.green); S.amt = ''; S.dirty = true;
  }
  function applyTemp(k, n) {
    if (!n) return toast('ENTER AN AMOUNT FIRST', PAL.gold);
    const old = k.temp; k.temp = Math.max(old, n);
    if (n > old) { addFx({ k: 'flash', t: 14, col: PAL.blue }); addFx({ k: 'float', t: 40, text: '+' + (k.temp - old), col: PAL.blue }); }
    toast(n > old ? 'A WARD OF ' + k.temp + ' TEMP HP SHIMMERS' : 'KEPT ' + old + ': TEMP HP DOESN\'T STACK', PAL.blue); S.amt = ''; S.dirty = true;
  }

  // ---------- navigation ----------
  const navList = btns => btns.filter(b => b.nav !== false && !b.disabled);
  function curFocus(cx) {
    const list = navList(cx.btns), id = S.focus[cx.key];
    if (list.find(b => b.id === id)) return id;
    const d = cx.def && list.find(b => b.id === cx.def) ? cx.def : (list[0] && list[0].id);
    S.focus[cx.key] = d; return d;
  }
  const focused = cx => cx.btns.find(b => b.id === curFocus(cx));
  function nav(btns, id, dir) {
    const list = navList(btns), c = list.find(b => b.id === id); if (!c) return list[0] && list[0].id;
    const cx = c.x + c.w / 2, cy = c.y + c.h / 2; let best = null, bs = Infinity;
    for (const b of list) {
      if (b === c) continue; const bx = b.x + b.w / 2, by = b.y + b.h / 2; let p, gap, off;
      if (dir === 'LEFT' || dir === 'RIGHT') { p = dir === 'LEFT' ? cx - bx : bx - cx; gap = Math.max(0, Math.max(b.y, c.y) - Math.min(b.y + b.h, c.y + c.h)); off = Math.abs(by - cy); }
      else { p = dir === 'UP' ? cy - by : by - cy; gap = Math.max(0, Math.max(b.x, c.x) - Math.min(b.x + b.w, c.x + c.w)); off = Math.abs(bx - cx); }
      if (p <= 1) continue; const s = p + gap * 3 + off * 0.3; if (s < bs) { bs = s; best = b; }
    }
    return best ? best.id : id;
  }
  const hit = (btns, x, y) => { for (let i = btns.length - 1; i >= 0; i--) { const b = btns[i]; if (!b.disabled && x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) return b; } return null; };
  const act = b => { if (b && !b.disabled && b.act) b.act(); };
  function touchStep(cx) {
    while (tq.length) {
      const e = tq.shift();
      if (e.t === 'down') { const b = hit(cx.btns, e.x, e.y); S.press = { id: b ? b.id : null, inside: !!b, ctx: cx.key }; if (b && b.nav !== false) S.focus[cx.key] = b.id; }
      else if (e.t === 'move') { if (S.press && S.press.id) { const b = hit(cx.btns, e.x, e.y); S.press.inside = !!b && b.id === S.press.id; } }
      else if (e.t === 'cancel') S.press = null;
      else if (e.t === 'up') {
        const p = S.press; S.press = null;
        if (p && p.id && p.ctx === cx.key) { const b = hit(cx.btns, e.x, e.y); if (b && b.id === p.id) { act(b); return true; } }
      }
    }
    return false;
  }

  // ---------- drawing ----------
  function footer(c, hints, edit) {
    R(c, 0, 180, W, 12, edit ? PAL.blue : PAL.panel); let x = 4;
    for (const [k, l] of hints) { const w = tw(k) + 4; R(c, x, 181, w, 9, edit ? PAL.ink : PAL.dim); tx(c, k, x + 2, 182, edit ? PAL.blue : PAL.ink); tx(c, l, x + w + 3, 182, edit ? PAL.ink : PAL.text); x += w + 3 + tw(l) + 8; }
  }
  const strip = (c, text, col) => { R(c, 0, 178, W, 14, col); txc(c, text, 0, W, 182, PAL.ink); };
  // The always-on reference page (top screen)
  // pixel heater shield: flat top with clipped corners, straight upper sides, convex curve down to a point
  function shield(c, x, y, w, h, col) {
    const half = w / 2, start = Math.round(h / 3);
    for (let r = 0; r < h; r++) {
      const p = r < start ? 0 : (r - start) / (h - 1 - start);
      const inset = r === 0 ? 2 : r === 1 ? 1 : Math.min(half - 1, Math.round((half - 1) * Math.pow(p, 1.8)));
      R(c, x + inset, y + r, w - inset * 2, 1, col);
    }
  }
  // The always-on reference page (top screen). Hierarchy: who → survive (HP, AC) → abilities+saves → secondary numbers → status
  // summary card over the abilities + secondary rows (y 97..175) for the focused spell or feature
  function drawSummary(c, s) {
    R(c, 0, 97, W, 79, PAL.panel); R(c, 0, 97, 2, 79, s.col || PAL.gold);
    if (s.icon) icon(c, s.icon, 6, 101 + (7 - isz(s.icon)[1] >> 1), [s.col || PAL.gold]);
    const nx = s.icon ? 6 + isz(s.icon)[0] + 4 : 6; tx(c, s.title, nx, 101, PAL.text);
    if (s.tag) txr(c, s.tag, 251, 101, s.tagCol || PAL.gold);
    tx(c, s.meta.slice(0, 41), 6, 112, PAL.dim);
    R(c, 6, 122, 244, 1, PAL.line);
    const lines = String(s.desc || '').split('\n').flatMap((p, i) => (i ? [''] : []).concat(wrap(p, 41))).filter((l, i, a) => !(l === '' && (i === 0 || a[i - 1] === '')));
    const vis = s.hint ? 4 : 5, more = lines.length > vis;
    lines.slice(0, vis).forEach((l, i) => { const last = i === vis - 1 && more; tx(c, last ? (l.length > 37 ? l.slice(0, 37) : l) + '...' : l, 6, 126 + i * 10, PAL.text); });
    if (s.hint) { const mt = 6 + tw('MORE ON THE FULL PAGE') + 8 <= 251 - tw(s.hint) ? 'MORE ON THE FULL PAGE' : 'MORE...'; if (more) tx(c, mt, 6, 167, PAL.dim); txr(c, s.hint, 251, 167, PAL.gold); }
  }
  function overviewTop(c, banner, sum) {
    const k = ch(), dm = mod(k.ab[1]), down = k.hp === 0; R(c, 0, 0, W, H, PAL.bg);
    // 1. identity
    drawPortrait(c, S.cur, 1, 1);
    tx(c, k.name, 55, 4, PAL.text, tw(k.name, 2) <= 199 ? 2 : 1);
    const lv = 'LEVEL ' + k.level, cl = (k.cls + (k.sub ? ', ' + k.sub : '')).slice(0, Math.floor((252 - tw(lv) - 8 - 55) / 6));
    tx(c, cl, 55, 24, PAL.text); tx(c, lv, 55 + tw(cl) + 8, 24, PAL.gold);
    const idl = [k.species, k.background].filter(Boolean).join(', '); if (idl) tx(c, idl.slice(0, 33), 55, 36, PAL.dim);
    // 2. survival: HP dominates, AC next, INIT + SPEED supporting
    const fxo = n => S.fx.find(f => f.k === n), sh = fxo('shake'), hx = sh ? (sh.t % 4 < 2 ? 2 : -2) : 0, cnt = fxo('count'), fl = fxo('flash');
    const shown = cnt ? Math.round(cnt.to + (cnt.from - cnt.to) * (cnt.t / cnt.T)) : k.hp;
    R(c, 1 + hx, 52, 120, 42, PAL.panel); if (fl && (fl.t >> 1) % 2 === 0) BOX(c, 1 + hx, 52, 120, 42, fl.col, 2);
    icon(c, down ? 'mskull' : 'mheart', 5 + hx, 56, [PAL.red]); tx(c, down ? 'DOWN' : 'HIT POINTS', 15 + hx, 56, down ? PAL.red : PAL.dim);
    if (k.temp) { const ts = '+' + k.temp; txr(c, ts, 117 + hx, 56, PAL.blue); icon(c, 'mshield', 117 + hx - tw(ts) - 10, 56, [PAL.blue]); }
    const cur = String(shown), ccol = down ? PAL.red : fl && fl.t > 4 ? fl.col : PAL.text; tx(c, cur, 5 + hx, 68, ccol, 3); tx(c, '/' + k.hpMax, 5 + hx + tw(cur, 3) + 3, 75, PAL.dim, 2);
    shield(c, 122, 52, 48, 42, PAL.dim); shield(c, 124, 54, 44, 38, PAL.panel); txc(c, String(k.ac), 122, 48, 56, PAL.text, 3); txc(c, 'AC', 122, 48, 79, PAL.dim);
    [['INIT', sg(dm), 52, 'bolt', PAL.gold], ['SPEED', String(k.speed), 74, 'boot', PAL.dim]].forEach(([l, v, y, ic, col]) => { R(c, 172, y, 83, 20, PAL.panel); icon(c, ic, 176, y + 5, [col, PAL.line]); tx(c, l, 188, y + 7, PAL.dim); txr(c, v, 250, y + 3, PAL.text, 2); });
    if (sum) drawSummary(c, sum); else {
    // 3. abilities with their saving throw underneath
    for (let i = 0; i < 6; i++) {
      const x = i * 43, y = 97, p = (k.saveProf >> i) & 1, sv = sg(svMod(k, i));
      R(c, x, y, 41, 45, PAL.panel); tx(c, AB[i], x + 4, y + 4, PAL.dim); txr(c, String(k.ab[i]), x + 37, y + 4, PAL.dim);
      txc(c, sg(mod(k.ab[i])), x, 41, y + 13, PAL.text, 2);
      R(c, x, y + 30, 41, 15, PAL.panel2);
      const lbl = tw('SAVE') + 3 + tw(sv) <= 39 ? 'SAVE' : '', w = lbl ? tw(lbl) + 3 + tw(sv) : tw(sv), x0 = x + ((41 - w) >> 1);
      if (lbl) tx(c, lbl, x0, y + 34, PAL.dim); tx(c, sv, x0 + (lbl ? tw(lbl) + 3 : 0), y + 34, p ? PAL.gold : PAL.text);
    }
    // 4. secondary numbers
    R(c, 0, 145, W, 31, PAL.panel);
    const pair = (x, y, l, v, col) => { tx(c, l, x, y, PAL.dim); tx(c, v, x + tw(l) + 5, y, col || PAL.text); return x + tw(l) + 5 + tw(v) + 14; };
    let x = pair(5, 149, 'PROF', sg(pb(k))); icon(c, 'eye', x, 149, [PAL.dim, PAL.gold]); pair(x + 15, 149, 'PASSIVE PERCEPTION', String(10 + skMod(k, 11)));
    const has = k.spellAb !== 255; x = 5; let y = 162;
    if (has) { icon(c, 'mspark', x, y, [PAL.blue]); x += 10; x = pair(x, y, 'SPELL DC', String(8 + pb(k) + mod(k.ab[k.spellAb]))); x = pair(x, y, 'ATK', sg(pb(k) + mod(k.ab[k.spellAb]))); }
    if (k.slotMax.some(v => v > 0)) {
      if (!has) { tx(c, 'SLOTS', x, y, PAL.dim); x += 36; }
      const x0 = x;
      for (let l = 0; l < 9; l++) { const m = k.slotMax[l]; if (!m) continue; if (x + 8 + m * 6 > 254) { x = x0; y += 9; }
        tx(c, String(l + 1), x, y, PAL.dim); x += 8; for (let i = 0; i < m; i++) { if (i < m - k.slotUsed[l]) R(c, x, y, 4, 7, PAL.blue); else BOX(c, x, y, 4, 7, PAL.dim, 1); x += 6; } x += 6; }
    } else if (!has) tx(c, 'NO SPELLCASTING', x, y, PAL.dim);
    }
    for (const f of S.fx) if (f.k === 'float') { const p = 1 - f.t / f.T; if (f.t > 6 || (f.t & 1)) tx(c, f.text, 84 + hx, Math.round(72 - p * 16), f.col, 2); }
    // 5. status
    if (banner) strip(c, banner, PAL.blue);
    else if (S.toast) strip(c, S.toast.text, S.toast.col);
    else {
      R(c, 0, 178, W, 14, PAL.panel);
      if (down) { icon(c, 'mskull', 4, 182, [PAL.red]); tx(c, 'DEATH SAVES', 14, 182, PAL.red); let px = 84; for (let i = 0; i < 3; i++, px += 8) { if (i < k.ds) tx(c, '●', px, 182, PAL.green); else tx(c, '○', px, 182, PAL.line); } px += 6; for (let i = 0; i < 3; i++, px += 9) { if (i < k.df) icon(c, 'mskull', px, 182, [PAL.red]); else tx(c, '○', px, 182, PAL.line); } }
      else { icon(c, 'mstar', 4, 182, [k.insp ? PAL.gold : PAL.line]); tx(c, k.insp ? 'INSPIRED' : 'NO INSPIRATION', 14, 182, k.insp ? PAL.gold : PAL.dim); tx(c, 'HIT DICE ' + (k.level - k.hdUsed) + '/' + k.level, 104, 182, PAL.dim); }
      const st = S.session === 'temp' ? ['TEMP SESSION', PAL.blue] : S.dirty ? ['UNSAVED', PAL.gold] : ['SAVED', PAL.green]; txr(c, st[0], 252, 182, st[1]);
    }
    drawFx(c);
  }
  function sparkle(c, f, x, y, w, h, col) {
    for (let i = 0; i < 7; i++) {
      if (((f.t + i * 3) >> 2) % 2) continue;
      const px = x + ((f.seed * 37 + i * 53) % w), py = y + ((f.seed * 17 + i * 29) % h);
      R(c, px - 1, py, 3, 1, col); R(c, px, py - 1, 1, 3, col); R(c, px, py, 1, 1, PAL.text);
    }
  }
  function drawFx(c) {
    for (const f of S.fx) if (f.k === 'sparkle') sparkle(c, f, f.rect[0], f.rect[1], f.rect[2], f.rect[3], f.col);
    const ov = S.fx.find(f => f.k === 'night' || f.k === 'fire'); if (!ov) return;
    c.globalAlpha = Math.min(1, ov.t / 18, (ov.T - ov.t + 1) / 8);
    R(c, 0, 0, W, H, PAL.bg);
    if (ov.k === 'night') {
      for (let i = 0; i < 14; i++) { const px = (ov.seed * 31 + i * 71) % 250 + 3, py = (ov.seed * 13 + i * 37) % 170 + 4; if (((ov.t + i * 5) >> 3) % 3) R(c, px, py, 1, 1, PAL.text); else { R(c, px - 1, py, 3, 1, PAL.gold); R(c, px, py - 1, 1, 3, PAL.gold); } }
      icon(c, 'moon', 128 - 17, 30, [PAL.text, PAL.gold], 3);
      txc(c, 'A NIGHT PASSES...', 0, W, 112, PAL.gold, 2); txc(c, 'HP, SPELL SLOTS AND HIT DICE RESTORED', 0, W, 136, PAL.dim);
    } else {
      icon(c, (ov.t >> 3) & 1 ? 'fire2' : 'fire', 128 - 17, 28, [PAL.gold, PAL.text, PAL.dim], 3);
      sparkle(c, ov, 90, 14, 76, 30, PAL.gold);
      txc(c, 'YOU REST BY THE FIRE', 0, W, 112, PAL.gold, 2); if (ov.sub) txc(c, ov.sub, 0, W, 136, PAL.dim);
    }
    c.globalAlpha = 1;
  }
  function selectTop(c) {
    R(c, 0, 0, W, H, PAL.bg); txc(c, 'POCKET SHEET', 0, W, 16, PAL.gold, 2); txc(c, 'CHARACTER REFERENCE', 0, W, 36, PAL.dim);
    R(c, 8, 52, 240, 98, PAL.panel); const k = S.slots[S.sel];
    if (k) {
      drawPortrait(c, S.sel, 14, 58);
      tx(c, k.name, 70, 60, PAL.text, tw(k.name, 2) <= 172 ? 2 : 1);
      tx(c, k.cls + (k.sub ? ' ' + k.sub : ''), 70, 80, PAL.text);
      tx(c, `LEVEL ${k.level}   HP ${k.hp}/${k.hpMax}`, 70, 92, PAL.dim); tx(c, `AC ${k.ac}   PROF ${sg(pb(k))}`, 70, 102, PAL.dim);
      tx(c, AB.slice(0, 3).map((a, i) => `${a} ${k.ab[i]}`).join('   '), 14, 120, PAL.dim);
      tx(c, AB.slice(3).map((a, i) => `${a} ${k.ab[i + 3]}`).join('   '), 14, 132, PAL.dim);
    } else { txc(c, 'EMPTY SLOT', 8, 240, 86, PAL.dim, 2); txc(c, 'PRESS A TO CREATE A CHARACTER', 8, 240, 108, PAL.dim); }
    if (S.session === 'sd') { tx(c, 'SAVING TO', 8, 156, PAL.dim); tx(c, 'FAT:/DATA/POCKETSHEET/', 8, 166, PAL.text); }
    else { tx(c, 'TEMPORARY SESSION', 8, 156, PAL.gold); tx(c, 'NOTHING WILL BE SAVED', 8, 166, PAL.text); }
    txr(c, `SLOT ${S.sel + 1}/3`, 248, 166, PAL.dim);
    if (S.toast) strip(c, S.toast.text, S.toast.col);
  }
  const chip = (b, label, val, gold) => { b.draw = (c, st) => { R(c, b.x, b.y, b.w, b.h, st.p ? PAL.panel2 : PAL.panel); tx(c, label, b.x + 5, b.y + 4, PAL.dim); tx(c, val, b.x + 5, b.y + 13, gold ? PAL.gold : PAL.text); if (st.f) focusBox(c, b); }; return b; };
  const staticChip = (c, x, w, label, val) => { R(c, x, 19, w, 22, PAL.panel); tx(c, label, x + 5, 23, PAL.dim); tx(c, val, x + 5, 32, PAL.text); };
  const row = (b, label, val, arrow) => { b.draw = (c, st) => { R(c, b.x, b.y, b.w, b.h - 1, st.p ? PAL.panel2 : PAL.panel); tx(c, label, b.x + 6, b.y + 4, PAL.dim); txr(c, val(), b.x + b.w - (arrow ? 16 : 6), b.y + 4, PAL.gold); if (arrow) tx(c, '▶', b.x + b.w - 10, b.y + 4, PAL.dim); if (st.f) focusBox(c, { x: b.x, y: b.y, w: b.w, h: b.h - 1 }); }; return b; };
  const PROF_GLYPH = ['○', '●', '◆'], PROF_COL = [PAL.line, PAL.gold, PAL.blue];
  const skillRow = (b, k, i, edit) => { b.draw = (c, st) => { const p = k.skill[i], h = b.h - 1; R(c, b.x, b.y, b.w, h, st.p ? PAL.panel2 : PAL.panel); const ty = b.y + ((h - 7) >> 1);
    tx(c, PROF_GLYPH[p], b.x + 3, ty, PROF_COL[p]); tx(c, SK[i][0], b.x + 11, ty, PAL.text); txr(c, sg(skMod(k, i)), b.x + b.w - 4, ty, p === 2 ? PAL.blue : edit ? PAL.gold : PAL.text);
    if (st.f) focusBox(c, { x: b.x, y: b.y, w: b.w, h }); }; return b; };

  // ---------- rests ----------
  function restSave(msg) {
    if (S.session !== 'sd') return toast(msg, PAL.gold);
    const r = saveAll(); if (r.ok) toast(msg + ' ✓'); else pushNotice(saveFailNotice(r));
  }
  function askLongRest(k) {
    pushNotice({ title: 'TAKE A LONG REST?', sub: 'RESTORES HP, SLOTS, HIT DICE', lines: ['RESTORES ALL HP, SPELL SLOTS, HIT DICE AND FEATURE USES.', 'CLEARS TEMP HP AND DEATH SAVES.'], def: 0, cancel: 0,
      btns: [{ label: 'CANCEL' }, { label: 'REST', primary: true, act: () => { k.hp = k.hpMax; k.temp = 0; k.ds = 0; k.df = 0; k.slotUsed.fill(0); k.hdUsed = 0; k.feats.forEach(f => { const lf = S.flib.get(f.name); if (!lf || lf.reset !== 'NONE') f.used = 0; }); if (k.feats.some(f => { const lf = S.flib.get(f.name); return lf && lf.inspire; })) k.insp = 1; S.dirty = true; S.ov = []; addFx({ k: 'night', t: 90 }); restSave('A NIGHT PASSES. FULLY RESTORED'); } }] });
  }
  function openShortRest(k) { pushOv({ type: 'srest', n: 0, hp: 0 }); }

  // ---------- spells ----------
  const mySpells = k => k.spells.map(s => ({ ...s, sp: S.lib.get(s.name) })).sort((a, b) => ((a.sp ? a.sp.level : 99) - (b.sp ? b.sp.level : 99)) || (a.name < b.name ? -1 : 1));
  function togglePrep(k, name) {
    const s = k.spells.find(x => x.name === name), sp = S.lib.get(name); if (!s) return;
    if (sp && sp.level === 0) return toast('CANTRIPS ARE ALWAYS READY', PAL.blue);
    s.prep ^= 1; S.dirty = true; toast(name + (s.prep ? ' PREPARED' : ' UNPREPARED'), s.prep ? PAL.gold : PAL.dim);
  }
  function openPicker(k) { S.scroll.pk = 0; pushOv({ type: 'spellpick', filter: -1, scroll: 0, lastF: null }); }
  function togglePick(k, it) {
    const i = k.spells.findIndex(s => s.name === it.name);
    if (i >= 0) { k.spells.splice(i, 1); S.dirty = true; return toast('REMOVED ' + it.name, PAL.dim); }
    if (k.spells.length >= MAXSP) return toast('SPELL LIST FULL (' + MAXSP + ')', PAL.gold);
    k.spells.push({ name: it.name, prep: 0 }); S.dirty = true; toast('ADDED ' + it.name, PAL.blue);
  }
  // scrolling list: only visible rows become buttons; D-pad up/down moves through the whole list
  // scrolling list (optionally several columns, filled row by row); only visible cells become buttons
  function listRows(btns, L) {
    const n = L.items.length, cols = L.cols || 1, nr = Math.ceil(n / cols), gap = cols > 1 ? 2 : 0, cw = Math.floor((L.w - gap * (cols - 1)) / cols);
    let top = S.scroll[L.pre] || 0; const f = S.focus[L.fkey];
    const fi = f && f.startsWith(L.pre) ? +f.slice(L.pre.length) : NaN;
    if (!isNaN(fi)) { if (fi >= n) S.focus[L.fkey] = n ? L.pre + (n - 1) : null; else { const r = Math.floor(fi / cols); if (r < top) top = r; if (r >= top + L.rows) top = r - L.rows + 1; } }
    top = clamp(top, 0, Math.max(0, nr - L.rows)); S.scroll[L.pre] = top;
    for (let i = top * cols; i < Math.min(n, (top + L.rows) * cols); i++) {
      const r = Math.floor(i / cols) - top, cc = i % cols;
      const b = { id: L.pre + i, x: L.x + cc * (cw + gap), y: L.y + r * L.h, w: cw, h: L.h };
      b.draw = (c, st) => L.draw(c, b, st, L.items[i], i); b.act = () => L.act(i); if (L.onY) b.onY = () => L.onY(i); btns.push(b);
    }
    if (nr > L.rows) {
      const hh = (L.rows * L.h) / 2 - 1;
      [['u', '▲', L.y, top > 0, -1], ['d', '▼', L.y + hh + 2, top + L.rows < nr, 1]].forEach(([s, g, y, en, dir]) => {
        const b = { id: 'scr' + L.pre + s, x: L.x + L.w + 2, y, w: 22, h: hh, nav: false };
        b.draw = (c, st) => { R(c, b.x, b.y, b.w, b.h, st.p ? PAL.text : PAL.panel); txc(c, g, b.x, b.w, b.y + ((b.h - 7) >> 1), st.p ? PAL.ink : en ? PAL.text : PAL.line); };
        b.act = () => { const nt = clamp(top + dir * L.rows, 0, Math.max(0, nr - L.rows)); S.scroll[L.pre] = nt; S.focus[L.fkey] = L.pre + nt * cols; };
        btns.push(b);
      });
    }
    return { keys: k => {
      const f2 = S.focus[L.fkey]; if (!f2 || !f2.startsWith(L.pre)) return false; const j = +f2.slice(L.pre.length); if (isNaN(j)) return false;
      if (k.rep.DOWN) { if (j + cols < n) S.focus[L.fkey] = L.pre + (j + cols); else if (Math.floor(j / cols) < nr - 1) S.focus[L.fkey] = L.pre + (n - 1); return true; }
      if (k.rep.UP && j - cols >= 0) { S.focus[L.fkey] = L.pre + (j - cols); return true; }
      if (cols > 1 && k.rep.RIGHT && j % cols < cols - 1 && j + 1 < n) { S.focus[L.fkey] = L.pre + (j + 1); return true; }
      if (cols > 1 && k.rep.LEFT && j % cols > 0) { S.focus[L.fkey] = L.pre + (j - 1); return true; }
      return false;
    } };
  }
  function spellRow(c, b, st, it, mine) {
    const sp = it.sp, lv = sp ? sp.level : -1, h = b.h - 1, y = b.y + 4;
    R(c, b.x, b.y, b.w, h, st.p ? PAL.panel2 : PAL.panel);
    R(c, b.x + 2, b.y + 2, 11, 11, lv <= 0 ? PAL.line : PAL.panel2); txc(c, lv < 0 ? '?' : lv === 0 ? 'C' : String(lv), b.x + 2, 11, y, lv < 0 ? PAL.red : PAL.text);
    let x = b.x + 17;
    if (mine) { if (lv > 0) tx(c, it.prep ? '●' : '○', x, y, it.prep ? PAL.gold : PAL.line); x += 9; }
    tx(c, it.name, x, y, lv < 0 ? PAL.red : mine && lv > 0 && !it.prep ? PAL.dim : PAL.text);
    let r = b.x + b.w - 4;
    if (!mine && it.known) { txr(c, '✓', r, y, PAL.green); r -= 10; }
    if (sp && sp.ritual) { txr(c, 'R', r, y, PAL.gold); r -= 9; }
    if (sp && sp.conc) txr(c, '◆', r, y, PAL.blue);
    if (st.f) focusBox(c, { x: b.x, y: b.y, w: b.w, h });
  }
  // top screen: full spell card with scrolling description
  function spellTop(c, name, sp, o) {
    const k = ch(); R(c, 0, 0, W, H, PAL.bg); R(c, 0, 0, W, 32, PAL.panel);
    tx(c, name, 6, 4, PAL.text, tw(name, 2) <= 244 ? 2 : 1);
    if (!sp) {
      tx(c, 'NOT IN YOUR SPELL LIBRARY', 6, 21, PAL.red);
      wrap('ADD IT TO /DATA/POCKETSHEET/SPELLS.TXT ON A PC (THE FILE EXPLAINS THE FORMAT), OR REMOVE IT FROM THIS CHARACTER.', 40).forEach((l, i) => tx(c, l, 6, 44 + i * 10, PAL.text));
    } else {
      tx(c, sp.level === 0 ? sp.school + ' CANTRIP' : ORD[sp.level - 1] + '-LEVEL ' + sp.school, 6, 21, PAL.dim);
      let r = 250; if (sp.ritual) { txr(c, 'RITUAL', r, 21, PAL.gold); r -= 42; } if (sp.conc) txr(c, 'CONC', r, 21, PAL.blue);
      [['TIME', sp.time], ['RANGE', sp.range], ['COMPONENTS', sp.comp], ['DURATION', sp.dur]].forEach(([l, v], i) => { const x = 3 + (i % 2) * 126, y = 35 + Math.floor(i / 2) * 20; R(c, x, y, 124, 18, PAL.panel); tx(c, l, x + 4, y + 2, PAL.dim); tx(c, (v || '-').slice(0, 20), x + 4, y + 10, PAL.text); });
      const lines = sp.desc.split('\n').flatMap((p, i) => (i ? [''] : []).concat(wrap(p, 40))), vis = 11;
      if (o) { o.maxScroll = Math.max(0, lines.length - vis); o.scroll = Math.min(o.scroll || 0, o.maxScroll); }
      const s0 = o ? o.scroll : 0;
      lines.slice(s0, s0 + vis).forEach((l, i) => tx(c, l, 6, 78 + i * 10, PAL.text));
      if (s0 > 0) txr(c, '▲', 252, 78, PAL.gold); if (s0 + vis < lines.length) txr(c, '▼', 252, 178, PAL.gold);
    }
    if (S.toast) strip(c, S.toast.text, S.toast.col);
  }

  // ---------- views ----------
  const V = {}, OV = {}, TABS = [];
  V.boot = () => ({ key: 'boot', btns: [],
    top: c => { R(c, 0, 0, W, H, PAL.bg); icon(c, 'sword', 128 - 11, 26, [PAL.text], 2); sparkle(c, { t: S.t, seed: 5 }, 96, 22, 64, 30, PAL.gold); txc(c, 'POCKET SHEET', 0, W, 64, PAL.gold, 3); R(c, 60, 92, 56, 1, PAL.line); R(c, 140, 92, 56, 1, PAL.line); txc(c, '◆', 0, W, 89, PAL.gold); txc(c, 'CHARACTER REFERENCE', 0, W, 100, PAL.dim); txc(c, 'V0.3', 0, W, 114, PAL.dim); txc(c, 'GAME TERMS FROM SRD 5.1, CC-BY-4.0', 0, W, 176, PAL.dim); },
    bot: c => { const t = 'MOUNTING SD CARD'; const x = Math.floor((W - tw(t + '...')) / 2); tx(c, t + '.'.repeat(1 + ((S.t / 12) | 0) % 3), x, 92, PAL.text); } });

  V.select = () => {
    const btns = [];
    for (let i = 0; i < 3; i++) {
      const b = { id: 'slot' + i, x: 8, y: 20 + i * 46, w: 240, h: 40 }, k = S.slots[i];
      b.draw = (c, st) => {
        R(c, b.x, b.y, b.w, b.h, st.p ? PAL.panel2 : PAL.panel); R(c, b.x, b.y, 20, b.h, PAL.panel2); txc(c, String(i + 1), b.x, 20, b.y + 16, PAL.dim);
        if (k) { tx(c, k.name, b.x + 28, b.y + 9, PAL.text); tx(c, `${k.cls}  LV ${k.level}`, b.x + 28, b.y + 24, PAL.dim); const hs = k.hp + '/' + k.hpMax, as = String(k.ac), rx = b.x + b.w - 8; txr(c, hs, rx, b.y + 9, PAL.text); icon(c, 'mheart', rx - tw(hs) - 10, b.y + 9, [PAL.red]); txr(c, as, rx, b.y + 24, PAL.dim); icon(c, 'mshield', rx - tw(as) - 10, b.y + 24, [PAL.dim]); }
        else tx(c, '+ NEW CHARACTER', b.x + 28, b.y + 16, PAL.dim);
        if (st.f) focusBox(c, b);
      };
      b.act = () => k ? openSlot(i) : newCharFlow(i); btns.push(b);
    }
    const f = S.focus.select; if (f && f.startsWith('slot')) S.sel = +f.slice(4);
    const del = { id: 'del', x: 8, y: 158, w: 100, h: 18, disabled: !S.slots[S.sel] };
    del.draw = (c, st) => { const dis = del.disabled; R(c, del.x, del.y, del.w, del.h, st.p ? PAL.text : PAL.panel); tx(c, '✗', del.x + 6, del.y + 6, dis ? PAL.line : st.p ? PAL.ink : PAL.red); tx(c, 'DELETE...', del.x + 16, del.y + 6, dis ? PAL.line : st.p ? PAL.ink : PAL.text); if (st.f) focusBox(c, del); };
    del.act = () => askDelete(S.sel); btns.push(del);
    const cx = { key: 'select', btns, def: 'slot' + S.sel, footer: [['A', S.slots[S.sel] ? 'OPEN' : 'CREATE'], ['X', 'DELETE'], ['▲▼', 'CHOOSE']] };
    cx.top = selectTop; cx.bot = c => tx(c, 'CHOOSE A CHARACTER', 8, 7, PAL.dim);
    cx.onKeys = k => { if (k.down.X) { if (S.slots[S.sel]) askDelete(S.sel); return true; } return false; };
    return cx;
  };

  const TAB_ORDER = [0, 4, 3, 2, 1, 5];
  function sumData(k, P) {
    if (P.kind === 'spell') {
      const it = k.spells.find(x => x.name === P.name), sp = S.lib.get(P.name), prep = it && it.prep;
      if (!sp) return { title: P.name, icon: 'mspark', col: PAL.red, meta: 'NOT IN YOUR SPELL LIBRARY', desc: 'ADD IT TO /DATA/POCKETSHEET/SPELLS.TXT ON A PC (THE FILE EXPLAINS THE FORMAT), OR REMOVE IT FROM THIS CHARACTER.' };
      return { title: P.name, icon: 'mspark', col: PAL.blue, tag: sp.level === 0 ? 'CANTRIP' : prep ? '● PREPARED' : 'NOT PREPARED', tagCol: sp.level === 0 || prep ? PAL.gold : PAL.dim,
        meta: [sp.level ? ORD[sp.level - 1] + ' LEVEL' : sp.school, sp.time, sp.range, sp.conc ? 'CONC' : '', sp.ritual ? 'RITUAL' : ''].filter(Boolean).join('  '),
        meta2: [sp.level ? sp.school : '', sp.comp ? 'COMP ' + sp.comp : '', sp.dur ? 'DUR ' + sp.dur : ''].filter(Boolean).join('  '), desc: sp.desc };
    }
    if (P.kind === 'weapon') {
      const w = k.weapons[P.idx]; if (!w) return null;
      const ai = wAbIdx(k, w), parts = [];
      if (w.mastery) parts.push('MASTERY: ' + MASTERY[w.mastery] + '. ' + masteryDesc(k, w));
      if (w.props) { parts.push('PROPERTIES: ' + w.props + '.'); w.props.split(',').map(x => x.trim()).filter(Boolean).forEach(p => { const nm = p.replace(/\s*\(.*\)$/, '').replace(/\s+\d+$/, ''), f = S.flib.get(nm); if (f) parts.push(nm + ': ' + f.desc); }); }
      return { title: w.name, icon: /\//.test(w.range) || /AMMUNITION|BARREL/.test(w.props) ? 'arrow' : 'sword', col: PAL.gold, tag: sg(wAtk(k, w)) + ' TO HIT', tagCol: ai < 0 ? PAL.red : PAL.gold,
        meta: [wDmg(k, w) + ' ' + DTYPE[w.type], w.range ? 'RANGE ' + w.range : ''].filter(Boolean).join('   '),
        meta2: [ai < 0 ? 'NO SPELL ABILITY' : (w.ab === 2 ? 'FINESSE ' : w.ab === 3 ? 'SPELL ' : '') + AB[ai] + ' ' + sg(wMod(k, w)), w.prof ? 'PROF ' + sg(pb(k)) : 'NOT PROFICIENT', w.bonus ? 'HIT BONUS ' + sg(w.bonus) : ''].filter(Boolean).join('   '),
        desc: parts.join('\n') || 'NO MASTERY OR PROPERTIES SET.' };
    }
    const it = k.feats.find(x => x.name === P.name), f = S.flib.get(P.name), max = featMax(k, f);
    if (!f) return { title: P.name, icon: 'mbook', col: PAL.red, meta: 'NOT IN YOUR FEATURE LIBRARY', desc: 'ADD IT TO /DATA/POCKETSHEET/FEATURES.TXT ON A PC, OR REMOVE IT FROM THIS CHARACTER.' };
    return { title: P.name, icon: 'mbook', col: [PAL.gold, PAL.gold, PAL.green, PAL.dim, PAL.blue, PAL.red, PAL.line][f.cat], tag: max ? (max - (it ? it.used : 0)) + ' OF ' + max + ' LEFT' : '',
      meta: FCAT[f.cat] + (max ? '   RESETS: ' + RESETS[f.reset] : '') + (f.inspire ? '   INSPIRES' : ''), meta2: max && (f.uses === 'PB' || AB.includes(f.uses)) ? 'USES = ' + (f.uses === 'PB' ? 'PROFICIENCY BONUS' : f.uses + ' MODIFIER') : '', desc: f.desc };
  }
  // expanded page: same card language as the hover summary, using the whole top screen
  function drawPage(c, s, o) {
    R(c, 0, 0, W, H, PAL.bg); R(c, 0, 0, W, 178, PAL.panel); R(c, 0, 0, 2, 178, s.col || PAL.gold);
    if (s.icon) icon(c, s.icon, 6, Math.max(1, 5 + (7 - isz(s.icon)[1] >> 1)), [s.col || PAL.gold]);
    const nx = s.icon ? 6 + isz(s.icon)[0] + 4 : 6; tx(c, s.title, nx, 5, PAL.text);
    if (s.tag) txr(c, s.tag, 251, 5, s.tagCol || PAL.gold);
    tx(c, s.meta.slice(0, 41), 6, 16, PAL.dim); let ry = 26;
    if (s.meta2) { tx(c, s.meta2.slice(0, 41), 6, 26, PAL.dim); ry = 36; }
    R(c, 6, ry, 244, 1, PAL.line);
    const lines = String(s.desc || '').split('\n').flatMap((p, i) => (i ? [''] : []).concat(wrap(p, 41))), y0 = ry + 4, vis = Math.floor((164 - y0) / 10);
    o.maxScroll = Math.max(0, lines.length - vis); o.scroll = Math.min(o.scroll || 0, o.maxScroll);
    lines.slice(o.scroll, o.scroll + vis).forEach((l, i) => tx(c, l, 6, y0 + i * 10, PAL.text));
    if (o.maxScroll) { tx(c, (o.scroll > 0 ? '▲' : ' ') + (o.scroll < o.maxScroll ? '▼' : ''), 6, 167, PAL.gold); tx(c, 'L/R SCROLL', 22, 167, PAL.dim); }
    txr(c, 'X: CLOSE', 251, 167, PAL.gold);
    if (S.toast) strip(c, S.toast.text, S.toast.col); else { R(c, 0, 178, W, 14, PAL.bg); txc(c, ch().name, 0, W, 182, PAL.dim); }
  }
  function peekFor(k, b) {
    if (!b) return null;
    if (S.tab === 3 && b.id.startsWith('srow')) { const it = mySpells(k)[+b.id.slice(4)]; return it ? { kind: 'spell', name: it.name } : null; }
    if (S.tab === 4 && (b.id.startsWith('frow') || b.id.startsWith('ft'))) { const fl = myFeats(k), tr = fl.filter(x => featMax(k, x.f) > 0).slice(0, 4), it = b.id.startsWith('frow') ? fl[+b.id.slice(4)] : tr[+b.id.slice(2)]; return it ? { kind: 'feat', name: it.name } : null; }
    if (S.tab === 2 && /^w\d+$/.test(b.id) && k.weapons[+b.id.slice(1)]) return { kind: 'weapon', idx: +b.id.slice(1) };
    return null;
  }
  V.sheet = () => {
    const k = ch(), btns = [];
    [['SKILLS', 'mpage', 0], ['FEATS', 'mbook', 4], ['MAGIC', 'mspark', 3], ['ARMS', 'msword', 2], ['FIGHT', 'mheart', 1], ['', 'mquill', 5]].forEach(([n, ic, i], p, arr) => {
      const ws = arr.map(([l]) => l ? 10 + tw(l) + 9 : 21), extra = W - ws.reduce((a, v) => a + v, 0); ws[0] += extra - 4 * Math.floor(extra / 5); for (let j = 1; j < 5; j++) ws[j] += Math.floor(extra / 5);
      const b = { id: 'tab' + i, x: ws.slice(0, p).reduce((a, v) => a + v, 0), y: 0, w: ws[p], h: 16, nav: false };
      b.draw = (c, st) => { const on = S.tab === i, col = on ? PAL.ink : PAL.dim; R(c, b.x, 0, b.w - 1, 16, on ? (i === 5 ? PAL.blue : PAL.gold) : st.p ? PAL.panel2 : PAL.panel); const x0 = b.x + ((b.w - 1 - (n ? 10 + tw(n) : 7)) >> 1); icon(c, ic, x0, 4, [on ? PAL.ink : i === 1 ? PAL.red : i === 3 ? PAL.blue : i === 4 ? PAL.gold : PAL.dim]); tx(c, n, x0 + 10, 5, col); };
      b.act = () => { S.tab = i; S.fpeek = null; }; btns.push(b);
    });
    const t = TABS[S.tab](k, btns);
    const cx = { key: 'sheet' + S.tab + (t.sub || ''), btns, footer: S.fpeek ? [['▲▼', 'BROWSE'], ['L/R', 'SCROLL'], ['X/B', 'CLOSE']] : t.footer, edit: !!t.edit, def: t.def, bot: t.bot };
    cx.top = c => {
      if (S.fpeek) {
        const P = S.fpeek, pk = peekFor(k, focused(cx)), kind = P.kind || 'feat';
        if (pk && pk.kind === kind && (pk.name !== P.name || pk.idx !== P.idx)) Object.assign(P, pk, { scroll: 0 });
        const sd = sumData(k, { ...P, kind }); if (!sd) { S.fpeek = null; return overviewTop(c); }
        return drawPage(c, sd, P);
      }
      const b = focused(cx), pk = peekFor(k, b), sum = pk ? sumData(k, pk) : null;
      if (sum) sum.hint = b.id.startsWith('ft') ? 'A: USE  X: FULL SCREEN' : 'X: FULL SCREEN';
      overviewTop(c, null, sum); };
    cx.onKeys = kk => {
      if (S.fpeek) {
        if (kk.down.B || kk.down.X) { S.fpeek = null; return true; }
        if (kk.rep.L) { S.fpeek.scroll = Math.max(0, (S.fpeek.scroll || 0) - 2); return true; }
        if (kk.rep.R) { S.fpeek.scroll = Math.min(S.fpeek.maxScroll || 0, (S.fpeek.scroll || 0) + 2); return true; }
      } else if (kk.down.X) { const pk = peekFor(k, focused(cx)); if (pk) { S.fpeek = { ...pk, scroll: 0 }; return true; } }
      if (t.onKeys && t.onKeys(kk)) return true;
      if (kk.down.L || kk.down.R) { const O = TAB_ORDER, p = O.indexOf(S.tab); S.tab = O[(p + (kk.down.L ? O.length - 1 : 1)) % O.length]; S.fpeek = null; return true; }
      if (kk.down.START) { pushOv({ type: 'menu' }); return true; }
      if (kk.down.SELECT) { if (S.tab === 5) S.tab = S.prevTab; else { S.prevTab = S.tab; S.tab = 5; } return true; }
      if (kk.down.X) { if (S.tab === 5) S.editPage = (S.editPage + 1) % 3; else if (S.tab === 4) openFeatPicker(); else if (S.tab === 3) openPicker(k); else if (S.tab === 2) addWeapon(k); return true; }
      if (kk.down.B) { if (S.tab === 1 && S.amt) S.amt = ''; return true; }
      return false;
    };
    return cx;
  };

  // SHEET: every skill at once; focused skill's breakdown in the strip
  TABS[0] = (k, btns) => {
    SK.forEach((s, i) => btns.push(skillRow({ id: 'sk' + i, x: 3 + (i < 9 ? 0 : 126), y: 17 + (i % 9) * 16, w: 124, h: 16 }, k, i)));
    return { def: 'sk0', footer: [['▲▼', 'BROWSE'], ['L/R', 'TABS'], ['ST', 'MENU'], ['SE', 'EDIT']],
      bot: c => {
        const f = S.focus.sheet0, i = f && f.startsWith('sk') ? +f.slice(2) : 0, ab = SK[i][1], p = k.skill[i], tot = skMod(k, i);
        R(c, 3, 163, 250, 15, PAL.panel2);
        const exp = i === 11 ? ` + EXPERT ${sg(2 * pb(k))}` : ` + EXPERTISE ${sg(2 * pb(k))} (2X PROF)`;
        tx(c, `${AB[ab]} ${sg(mod(k.ab[ab]))}` + (p === 2 ? exp : p ? ` + PROF ${sg(pb(k))}` : '  NOT PROFICIENT') + ` = ${sg(tot)}` + (i === 11 ? `  PASSIVE ${10 + tot}` : ''), 7, 167, PAL.text);
      } };
  };

  // COMBAT: HP management
  TABS[1] = (k, btns) => {
    const n = +S.amt || 0;
    const insp = { id: 'insp', x: 174, y: 19, w: 79, h: 22 };
    insp.draw = (c, st) => { R(c, insp.x, insp.y, insp.w, insp.h, st.p ? PAL.panel2 : PAL.panel); icon(c, 'star', insp.x + 4, insp.y + 5, [k.insp ? PAL.gold : PAL.line]); tx(c, k.insp ? 'INSPIRED' : 'UNINSPIRED', insp.x + 19, insp.y + 8, k.insp ? PAL.gold : PAL.dim); if (st.f) focusBox(c, insp); };
    insp.act = () => { k.insp ^= 1; S.dirty = true; if (k.insp) { toast('INSPIRED!', PAL.gold); addFx({ k: 'sparkle', t: 40, rect: [2, 178, 90, 14], col: PAL.gold }); } else toast('INSPIRATION SPENT', PAL.dim); };
    btns.push(insp);
    ['7', '8', '9', '4', '5', '6', '1', '2', '3', 'CLR', '0', 'DEL'].forEach((l, j) => {
      const b = plain({ id: 'k' + l, x: 3 + (j % 3) * 31, y: 67 + Math.floor(j / 3) * 24, w: 29, h: 22 }, l, { sc: l.length === 1 ? 2 : 1 });
      b.act = () => { if (l === 'CLR') S.amt = ''; else if (l === 'DEL') S.amt = S.amt.slice(0, -1); else if (S.amt.length < 3) S.amt = (S.amt === '0' ? '' : S.amt) + l; };
      btns.push(b);
    });
    [['dmg', 'DAMAGE', 44, 24, PAL.red, () => applyDmg(k, n), 'sword'], ['heal', 'HEAL', 70, 24, PAL.green, () => applyHeal(k, n), 'heart'], ['temp', 'SET TEMP HP', 96, 18, PAL.blue, () => applyTemp(k, n), 'shield']].forEach(([id, l, y, h, col, f, ic]) => {
      const b = iconBtn({ id, x: 100, y, w: 153, h }, ic, [l], n ? { bg: col, fg: PAL.ink, ic1: 'bg', sc: h >= 24 ? 2 : 1 } : { bg: PAL.panel2, fg: PAL.dim, ic: col, ic1: 'bg', sc: h >= 24 ? 2 : 1 }); b.act = f; btns.push(b);
    });
    [['dss', 'SAVES', 'ds', PAL.green], ['dsf', 'FAILS', 'df', PAL.red]].forEach(([id, l, f, col], j) => {
      const b = { id, x: 100 + j * 78, y: 127, w: 75, h: 18 };
      b.draw = (c, st) => { R(c, b.x, b.y, b.w, b.h, st.p ? PAL.panel2 : PAL.panel); tx(c, l, b.x + 5, b.y + 6, PAL.dim); for (let p = 0; p < 3; p++) { if (p < k[f] && f === 'df') icon(c, 'mskull', b.x + 44 + p * 9, b.y + 6, [col]); else tx(c, p < k[f] ? '●' : '○', b.x + 44 + p * 9, b.y + 6, p < k[f] ? col : PAL.line); } if (st.f) focusBox(c, b); };
      b.act = () => { k[f] = (k[f] + 1) % 4; S.dirty = true; }; btns.push(b);
    });
    const sr = iconBtn({ id: 'srest', x: 100, y: 150, w: 75, h: 24 }, () => (S.frame >> 3) & 1 ? 'fire2' : 'fire', ['SHORT', 'REST'], { ic: PAL.gold, ic1: PAL.text, ic2: PAL.dim }); sr.act = () => openShortRest(k);
    const lr = iconBtn({ id: 'lrest', x: 178, y: 150, w: 75, h: 24 }, 'moon', ['LONG', 'REST'], { ic: PAL.text, ic1: PAL.gold }); lr.act = () => askLongRest(k);
    btns.push(sr, lr);
    return { def: 'k7', footer: [['A', 'PRESS'], ['B', 'CLEAR'], ['L/R', 'TABS'], ['ST', 'MENU']],
      bot: c => {
        R(c, 3, 19, 168, 22, PAL.panel); icon(c, k.hp === 0 ? 'skull' : 'heart', 7, 25, [PAL.red, PAL.panel]); tx(c, `${k.hp}/${k.hpMax}`, 24, 23, k.hp === 0 ? PAL.red : PAL.text, 2); if (k.temp) { const ts = '+' + k.temp; txr(c, ts, 166, 27, PAL.blue); icon(c, 'mshield', 166 - tw(ts) - 10, 27, [PAL.blue]); }
        R(c, 3, 44, 91, 20, PAL.panel); tx(c, 'AMT', 8, 51, PAL.dim); txr(c, S.amt || '0', 89, 47, S.amt ? PAL.text : PAL.line, 2);
        icon(c, 'mskull', 100, 118, [k.hp === 0 ? PAL.red : PAL.dim]); tx(c, 'DEATH SAVES', 110, 118, k.hp === 0 ? PAL.red : PAL.dim); tx(c, `HIT DICE ${k.level - k.hdUsed}/${k.level} D${k.hitDie}`, 4, 167, PAL.dim);
      } };
  };

  // WEAPONS: up to 4 attacks, full numbers at a glance; A edits, X adds
  function addWeapon(k) {
    if (k.weapons.length >= NW) return toast(`${NW} WEAPONS MAX. EDIT OR DELETE ONE.`, PAL.gold);
    kbd({ title: 'WEAPON NAME', field: 'NAME', max: 12, onOk: v => { popOv(); k.weapons.push(newWeapon(v)); S.dirty = true; S.focus.sheet2 = 'w' + (k.weapons.length - 1); pushOv({ type: 'wdetail', idx: k.weapons.length - 1 }); pushOv({ type: 'weapon', idx: k.weapons.length - 1 }); } });
  }
  const DT_SHORT = ['SLASH', 'PIERCE', 'BLUDGEON', 'FIRE', 'COLD', 'LIGHTNING', 'THUNDER', 'ACID', 'POISON', 'NECROTIC', 'RADIANT', 'FORCE', 'PSYCHIC'];
  TABS[2] = (k, btns) => {
    k.weapons.forEach((w, i) => {
      const b = { id: 'w' + i, x: 3, y: 18 + i * 38, w: 250, h: 36 };
      b.draw = (c, st) => {
        const x = b.x, y = b.y, ai = wAbIdx(k, w);
        R(c, x, y, b.w, b.h, st.p ? PAL.panel2 : PAL.panel);
        // row 1: identity
        icon(c, /\//.test(w.range) || /AMMUNITION|BARREL/.test(w.props) ? 'arrow' : 'sword', x + 5, y + 4, [PAL.dim]); tx(c, w.name, x + 20, y + 5, PAL.text);
        if (w.mastery) { const mx = x + 20 + tw(w.name) + 8; tx(c, '◆', mx, y + 5, PAL.blue); tx(c, MASTERY[w.mastery], mx + 8, y + 5, PAL.blue); }
        if (w.range) txr(c, w.range, x + b.w - 6, y + 5, PAL.dim);
        // row 2: the two numbers you need
        const a = sg(wAtk(k, w)); tx(c, a, x + 6, y + 17, ai < 0 ? PAL.red : PAL.gold, 2); tx(c, 'HIT', x + 6 + tw(a, 2) + 4, y + 24, PAL.dim);
        const d = wDmg(k, w), dx = x + 76; tx(c, d, dx, y + 17, PAL.text, 2); tx(c, DT_SHORT[w.type], dx + tw(d, 2) + 5, y + 24, PAL.dim);
        if (!w.prof) txr(c, 'NOT PROF', x + b.w - 6, y + 24, PAL.dim);
        if (st.f) focusBox(c, b);
      };
      b.act = () => pushOv({ type: 'wdetail', idx: i }); btns.push(b);
    });
    if (k.weapons.length < NW) {
      const i = k.weapons.length, b = { id: 'wadd', x: 3, y: 18 + i * 38, w: 250, h: i ? 22 : 36 };
      b.draw = (c, st) => { R(c, b.x, b.y, b.w, b.h, st.p ? PAL.text : PAL.bg); BOX(c, b.x, b.y, b.w, b.h, st.p ? PAL.text : PAL.line, 1); txc(c, '+ ADD WEAPON', b.x, b.w, b.y + ((b.h - 7) >> 1), st.p ? PAL.ink : PAL.dim); if (st.f) focusBox(c, b); };
      b.act = () => addWeapon(k); btns.push(b);
    }
    return { def: k.weapons.length ? 'w0' : 'wadd', footer: [['A', k.weapons.length ? 'DETAILS' : 'ADD'], ['X', 'EXPAND'], ['L/R', 'TABS'], ['ST', 'MENU']],
      bot: c => { if (!k.weapons.length) { txc(c, 'NO WEAPONS YET', 0, W, 80, PAL.text); txc(c, 'ADD UP TO 4. TO-HIT AND DAMAGE ARE', 0, W, 96, PAL.dim); txc(c, 'WORKED OUT FROM YOUR SCORES.', 0, W, 106, PAL.dim); } } };
  };
  // weapon details: full card on the top screen, actions below
  function weaponLines(k, w) {
    const L = [];
    if (w.mastery) { L.push({ t: '◆ MASTERY: ' + MASTERY[w.mastery], col: PAL.blue }); masteryDesc(k, w).split('\n').forEach((p, i) => { if (i) L.push({ t: '' }); wrap(p, 41).forEach(l => L.push({ t: l })); }); }
    else L.push({ t: 'NO MASTERY SET', col: PAL.dim });
    if (w.props) {
      L.push({ t: '' }); wrap('PROPERTIES: ' + w.props, 41).forEach(l => L.push({ t: l, col: PAL.gold }));
      w.props.split(',').map(s => s.trim()).filter(Boolean).forEach(p => { const nm = p.replace(/\s*\(.*\)$/, '').replace(/\s+\d+$/, ''), f = S.flib.get(nm); if (!f) return; L.push({ t: '' }); f.desc.split('\n').forEach((para, i) => wrap((i ? '' : nm + ': ') + para, 41).forEach((l, j) => L.push({ t: l, head: i === 0 && j === 0 ? nm.length + 1 : 0 }))); });
    }
    return L;
  }
  function weaponTop(c, k, w, o) {
    R(c, 0, 0, W, H, PAL.bg); const ai = wAbIdx(k, w);
    R(c, 0, 0, W, 32, PAL.panel); tx(c, w.name, 6, 4, PAL.text, 2); icon(c, /\//.test(w.range) || /AMMUNITION|BARREL/.test(w.props) ? 'arrow' : 'sword', 230, 5, [PAL.gold], 2);
    tx(c, [ai < 0 ? 'NO SPELL ABILITY SET' : (w.ab === 2 ? 'FINESSE (' + AB[ai] + ')' : w.ab === 3 ? 'SPELL (' + AB[ai] + ')' : AB[ai]), w.prof ? 'PROFICIENT' : 'NOT PROFICIENT', w.bonus ? sg(w.bonus) + ' HIT BONUS' : ''].filter(Boolean).join('   '), 6, 21, ai < 0 ? PAL.red : PAL.dim);
    R(c, 1, 35, 82, 40, PAL.panel); tx(c, 'TO HIT', 6, 39, PAL.dim); tx(c, sg(wAtk(k, w)), 6, 50, PAL.gold, 3);
    const d = wDmg(k, w), big = tw(d, 3) <= 160 ? 3 : 2;
    R(c, 85, 35, 170, 40, PAL.panel); tx(c, 'DAMAGE', 90, 39, PAL.dim); txr(c, DTYPE[w.type], 250, 39, PAL.dim); tx(c, d, 90, big === 3 ? 50 : 54, PAL.text, big);
    const m = wMod(k, w), parts = [['ABILITY', ai < 0 ? '-' : sg(m)], ['PROF', w.prof ? sg(pb(k)) : '-'], ['RANGE', w.range || '-']];
    parts.forEach(([l, v], i) => { const x = 1 + i * 85; R(c, x, 77, 83, 16, PAL.panel); tx(c, l, x + 5, 82, PAL.dim); txr(c, v, x + 78, 82, PAL.text); });
    const lines = weaponLines(k, w), vis = 8;
    if (o) { o.maxScroll = Math.max(0, lines.length - vis); o.scroll = Math.min(o.scroll || 0, o.maxScroll); }
    const s0 = o ? o.scroll || 0 : 0;
    lines.slice(s0, s0 + vis).forEach((l, i) => { const y = 98 + i * 10; if (l.head) { tx(c, l.t.slice(0, l.head), 6, y, PAL.gold); tx(c, l.t.slice(l.head), 6 + l.head * 6, y, PAL.text); } else tx(c, l.t, 6, y, l.col || PAL.text); });
    if (s0 > 0) txr(c, '▲', 252, 98, PAL.gold); if (s0 + vis < lines.length) txr(c, '▼', 252, 168, PAL.gold);
    if (S.toast) strip(c, S.toast.text, S.toast.col);
  }
  OV.wdetail = o => {
    const k = ch(), w = k.weapons[o.idx]; if (!w) { S.ov.pop(); return ctxNow(); }
    const n = k.weapons.length, btns = [];
    const ed = plain({ id: 'wdedit', x: 3, y: 22, w: 250, h: 28 }, 'EDIT WEAPON', { bg: PAL.gold, fg: PAL.ink }); ed.act = () => pushOv({ type: 'weapon', idx: o.idx }); btns.push(ed);
    if (n > 1) {
      const pv = plain({ id: 'wdprev', x: 3, y: 56, w: 123, h: 26 }, '◀ PREVIOUS'); pv.act = () => { o.idx = (o.idx + n - 1) % n; o.scroll = 0; S.focus.sheet2 = 'w' + o.idx; };
      const nx = plain({ id: 'wdnext', x: 130, y: 56, w: 123, h: 26 }, 'NEXT ▶'); nx.act = () => { o.idx = (o.idx + 1) % n; o.scroll = 0; S.focus.sheet2 = 'w' + o.idx; };
      btns.push(pv, nx);
    }
    const back = plain({ id: 'wdback', x: 153, y: 156, w: 100, h: 20 }, 'BACK'); back.act = () => popOv(); btns.push(back);
    const cx = { key: 'wd' + o.uid, btns, def: 'wdedit', footer: [['A', 'PRESS'], ['L/R', 'SCROLL'], ['B', 'BACK']] };
    cx.bot = c => {
      R(c, 0, 0, W, 16, PAL.panel); tx(c, 'WEAPON', 6, 5, PAL.gold); txr(c, (o.idx + 1) + ' OF ' + n, 250, 5, PAL.dim);
      R(c, 3, 88, 250, 64, PAL.panel); tx(c, w.name, 9, 94, PAL.text);
      tx(c, sg(wAtk(k, w)) + ' TO HIT', 9, 108, PAL.gold); tx(c, wDmg(k, w) + ' ' + DTYPE[w.type], 9, 120, PAL.text);
      tx(c, w.mastery ? '◆ ' + masteryTag(k, w) : 'NO MASTERY', 9, 132, w.mastery ? PAL.blue : PAL.dim); if (w.props) tx(c, w.props.slice(0, 40), 9, 143, PAL.dim);
      tx(c, 'DETAILS ON TOP SCREEN', 6, 163, PAL.dim);
    };
    cx.top = c => weaponTop(c, k, w, o);
    cx.onKeys = kk => {
      if (kk.down.B) { popOv(); return true; }
      if (kk.rep.L) { o.scroll = Math.max(0, (o.scroll || 0) - 2); return true; }
      if (kk.rep.R) { o.scroll = Math.min(o.maxScroll || 0, (o.scroll || 0) + 2); return true; }
      return false;
    };
    return cx;
  };
  OV.weapon = o => {
    const k = ch(), w = k.weapons[o.idx]; if (!w) { S.ov.pop(); return ctxNow(); }
    const mark = () => { S.dirty = true; };
    const fields = [
      ['NAME', () => w.name, () => kbd({ title: 'WEAPON NAME', field: 'NAME', val: w.name, max: 12, onOk: v => { w.name = v; mark(); popOv(); } }), true],
      ['ABILITY', () => (w.ab === 2 ? 'FINESSE (' + AB[wAbIdx(k, w)] + ')' : w.ab === 3 ? 'SPELL (' + (k.spellAb === 255 ? 'NONE' : AB[k.spellAb]) + ')' : WAB[w.ab]), () => { w.ab = (w.ab + 1) % 4; mark(); }],
      ['PROFICIENT', () => w.prof ? 'YES' : 'NO', () => { w.prof ^= 1; mark(); }],
      ['HIT BONUS', () => w.bonus ? sg(w.bonus) : 'NONE', () => stepper({ title: 'TO-HIT BONUS (MAGIC, ARCHERY)', min: -5, max: 10, big: 1, get: () => w.bonus, set: v => { w.bonus = v; } }), true],
      ['DAMAGE DICE', () => w.n + ' DICE', () => stepper({ title: 'NUMBER OF DICE', min: 1, max: 9, big: 1, get: () => w.n, set: v => { w.n = v; } }), true],
      ['DIE SIZE', () => 'D' + w.die, () => { w.die = DIE[(DIE.indexOf(w.die) + 1) % DIE.length]; mark(); }],
      ['DAMAGE BONUS', () => w.dmg ? sg(w.dmg) : 'NONE', () => stepper({ title: 'DAMAGE BONUS (ADDED TO DICE)', min: -10, max: 30, big: 5, get: () => w.dmg, set: v => { w.dmg = v; } }), true],
      ['DAMAGE TYPE', () => DTYPE[w.type], () => { w.type = (w.type + 1) % DTYPE.length; mark(); }],
      ['MASTERY', () => MASTERY[w.mastery], () => { w.mastery = (w.mastery + 1) % MASTERY.length; mark(); }],
      ['RANGE', () => w.range || '-', () => kbd({ title: 'RANGE (E.G. 5 FT, 80/320)', field: 'RANGE', val: w.range, max: 7, allowEmpty: true, onOk: v => { w.range = v; mark(); popOv(); } }), true],
      ['PROPERTIES', () => w.props ? w.props.slice(0, 18) + (w.props.length > 18 ? '..' : '') : '-', () => kbd({ title: 'PROPERTIES, COMMA SEPARATED', field: 'PROPERTIES', val: w.props, max: 28, allowEmpty: true, onOk: v => { w.props = v; mark(); popOv(); } }), true],
    ];
    const btns = [], WL = listRows(btns, { pre: 'wf', fkey: 'wep' + o.uid, items: fields, x: 3, y: 18, w: 226, h: 16, rows: 9, act: i => fields[i][2](),
      draw: (c, b, st, it) => { const h = b.h - 1; R(c, b.x, b.y, b.w, h, st.p ? PAL.panel2 : PAL.panel); tx(c, it[0], b.x + 6, b.y + 4, PAL.dim); txr(c, it[1](), b.x + b.w - (it[3] ? 16 : 6), b.y + 4, PAL.gold); if (it[3]) tx(c, '▶', b.x + b.w - 10, b.y + 4, PAL.dim); if (st.f) focusBox(c, { x: b.x, y: b.y, w: b.w, h }); } });
    const del = { id: 'wdel', x: 3, y: 163, w: 100, h: 16 };
    del.draw = (c, st) => { R(c, del.x, del.y, del.w, del.h, st.p ? PAL.text : PAL.panel); tx(c, '✗', del.x + 6, del.y + 5, st.p ? PAL.ink : PAL.red); tx(c, 'DELETE...', del.x + 16, del.y + 5, st.p ? PAL.ink : PAL.text); if (st.f) focusBox(c, del); };
    del.act = () => pushNotice({ title: `DELETE ${w.name}?`, tone: 'err', sub: 'REMOVES THIS WEAPON', lines: ['THIS REMOVES THE WEAPON FROM THIS CHARACTER.'], btns: [{ label: 'KEEP' }, { label: 'DELETE', danger: true, act: () => { k.weapons.splice(o.idx, 1); mark(); while (S.ov.length && S.ov[S.ov.length - 1].type !== 'menu') S.ov.pop(); S.press = null; toast('WEAPON DELETED', PAL.gold); } }], def: 0, cancel: 0 });
    const done = plain({ id: 'wdone', x: 153, y: 163, w: 100, h: 16 }, 'DONE', { bg: PAL.gold, fg: PAL.ink }); done.act = () => popOv();
    btns.push(del, done);
    const cx = { key: 'wep' + o.uid, btns, def: 'wf0', edit: true, footer: [['A', 'CHANGE'], ['B', 'DONE'], ['▲▼', 'FIELD']] };
    cx.bot = c => { R(c, 0, 0, W, 16, PAL.panel); tx(c, 'EDIT WEAPON', 6, 5, PAL.gold); txr(c, `${o.idx + 1} OF ${k.weapons.length}`, 250, 5, PAL.dim); };
    cx.top = c => weaponTop(c, k, w, o);
    cx.onKeys = kk => { if (kk.down.B || kk.down.START) { popOv(); return true; } return WL.keys(kk); };
    return cx;
  };

  // SPELLS: one screen. Slot strip (tap a level to spend, SET to change maximums) + known spell list (A opens the description)
  TABS[3] = (k, btns) => {
    const list = mySpells(k), np = list.filter(s => s.prep && s.sp && s.sp.level > 0).length;
    const lv = [0, 1, 2, 3, 4, 5, 6, 7, 8].filter(l => k.slotMax[l] > 0);
    const cw = 25, gap = 2;
    lv.forEach((l, j) => {
      const b = { id: 'sl' + l, x: 3 + j * (cw + gap), y: 18, w: cw, h: 28 };
      b.draw = (c, st) => {
        const m = k.slotMax[l], av = m - k.slotUsed[l];
        R(c, b.x, b.y, b.w, b.h, st.p ? PAL.panel2 : PAL.panel); txc(c, String(l + 1), b.x, b.w, b.y + 4, av ? PAL.text : PAL.line);
        const gw = m * 5 - 1, gx = b.x + ((b.w - gw) >> 1);
        for (let i = 0; i < m; i++) { if (i < av) R(c, gx + i * 5, b.y + 15, 4, 8, PAL.blue); else BOX(c, gx + i * 5, b.y + 15, 4, 8, PAL.dim, 1); }
        if (st.f) focusBox(c, b);
      };
      b.act = () => { if (k.slotMax[l] - k.slotUsed[l] > 0) { k.slotUsed[l]++; S.dirty = true; toast(ORD[l] + ' LEVEL: ' + (k.slotMax[l] - k.slotUsed[l]) + ' OF ' + k.slotMax[l] + ' LEFT', PAL.blue); } else toast('NO ' + ORD[l] + ' LEVEL SLOTS LEFT. Y RESTORES ONE', PAL.gold); };
      b.onY = () => { if (k.slotUsed[l] > 0) { k.slotUsed[l]--; S.dirty = true; toast(ORD[l] + ' LEVEL: RESTORED (' + (k.slotMax[l] - k.slotUsed[l]) + ' LEFT)', PAL.blue); } };
      btns.push(b);
    });
    const set = plain({ id: 'setslots', x: 3 + lv.length * (cw + gap), y: 18, w: lv.length ? 34 : 120, h: 28 }, lv.length ? 'SET' : 'SET SLOTS'); set.act = () => pushOv({ type: 'slotset' }); btns.push(set);
    const add = plain({ id: 'spadd', x: 205, y: 18, w: 48, h: 28 }, '+ ADD'); add.act = () => openPicker(k); btns.push(add);
    listRows(btns, { pre: 'srow', fkey: 'sheet3', items: list, x: 3, y: 49, w: 226, h: 16, rows: 7, draw: (c, b, st, it) => spellRow(c, b, st, it, true),
      act: i => pushOv({ type: 'spell', names: list.map(s => s.name), i, scroll: 0 }), onY: i => togglePrep(k, list[i].name) });
    const L = { keys: kk => {
      const f = S.focus.sheet3; if (!f || !f.startsWith('srow')) return false; const j = +f.slice(4);
      if (kk.rep.DOWN) { if (j < list.length - 1) S.focus.sheet3 = 'srow' + (j + 1); return true; }
      if (kk.rep.UP) { if (j > 0) { S.focus.sheet3 = 'srow' + (j - 1); return true; } S.focus.sheet3 = lv.length ? 'sl' + lv[0] : 'setslots'; return true; }
      return false;
    } };
    const f = S.focus.sheet3 || '', onSlot = f.startsWith('sl') && f !== 'slotset';
    return { def: list.length ? 'srow0' : lv.length ? 'sl' + lv[0] : 'spadd', onKeys: L.keys,
      footer: onSlot ? [['A', 'SPEND'], ['Y', 'RESTORE'], ['X', 'ADD SPELL'], ['L/R', 'TABS']] : list.length ? [['A', 'DETAILS'], ['Y', 'PREPARE'], ['X', 'EXPAND'], ['L/R', 'TABS']] : [['A', 'PRESS'], ['X', 'ADD SPELL'], ['L/R', 'TABS']],
      bot: c => {
        if (!list.length) { R(c, 3, 49, 250, 112, PAL.panel); txc(c, 'NO SPELLS YET', 3, 250, 86, PAL.text); txc(c, 'TAP + ADD (OR PRESS X) TO PICK', 3, 250, 102, PAL.dim); txc(c, 'FROM YOUR LIBRARY OF ' + S.lib.size + ' SPELLS.', 3, 250, 112, PAL.dim); }
        let x = 4; [['●', np + ' PREPARED', PAL.gold], ['◆', 'CONC', PAL.blue], ['R', 'RITUAL', PAL.gold]].forEach(([g, l, col]) => { tx(c, g, x, 169, col); tx(c, l, x + 8, 169, PAL.dim); x += 8 + tw(l) + 12; });
      } };
  };
  OV.slotset = o => {
    const k = ch(), btns = [];
    for (let l = 0; l < 9; l++) {
      const rx = 3 + (l < 5 ? 0 : 126), ry = 20 + (l < 5 ? l : l - 5) * 30, b = { id: 'slr' + l, x: rx, y: ry, w: 124, h: 28 };
      b.draw = (c, st) => { R(c, rx, ry, 124, 28, st.p ? PAL.panel2 : PAL.panel); tx(c, ORD[l], rx + 5, ry + 11, PAL.dim); tx(c, 'MAX ' + k.slotMax[l], rx + 32, ry + 11, k.slotMax[l] ? PAL.gold : PAL.line); tx(c, '▶', rx + 114, ry + 11, PAL.dim); if (st.f) focusBox(c, b); };
      b.act = () => stepper({ title: ORD[l] + ' LEVEL SLOTS', min: 0, max: 4, get: () => k.slotMax[l], set: v => { k.slotMax[l] = v; k.slotUsed[l] = Math.min(k.slotUsed[l], v); } });
      btns.push(b);
    }
    const done = plain({ id: 'ssdone', x: 129, y: 140, w: 124, h: 28 }, 'DONE', { bg: PAL.gold, fg: PAL.ink }); done.act = () => popOv(); btns.push(done);
    const cx = { key: 'ss' + o.uid, btns, def: 'slr0', edit: true, footer: [['A', 'SET MAX'], ['B', 'DONE']] };
    cx.bot = c => { R(c, 0, 0, W, 16, PAL.panel); tx(c, 'SPELL SLOTS: MAXIMUMS', 6, 5, PAL.gold); };
    cx.top = c => overviewTop(c, 'SET HOW MANY SLOTS YOU HAVE AT EACH LEVEL');
    cx.onKeys = kk => { if (kk.down.B || kk.down.START) { popOv(); return true; } return false; };
    return cx;
  };

  // FEATS: trackers (features with uses) as chips, then every feature; A opens the full text
  const myFeats = k => k.feats.map(f => ({ ...f, f: S.flib.get(f.name) })).sort((a, b) => ((a.f ? a.f.cat : 9) - (b.f ? b.f.cat : 9)) || (a.name < b.name ? -1 : 1));
  function spendFeat(k, name, d) {
    const f = k.feats.find(x => x.name === name), lf = S.flib.get(name), max = featMax(k, lf); if (!f || !max) return;
    if (d > 0) { if (f.used >= max) return toast('NO ' + name + ' LEFT', PAL.gold); f.used++; } else { if (f.used <= 0) return toast(name + ' IS FULL', PAL.dim); f.used--; }
    S.dirty = true; toast(name + ': ' + (max - f.used) + ' OF ' + max + ' LEFT', PAL.gold);
  }
  function openFeatPicker() { S.scroll.fk = 0; pushOv({ type: 'featpick', filter: -1, scroll: 0, lastF: null }); }
  function featPips(c, x, y, max, used, col) { if (max > 8) { txr(c, (max - used) + '/' + max, x, y, col); return; } for (let i = 0; i < max; i++) { const px = x - (max - i) * 7 + 2; if (i < max - used) R(c, px, y, 5, 7, col); else BOX(c, px, y, 5, 7, PAL.dim, 1); } }
  TABS[4] = (k, btns) => {
    const list = myFeats(k), tracked = list.filter(x => featMax(k, x.f) > 0).slice(0, 4), rowsT = Math.ceil(tracked.length / 2);
    tracked.forEach((x, j) => {
      const max = featMax(k, x.f), b = { id: 'ft' + j, x: 3 + (j % 2) * 126, y: 18 + Math.floor(j / 2) * 26, w: 124, h: 24 };
      b.draw = (c, st) => { R(c, b.x, b.y, b.w, b.h, st.p ? PAL.panel2 : PAL.panel); tx(c, x.name.slice(0, 19), b.x + 5, b.y + 4, PAL.text); tx(c, RESETS[x.f.reset] || '', b.x + 5, b.y + 14, PAL.dim); featPips(c, b.x + b.w - 5, b.y + 14, max, x.used, PAL.gold); if (st.f) focusBox(c, b); };
      b.act = () => spendFeat(k, x.name, 1); b.onY = () => spendFeat(k, x.name, -1); btns.push(b);
    });
    const y0 = 18 + rowsT * 26 + (rowsT ? 1 : 0), nrows = Math.max(3, Math.floor((162 - y0) / 16));
    const L = listRows(btns, { pre: 'frow', fkey: 'sheet4', items: list, x: 3, y: y0, w: 226, h: 16, rows: nrows, cols: 2,
      act: i => { S.fpeek = { name: list[i].name, scroll: 0 }; },
      draw: (c, b, st, it) => {
        const h = b.h - 1, y = b.y + 4, cat = it.f ? it.f.cat : -1, ccol = cat < 0 ? PAL.red : [PAL.gold, PAL.gold, PAL.green, PAL.dim, PAL.blue, PAL.red, PAL.line][cat];
        R(c, b.x, b.y, b.w, h, st.p ? PAL.panel2 : PAL.panel); R(c, b.x, b.y, 2, h, ccol);
        const max = featMax(k, it.f), cnt = max ? (max - it.used) + '/' + max : '', room = Math.floor((b.w - 8 - (cnt ? tw(cnt) + 4 : 0)) / 6);
        tx(c, it.name.length > room ? it.name.slice(0, room - 1) + '.' : it.name, b.x + 5, y, cat < 0 ? PAL.red : PAL.text);
        if (cnt) txr(c, cnt, b.x + b.w - 3, y, PAL.gold);
        if (S.fpeek && S.fpeek.name === it.name) BOX(c, b.x, b.y, b.w, h, PAL.blue, 1);
        if (st.f) focusBox(c, { x: b.x, y: b.y, w: b.w, h });
      } });
    const add = plain({ id: 'fadd', x: 181, y: 164, w: 72, h: 14 }, '+ ADD'); add.act = () => openFeatPicker(); btns.push(add);
    const f = S.focus.sheet4 || '', onChip = f.startsWith('ft');
    const keys = kk => {
      if (S.fpeek) {
        if (kk.down.B) { S.fpeek = null; return true; }
        if (kk.rep.L) { S.fpeek.scroll = Math.max(0, S.fpeek.scroll - 2); return true; }
        if (kk.rep.R) { S.fpeek.scroll = Math.min(S.fpeek.maxScroll || 0, S.fpeek.scroll + 2); return true; }
      }
      return L.keys(kk);
    };
    return { def: tracked.length ? 'ft0' : list.length ? 'frow0' : 'fadd', onKeys: keys,
      footer: S.fpeek ? [['▲▼', 'BROWSE'], ['L/R', 'SCROLL'], ['B', 'CLOSE']] : onChip ? [['A', 'USE'], ['Y', 'RESTORE'], ['X', 'EXPAND'], ['L/R', 'TABS']] : [['A', 'READ'], ['X', 'EXPAND'], ['L/R', 'TABS'], ['ST', 'MENU']],
      bot: c => {
        if (!list.length) { R(c, 3, 18, 250, 140, PAL.panel); txc(c, 'NO FEATURES YET', 3, 250, 70, PAL.text); txc(c, 'TAP + ADD (OR PRESS X) TO PICK CLASS', 3, 250, 86, PAL.dim); txc(c, 'FEATURES, FEATS, TRAITS AND ITEMS.', 3, 250, 96, PAL.dim); }
        tx(c, list.length + ' FEATURES' + (tracked.length ? ', ' + tracked.length + ' TRACKED' : ''), 4, 168, PAL.dim);
      } };
  };
  function featTop(c, name, f, o, k) {
    R(c, 0, 0, W, H, PAL.bg); R(c, 0, 0, W, 32, PAL.panel); tx(c, name, 6, 4, PAL.text, tw(name, 2) <= 244 ? 2 : 1);
    if (!f) { tx(c, 'NOT IN YOUR FEATURE LIBRARY', 6, 21, PAL.red); wrap('ADD IT TO /DATA/POCKETSHEET/FEATURES.TXT ON A PC, OR REMOVE IT FROM THIS CHARACTER.', 40).forEach((l, i) => tx(c, l, 6, 44 + i * 10, PAL.text)); if (S.toast) strip(c, S.toast.text, S.toast.col); return; }
    tx(c, FCAT[f.cat] + (f.src !== 'BUILT-IN' ? '   FROM ' + f.src : ''), 6, 21, PAL.dim); if (f.inspire) { txr(c, 'INSPIRES', 250, 21, PAL.gold); icon(c, 'mstar', 250 - tw('INSPIRES') - 10, 21, [PAL.gold]); }
    let y0 = 38; const max = featMax(k, f);
    if (max) { const used = (k.feats.find(x => x.name === name) || { used: 0 }).used; R(c, 3, 35, 250, 18, PAL.panel); tx(c, 'USES', 8, 41, PAL.dim); tx(c, (max - used) + ' OF ' + max + (f.uses === 'PB' ? ' (PROF BONUS)' : AB.includes(f.uses) ? ' (' + f.uses + ')' : ''), 38, 41, PAL.gold); txr(c, RESETS[f.reset], 248, 41, PAL.dim); y0 = 58; }
    const lines = f.desc.split('\n').flatMap((p, i) => (i ? [''] : []).concat(wrap(p, 40))), vis = Math.floor((176 - y0) / 10);
    if (o) { o.maxScroll = Math.max(0, lines.length - vis); o.scroll = Math.min(o.scroll || 0, o.maxScroll); }
    const s0 = o ? o.scroll : 0; lines.slice(s0, s0 + vis).forEach((l, i) => tx(c, l, 6, y0 + i * 10, PAL.text));
    if (s0 > 0) txr(c, '▲', 252, y0, PAL.gold); if (s0 + vis < lines.length) txr(c, '▼', 252, 168, PAL.gold);
    if (S.toast) strip(c, S.toast.text, S.toast.col);
  }
  OV.feat = o => {
    const k = ch(), name = o.names[o.i], lf = S.flib.get(name), mine = k.feats.find(x => x.name === name); if (!mine) { S.ov.pop(); return ctxNow(); }
    const max = featMax(k, lf), btns = [];
    if (max) {
      const use = iconBtn({ id: 'fuse', x: 3, y: 22, w: 123, h: 28 }, 'mstar', ['USE ONE'], { bg: PAL.gold, fg: PAL.ink }); use.act = () => spendFeat(k, name, 1);
      const back1 = plain({ id: 'fres', x: 130, y: 22, w: 60, h: 28 }, '+1 BACK'); back1.act = () => spendFeat(k, name, -1);
      const all = plain({ id: 'fall', x: 193, y: 22, w: 60, h: 28 }, lf.reset === 'NONE' ? 'RELOAD' : 'RESET'); all.act = () => { mine.used = 0; S.dirty = true; toast(name + ': ALL ' + max + ' BACK', PAL.gold); };
      btns.push(use, back1, all);
    }
    const up = plain({ id: 'fup', x: 3, y: 56, w: 123, h: 26 }, '▲ SCROLL'); up.act = () => { o.scroll = Math.max(0, o.scroll - 5); };
    const dn = plain({ id: 'fdn', x: 130, y: 56, w: 123, h: 26 }, 'SCROLL ▼'); dn.act = () => { o.scroll = Math.min(o.maxScroll || 0, o.scroll + 5); };
    btns.push(up, dn);
    if (o.names.length > 1) {
      const pv = plain({ id: 'fprev', x: 3, y: 88, w: 123, h: 24 }, '◀ PREVIOUS'); pv.act = () => { o.i = (o.i + o.names.length - 1) % o.names.length; o.scroll = 0; };
      const nx = plain({ id: 'fnext', x: 130, y: 88, w: 123, h: 24 }, 'NEXT ▶'); nx.act = () => { o.i = (o.i + 1) % o.names.length; o.scroll = 0; };
      btns.push(pv, nx);
    }
    const rem = { id: 'frem', x: 3, y: 156, w: 100, h: 20 };
    rem.draw = (c, st) => { R(c, rem.x, rem.y, rem.w, rem.h, st.p ? PAL.text : PAL.panel); tx(c, '✗', rem.x + 6, rem.y + 7, st.p ? PAL.ink : PAL.red); tx(c, 'REMOVE...', rem.x + 16, rem.y + 7, st.p ? PAL.ink : PAL.text); if (st.f) focusBox(c, rem); };
    rem.act = () => pushNotice({ title: 'REMOVE ' + name + '?', tone: 'err', sub: 'FROM THIS CHARACTER', lines: ['REMOVES THE FEATURE FROM THIS CHARACTER.', 'IT STAYS IN YOUR LIBRARY, SO YOU CAN ADD IT BACK.'], def: 0, cancel: 0,
      btns: [{ label: 'KEEP' }, { label: 'REMOVE', danger: true, act: () => { k.feats = k.feats.filter(x => x.name !== name); S.dirty = true; popOv(); popOv(); toast('REMOVED ' + name, PAL.dim); } }] });
    const back = plain({ id: 'fback', x: 153, y: 156, w: 100, h: 20 }, 'BACK', { bg: PAL.gold, fg: PAL.ink }); back.act = () => popOv();
    btns.push(rem, back);
    const cx = { key: 'fe' + o.uid, btns, def: max ? 'fuse' : 'fdn', footer: [['A', 'PRESS'], ['L/R', 'SCROLL'], ['B', 'BACK']] };
    cx.bot = c => { R(c, 0, 0, W, 16, PAL.panel); tx(c, 'FEATURE', 6, 5, PAL.gold); txr(c, (o.i + 1) + ' OF ' + o.names.length, 250, 5, PAL.dim); if (!max) txc(c, 'NO USES TO TRACK', 0, W, 32, PAL.dim); txc(c, 'READ THE FULL TEXT ON THE TOP SCREEN', 0, W, 132, PAL.dim); };
    cx.top = c => featTop(c, name, lf, o, k);
    cx.onKeys = kk => { if (kk.down.B) { popOv(); return true; } if (kk.rep.L) { o.scroll = Math.max(0, o.scroll - 2); return true; } if (kk.rep.R) { o.scroll = Math.min(o.maxScroll || 0, o.scroll + 2); return true; } return false; };
    return cx;
  };
  OV.featpick = o => {
    const k = ch(), btns = [], fkey = 'fpick' + o.uid;
    ['ALL'].concat(FCAT3).forEach((l, j) => {
      const fv = j - 1, b = { id: 'fpf' + j, x: 3 + j * 31, y: 18, w: 29, h: 16 };
      b.draw = (c, st) => { const on = o.filter === fv; R(c, b.x, b.y, b.w, b.h, on ? PAL.gold : st.p ? PAL.panel2 : PAL.panel); txc(c, l, b.x, b.w, b.y + 5, on ? PAL.ink : PAL.dim); if (st.f) focusBox(c, b); };
      b.act = () => { o.filter = fv; S.scroll.fk = 0; }; btns.push(b);
    });
    const known = new Set(k.feats.map(x => x.name));
    const items = [...S.flib.values()].filter(f => o.filter < 0 || f.cat === o.filter).sort((a, b) => a.cat - b.cat || (a.name < b.name ? -1 : 1)).map(f => ({ name: f.name, f, known: known.has(f.name) }));
    const L = listRows(btns, { pre: 'fk', fkey, items, x: 3, y: 37, w: 226, h: 16, rows: 8,
      act: i => { const it = items[i], j = k.feats.findIndex(x => x.name === it.name); if (j >= 0) { k.feats.splice(j, 1); toast('REMOVED ' + it.name, PAL.dim); } else if (k.feats.length >= MAXF) toast('FEATURE LIST FULL (' + MAXF + ')', PAL.gold); else { k.feats.push({ name: it.name, used: 0 }); toast('ADDED ' + it.name, PAL.blue); } S.dirty = true; },
      draw: (c, b, st, it) => { const h = b.h - 1, y = b.y + 4; R(c, b.x, b.y, b.w, h, st.p ? PAL.panel2 : PAL.panel); R(c, b.x + 2, b.y + 2, 19, 11, PAL.panel2); txc(c, FCAT3[it.f.cat], b.x + 2, 19, y, PAL.dim); tx(c, it.name, b.x + 25, y, PAL.text); if (it.known) txr(c, '✓', b.x + b.w - 4, y, PAL.green); if (st.f) focusBox(c, { x: b.x, y: b.y, w: b.w, h }); } });
    const cx = { key: fkey, btns, def: items.length ? 'fk0' : 'fpf0', footer: [['A', 'ADD/REMOVE'], ['L/R', 'READ'], ['B', 'DONE']] };
    const focusItem = () => { const f = S.focus[fkey]; return f && f.startsWith('fk') ? items[+f.slice(2)] : null; };
    cx.bot = c => { if (!items.length) { R(c, 3, 37, 250, 128, PAL.panel); txc(c, 'NOTHING IN THIS CATEGORY', 3, 250, 96, PAL.dim); } tx(c, k.feats.length + '/' + MAXF + ' ON SHEET', 4, 169, PAL.dim); txr(c, S.flibInfo.state === 'ok' ? 'FEATURES.TXT +' + S.flibInfo.added : 'BUILT-IN ONLY', 252, 169, PAL.dim); };
    cx.top = c => {
      const it = focusItem(); if (it) { if (o.lastF !== it.name) { o.lastF = it.name; o.scroll = 0; } return featTop(c, it.name, it.f, o, k); }
      R(c, 0, 0, W, H, PAL.bg); txc(c, 'ADD FEATURES', 0, W, 20, PAL.gold, 2);
      ['PICK A CATEGORY, THEN PRESS A TO ADD OR REMOVE. THE FULL TEXT SHOWS HERE.', 'LIBRARY: ' + S.flib.size + ' ENTRIES (' + FSTARTER.length + ' BUILT IN).', 'ADD YOUR OWN CLASS FEATURES, FEATS AND ITEM PROPERTIES IN /DATA/POCKETSHEET/FEATURES.TXT ON A PC.']
        .flatMap(p => wrap(p, 40).concat([''])).forEach((l, i) => tx(c, l, 8, 50 + i * 11, PAL.text));
      if (S.toast) strip(c, S.toast.text, S.toast.col);
    };
    cx.onKeys = kk => { if (kk.down.B) { popOv(); return true; } if (kk.rep.L) { o.scroll = Math.max(0, o.scroll - 2); return true; } if (kk.rep.R) { o.scroll = Math.min(o.maxScroll || 0, o.scroll + 2); return true; } return L.keys(kk); };
    return cx;
  };

  // EDIT: three pages (INFO / ABILITIES / SKILLS)
  TABS[5] = (k, btns) => {
    const pg = S.editPage;
    ['INFO', 'ABILITIES', 'SKILLS'].forEach((n, i) => {
      const b = { id: 'pg' + i, x: 3 + i * 84, y: 17, w: 82, h: 14 };
      b.draw = (c, st) => { const on = pg === i; R(c, b.x, b.y, b.w, b.h, on ? PAL.blue : st.p ? PAL.panel2 : PAL.panel); txc(c, n, b.x, b.w, b.y + 4, on ? PAL.ink : PAL.dim); if (st.f) focusBox(c, b); };
      b.act = () => { S.editPage = i; }; btns.push(b);
    });
    if (pg === 0) {
      const text = (field, title, max, empty) => () => kbd({ title, field: title, val: k[field], max, allowEmpty: empty, onOk: v => { k[field] = v; S.dirty = true; popOv(); toast(title + ' UPDATED', PAL.blue); } });
      const num = cfg => () => stepper(cfg);
      const items = [['NAME', () => k.name, text('name', 'NAME', 16)], ['CLASS', () => k.cls, text('cls', 'CLASS', 12)], ['SUBCLASS', () => k.sub || '-', text('sub', 'SUBCLASS', 16, true)],
        ['SPECIES', () => k.species || '-', text('species', 'SPECIES', 16, true)], ['BACKGROUND', () => k.background || '-', text('background', 'BACKGROUND', 16, true)],
        ['LEVEL', () => String(k.level), num({ title: 'LEVEL', min: 1, max: 20, get: () => k.level, set: v => { k.level = v; k.hdUsed = Math.min(k.hdUsed, v); }, preview: v => `LEVEL ${k.level} → ${v}   PROF ${sg(pb(k))} → ${sg(2 + Math.floor((v - 1) / 4))}` })],
        ['MAX HP', () => String(k.hpMax), num({ title: 'MAX HP', min: 1, max: 999, big: 10, get: () => k.hpMax, set: v => { if (k.hp === k.hpMax) k.hp = v; k.hpMax = v; k.hp = Math.min(k.hp, v); } })],
        ['HIT DIE', () => 'D' + k.hitDie, () => { k.hitDie = HITDIE[(HITDIE.indexOf(k.hitDie) + 1) % HITDIE.length]; S.dirty = true; }],
        ['ARMOR CLASS', () => String(k.ac), num({ title: 'ARMOR CLASS', min: 0, max: 40, get: () => k.ac, set: v => { k.ac = v; } })],
        ['SPEED', () => String(k.speed), num({ title: 'SPEED (FEET)', min: 0, max: 120, step: 5, big: 10, get: () => k.speed, set: v => { k.speed = v; } })],
        ['SPELL ABILITY', () => k.spellAb === 255 ? 'NONE' : AB[k.spellAb], () => { const sa = k.spellAb; k.spellAb = sa === 255 ? 0 : sa === 5 ? 255 : sa + 1; S.dirty = true; }],
      ].map(it => [...it, it[0] !== 'HIT DIE' && it[0] !== 'SPELL ABILITY' ? 1 : 0]);
      var editL = listRows(btns, { pre: 'f', fkey: 'sheet5_0', items, x: 3, y: 33, w: 226, h: 16, rows: 9, act: i => items[i][2](),
        draw: (c, b, st, it) => { const h = b.h - 1; R(c, b.x, b.y, b.w, h, st.p ? PAL.panel2 : PAL.panel); tx(c, it[0], b.x + 6, b.y + 4, PAL.dim); txr(c, it[1](), b.x + b.w - (it[3] ? 16 : 6), b.y + 4, PAL.gold); if (it[3]) tx(c, '▶', b.x + b.w - 10, b.y + 4, PAL.dim); if (st.f) focusBox(c, { x: b.x, y: b.y, w: b.w, h }); } });
    } else if (pg === 1) {
      for (let i = 0; i < 6; i++) {
        const y = 34 + i * 24, s = { id: 'sc' + i, x: 3, y, w: 168, h: 21 }, v = { id: 'sv' + i, x: 174, y, w: 79, h: 21 };
        s.draw = (c, st) => { R(c, s.x, y, s.w, s.h, st.p ? PAL.panel2 : PAL.panel); tx(c, ABF[i], s.x + 6, y + 7, PAL.text); txr(c, `${k.ab[i]}  (${sg(mod(k.ab[i]))})`, s.x + s.w - 6, y + 7, PAL.gold); if (st.f) focusBox(c, s); };
        s.act = () => stepper({ title: ABF[i] + ' SCORE', min: 1, max: 30, get: () => k.ab[i], set: x => { k.ab[i] = x; }, preview: x => `${AB[i]} ${k.ab[i]} → ${x}   MOD ${sg(mod(k.ab[i]))} → ${sg(mod(x))}` });
        v.draw = (c, st) => { const p = (k.saveProf >> i) & 1; R(c, v.x, y, v.w, v.h, st.p ? PAL.panel2 : PAL.panel); tx(c, 'SAVE PROF', v.x + 6, y + 7, PAL.dim); txr(c, p ? '●' : '○', v.x + v.w - 6, y + 7, p ? PAL.gold : PAL.line); if (st.f) focusBox(c, v); };
        v.act = () => { k.saveProf ^= 1 << i; S.dirty = true; };
        btns.push(s, v);
      }
    } else {
      const setP = (i, v) => { k.skill[i] = v; S.dirty = true; toast(`${SK[i][0]}: ${['NOT PROFICIENT', 'PROFICIENT', 'EXPERTISE'][v]} (${sg(skMod(k, i))})`, v === 2 ? PAL.blue : v ? PAL.gold : PAL.dim); };
      SK.forEach((s, i) => { const b = skillRow({ id: 'ps' + i, x: 3 + (i < 9 ? 0 : 126), y: 32 + (i % 9) * 15, w: 124, h: 15 }, k, i, true); b.act = () => setP(i, (k.skill[i] + 1) % 3); b.onY = () => setP(i, k.skill[i] === 2 ? 1 : 2); btns.push(b); });
    }
    return { sub: '_' + pg, edit: true, onKeys: pg === 0 ? editL.keys : null, def: ['f0', 'sc0', 'ps0'][pg], footer: pg === 2 ? [['A', 'CYCLE'], ['Y', 'EXPERTISE'], ['X', 'PAGE'], ['SE', 'DONE']] : [['A', 'CHANGE'], ['X', 'PAGE'], ['L/R', 'TABS'], ['SE', 'DONE']],
      bot: pg !== 2 ? null : c => { const b = pb(k); let x = 5; [[0, 'NONE'], [1, 'PROF ' + sg(b)], [2, 'EXPERTISE ' + sg(b * 2)]].forEach(([p, l]) => { tx(c, PROF_GLYPH[p], x, 170, PROF_COL[p]); tx(c, l, x + 8, 170, p ? PROF_COL[p] : PAL.dim); x += 8 + tw(l) + 14; }); } };
  };

  // ---------- overlays ----------
  function baseTop(c) { if (S.view === 'sheet') overviewTop(c); else V[S.view]().top(c); }
  OV.notice = o => {
    const n = o.btns.length;
    const btns = o.btns.map((bt, j) => {
      const b = plain({ id: 'nb' + j, x: n === 1 ? 68 : (j === 0 ? 12 : 134), y: 136, w: n === 1 ? 120 : 110, h: 26 }, bt.label, bt.danger ? { bg: PAL.red, fg: PAL.ink } : bt.primary ? { bg: PAL.gold, fg: PAL.ink } : {});
      b.act = () => bt.act ? bt.act() : popOv(); return b;
    });
    const cx = { key: 'ov' + o.uid, btns, def: 'nb' + (o.def || 0), footer: [['◀▶', 'CHOOSE'], ['A', 'OK']].concat(o.cancel != null || n === 1 ? [['B', o.btns[o.cancel ?? 0].label]] : []) };
    cx.bot = c => {
      const err = o.tone === 'err'; R(c, 0, 0, W, 16, err ? PAL.red : PAL.panel); tx(c, o.title, 6, 5, err ? PAL.ink : PAL.gold);
      let y = 26; for (const l of o.lines.flatMap(l => wrap(l, 40))) { tx(c, l, 8, y, PAL.text); y += 11; }
      if (o.err) tx(c, o.err, 8, 122, PAL.red);
    };
    cx.top = c => {
      const t = S.toast; S.toast = null; baseTop(c); S.toast = t; c.fillStyle = 'rgba(8,4,16,0.72)'; c.fillRect(0, 0, W, H);
      const err = o.tone === 'err'; R(c, 16, 64, 224, 64, PAL.panel); BOX(c, 16, 64, 224, 64, err ? PAL.red : PAL.gold, 1);
      const sc = tw(o.title, 2) <= 208 ? 2 : 1; txc(c, o.title, 16, 224, sc === 2 ? 78 : 82, err ? PAL.red : PAL.gold, sc);
      txc(c, o.sub || 'SEE THE LOWER SCREEN', 16, 224, 104, PAL.dim);
    };
    cx.onKeys = k => { if (k.down.B) { const ci = o.cancel != null ? o.cancel : (n === 1 ? 0 : null); if (ci != null) act(btns[ci]); return true; } return false; };
    return cx;
  };
  OV.menu = o => {
    const k = ch();
    const items = [
      ['RESUME', () => popOv(), 'BACK TO THE SHEET. B OR START ALSO CLOSE THIS MENU.'],
      ['SAVE NOW', () => { popOv(); doSave(); }, 'WRITES FAT:/DATA/POCKETSHEET/SHEET.SAV. ' + (S.session === 'temp' ? 'TEMPORARY SESSION: SAVING IS OFF.' : S.dirty ? 'YOU HAVE UNSAVED CHANGES.' : 'EVERYTHING IS SAVED.')],
      ['SHORT REST', () => { popOv(); openShortRest(k); }, 'SPEND HIT DICE TO REGAIN HP. YOU ENTER WHAT YOU ROLLED, OR USE THE AVERAGE.'],
      ['LONG REST', () => askLongRest(k), 'RESTORES ALL HP, SPELL SLOTS AND HIT DICE. CLEARS TEMP HP AND DEATH SAVES. ASKS FIRST.'],
      ['EDIT SHEET', () => { popOv(); S.prevTab = S.tab === 5 ? S.prevTab : S.tab; S.tab = 5; }, 'NAME, CLASS, SUBCLASS, LEVEL, HP, AC, SPEED, SCORES AND PROFICIENCIES. SHORTCUT: SELECT.'],
      ['CHARACTERS', () => { popOv(); leave(); }, 'BACK TO THE CHARACTER LIST. ' + (S.session === 'sd' ? 'UNSAVED CHANGES ARE SAVED FIRST.' : 'TEMPORARY SESSION: NOTHING IS SAVED.')],
    ];
    const btns = items.map(([l, f], i) => {
      const b = { id: 'm' + i, x: 8, y: 20 + i * 19, w: 240, h: 17 };
      b.draw = (c, st) => { R(c, b.x, b.y, b.w, b.h, st.p ? PAL.text : PAL.panel); tx(c, l, b.x + 8, b.y + 5, st.p ? PAL.ink : PAL.text); if (st.f) focusBox(c, b); };
      b.act = f; return b;
    });
    const cx = { key: 'menu', btns, def: 'm0', footer: [['A', 'CHOOSE'], ['B', 'CLOSE']] };
    cx.bot = c => {
      R(c, 0, 0, W, 16, PAL.panel); tx(c, 'MENU', 6, 5, PAL.gold); txr(c, `SLOT ${S.cur + 1}`, 250, 5, PAL.dim);
      const fi = +(curFocus(cx) || 'm0').slice(1); wrap(items[fi][2], 40).slice(0, 3).forEach((l, i) => tx(c, l, 8, 142 + i * 11, PAL.dim));
    };
    cx.top = c => overviewTop(c);
    cx.onKeys = kk => { if (kk.down.B || kk.down.START) { popOv(); return true; } return false; };
    return cx;
  };
  OV.stepper = o => {
    const set = v => { o.val = clamp(v, o.min, o.max); };
    const btns = [-o.big, -o.step, o.step, o.big].map((d, j) => { const b = plain({ id: 'st' + j, x: 4 + j * 62, y: 96, w: 58, h: 26, nav: false }, (d > 0 ? '+' : '-') + Math.abs(d), { sc: 2 }); b.act = () => set(o.val + d); return b; });
    const okB = plain({ id: 'stok', x: 132, y: 140, w: 120, h: 26, nav: false }, 'OK', { bg: PAL.gold, fg: PAL.ink });
    okB.act = () => { const changed = o.val !== o.get(); o.set(o.val); if (changed) S.dirty = true; popOv(); };
    const cB = plain({ id: 'stc', x: 4, y: 140, w: 120, h: 26, nav: false }, 'CANCEL'); cB.act = () => popOv();
    btns.push(cB, okB);
    const cx = { key: 'stp' + o.uid, btns, footer: [['◀▶', '+/-' + o.step], ['▲▼', '+/-' + o.big], ['A', 'OK'], ['B', 'CANCEL']] };
    cx.bot = c => { R(c, 0, 0, W, 16, PAL.panel); tx(c, o.title, 6, 5, PAL.gold); txc(c, String(o.val), 0, W, 36, PAL.gold, 4); txc(c, `RANGE ${o.min}-${o.max}`, 0, W, 74, PAL.dim); };
    cx.top = c => overviewTop(c, o.preview ? o.preview(o.val) : `${o.title} ${o.get()} → ${o.val}`);
    cx.onKeys = k => {
      if (k.rep.LEFT) set(o.val - o.step); else if (k.rep.RIGHT) set(o.val + o.step); else if (k.rep.UP) set(o.val + o.big); else if (k.rep.DOWN) set(o.val - o.big);
      if (k.down.A) okB.act(); else if (k.down.B) popOv(); return true;
    };
    return cx;
  };
  OV.kbd = o => {
    const rows = ['1234567890', 'QWERTYUIOP', "ASDFGHJKL'", 'ZXCVBNM.,/', '-&()+!?:'], btns = [];
    const type = chr => { o.err = null; if (o.val.length >= o.max) { o.err = `${o.max} CHARACTERS MAX`; return; } if (chr === ' ' && (!o.val || o.val.endsWith(' '))) return; o.val += chr; };
    const done = () => { const v = o.val.trim(); if (!v && !o.allowEmpty) { o.err = `${o.field || 'TEXT'} CAN'T BE EMPTY`; return; } o.onOk(v); };
    const del = () => { o.err = null; o.val = o.val.slice(0, -1); };
    rows.forEach((r, ri) => [...r].forEach((chr, ci) => { const b = plain({ id: `kb${ri}_${ci}`, x: 3 + ci * 25, y: 18 + ri * 24, w: 23, h: 22 }, chr); b.act = () => type(chr); btns.push(b); }));
    [['kcancel', 3, 58, 'CANCEL', () => popOv()], ['kspace', 63, 96, 'SPACE', () => type(' ')], ['kdel', 161, 42, 'DEL', del], ['kok', 205, 48, 'OK', done]].forEach(([id, x, w, l, f]) => {
      const b = plain({ id, x, y: 139, w, h: 22 }, l, id === 'kok' ? { bg: PAL.gold, fg: PAL.ink } : {}); b.act = f; btns.push(b);
    });
    const blink = () => (S.frame >> 5) & 1;
    const cx = { key: 'kbd' + o.uid, btns, def: 'kb1_0', footer: [['A', 'TYPE'], ['B', 'DELETE'], ['ST', 'DONE']] };
    cx.bot = c => { R(c, 0, 0, W, 16, PAL.panel); tx(c, o.title, 6, 5, PAL.gold); R(c, 3, 164, 250, 14, PAL.panel); tx(c, o.val, 8, 167, PAL.text); if (blink()) R(c, 8 + o.val.length * 6, 174, 5, 1, PAL.gold); };
    cx.top = c => {
      R(c, 0, 0, W, H, PAL.bg); tx(c, o.title, 8, 24, PAL.gold); R(c, 8, 52, 240, 30, PAL.panel); BOX(c, 8, 52, 240, 30, PAL.line, 1);
      tx(c, o.val, 14, 60, PAL.text, 2); if (blink()) R(c, 14 + o.val.length * 12, 75, 10, 2, PAL.gold);
      txr(c, `${o.val.length}/${o.max}`, 248, 88, PAL.dim); if (o.err) tx(c, o.err, 8, 104, PAL.red);
      if (o.allowEmpty) tx(c, 'OPTIONAL: OK WITH NOTHING TYPED CLEARS IT.', 8, 120, PAL.dim);
      tx(c, 'TOUCH A KEY, OR MOVE WITH THE D-PAD', 8, 150, PAL.dim); tx(c, 'AND PRESS A. B DELETES. START IS DONE.', 8, 162, PAL.dim);
    };
    cx.onKeys = k => { if (k.down.B) { if (o.val) del(); else popOv(); return true; } if (k.down.START) { done(); return true; } return false; };
    return cx;
  };

  OV.spell = o => {
    const k = ch(), name = o.names[o.i], s = k.spells.find(x => x.name === name), sp = S.lib.get(name);
    if (!s) { S.ov.pop(); return ctxNow(); }
    const lv = sp ? sp.level : -1, left = lv > 0 ? k.slotMax[lv - 1] - k.slotUsed[lv - 1] : 0, btns = [];
    const prep = { id: 'sprep', x: 3, y: 20, w: 250, h: 24 };
    prep.draw = (c, st) => { R(c, prep.x, prep.y, prep.w, prep.h, st.p ? PAL.panel2 : PAL.panel); if (lv === 0) tx(c, 'CANTRIP: ALWAYS READY', prep.x + 8, prep.y + 9, PAL.dim); else { tx(c, 'PREPARED', prep.x + 8, prep.y + 9, PAL.text); txr(c, s.prep ? 'YES ●' : 'NO ○', prep.x + prep.w - 8, prep.y + 9, s.prep ? PAL.gold : PAL.dim); } if (st.f) focusBox(c, prep); };
    prep.act = () => togglePrep(k, name);
    const use = { id: 'spuse', x: 3, y: 48, w: 250, h: 24 };
    use.draw = (c, st) => { const ok = lv > 0 && left > 0; R(c, use.x, use.y, use.w, use.h, st.p ? PAL.text : ok ? PAL.blue : PAL.panel);
      const t = lv <= 0 ? 'NO SLOT NEEDED' : !k.slotMax[lv - 1] ? 'NO ' + ORD[lv - 1] + ' LEVEL SLOTS SET' : left ? 'USE A ' + ORD[lv - 1] + ' LEVEL SLOT (' + left + ' LEFT)' : 'NO ' + ORD[lv - 1] + ' LEVEL SLOTS LEFT';
      txc(c, t, use.x, use.w, use.y + 9, st.p || ok ? PAL.ink : PAL.dim); if (st.f) focusBox(c, use); };
    use.act = () => { if (lv > 0 && left > 0) { k.slotUsed[lv - 1]++; S.dirty = true; toast('CAST ' + name + ': ' + (left - 1) + ' ' + ORD[lv - 1] + ' LEFT', PAL.blue); } else if (lv > 0) toast('NO ' + ORD[lv - 1] + ' LEVEL SLOTS LEFT', PAL.gold); };
    const up = plain({ id: 'spup', x: 3, y: 76, w: 123, h: 30 }, '▲ SCROLL'); up.act = () => { o.scroll = Math.max(0, o.scroll - 5); };
    const dn = plain({ id: 'spdn', x: 130, y: 76, w: 123, h: 30 }, 'SCROLL ▼'); dn.act = () => { o.scroll = Math.min(o.maxScroll || 0, o.scroll + 5); };
    btns.push(prep, use, up, dn);
    if (o.names.length > 1) {
      const pv = plain({ id: 'spprev', x: 3, y: 110, w: 123, h: 24 }, '◀ PREVIOUS'); pv.act = () => { o.i = (o.i + o.names.length - 1) % o.names.length; o.scroll = 0; };
      const nx = plain({ id: 'spnext', x: 130, y: 110, w: 123, h: 24 }, 'NEXT ▶'); nx.act = () => { o.i = (o.i + 1) % o.names.length; o.scroll = 0; };
      btns.push(pv, nx);
    }
    const rem = { id: 'sprem', x: 3, y: 156, w: 100, h: 20 };
    rem.draw = (c, st) => { R(c, rem.x, rem.y, rem.w, rem.h, st.p ? PAL.text : PAL.panel); tx(c, '✗', rem.x + 6, rem.y + 7, st.p ? PAL.ink : PAL.red); tx(c, 'REMOVE...', rem.x + 16, rem.y + 7, st.p ? PAL.ink : PAL.text); if (st.f) focusBox(c, rem); };
    rem.act = () => pushNotice({ title: 'REMOVE ' + name + '?', tone: 'err', sub: 'FROM THIS CHARACTER', lines: ['REMOVES THE SPELL FROM THIS CHARACTER.', 'IT STAYS IN YOUR LIBRARY, SO YOU CAN ADD IT BACK.'], def: 0, cancel: 0,
      btns: [{ label: 'KEEP' }, { label: 'REMOVE', danger: true, act: () => { k.spells = k.spells.filter(x => x.name !== name); S.dirty = true; popOv(); popOv(); toast('REMOVED ' + name, PAL.dim); } }] });
    const back = plain({ id: 'spback', x: 153, y: 156, w: 100, h: 20 }, 'BACK', { bg: PAL.gold, fg: PAL.ink }); back.act = () => popOv();
    btns.push(rem, back);
    const cx = { key: 'spl' + o.uid, btns, def: 'sprep', footer: [['A', 'PRESS'], ['L/R', 'SCROLL'], ['Y', 'PREPARE'], ['B', 'BACK']] };
    cx.bot = c => { R(c, 0, 0, W, 16, PAL.panel); tx(c, 'SPELL', 6, 5, PAL.gold); txr(c, (o.i + 1) + ' OF ' + o.names.length, 250, 5, PAL.dim); txc(c, 'READ THE FULL SPELL ON THE TOP SCREEN', 0, W, 141, PAL.dim); };
    cx.top = c => spellTop(c, name, sp, o);
    cx.onKeys = kk => {
      if (kk.down.B) { popOv(); return true; }
      if (kk.rep.L) { o.scroll = Math.max(0, o.scroll - 2); return true; }
      if (kk.rep.R) { o.scroll = Math.min(o.maxScroll || 0, o.scroll + 2); return true; }
      if (kk.down.Y) { togglePrep(k, name); return true; }
      return false;
    };
    return cx;
  };
  OV.spellpick = o => {
    const k = ch(), btns = [], fkey = 'pick' + o.uid;
    ['ALL', 'C', '1', '2', '3', '4', '5', '6', '7', '8', '9'].forEach((l, j) => {
      const f = j - 1, b = { id: 'pf' + j, x: 3 + j * 23, y: 18, w: 21, h: 16 };
      b.draw = (c, st) => { const on = o.filter === f; R(c, b.x, b.y, b.w, b.h, on ? PAL.gold : st.p ? PAL.panel2 : PAL.panel); txc(c, l, b.x, b.w, b.y + 5, on ? PAL.ink : PAL.dim); if (st.f) focusBox(c, b); };
      b.act = () => { o.filter = f; S.scroll.pk = 0; }; btns.push(b);
    });
    const known = new Set(k.spells.map(s => s.name));
    const items = [...S.lib.values()].filter(sp => o.filter < 0 || sp.level === o.filter).sort((a, b) => a.level - b.level || (a.name < b.name ? -1 : 1)).map(sp => ({ name: sp.name, sp, known: known.has(sp.name) }));
    const L = listRows(btns, { pre: 'pk', fkey, items, x: 3, y: 37, w: 226, h: 16, rows: 8, draw: (c, b, st, it) => spellRow(c, b, st, it, false), act: i => togglePick(k, items[i]) });
    const cx = { key: fkey, btns, def: items.length ? 'pk0' : 'pf0', footer: [['A', 'ADD/REMOVE'], ['L/R', 'READ'], ['B', 'DONE']] };
    const focusItem = () => { const f = S.focus[fkey]; return f && f.startsWith('pk') ? items[+f.slice(2)] : null; };
    cx.bot = c => {
      if (!items.length) { R(c, 3, 37, 250, 128, PAL.panel); txc(c, 'NO SPELLS AT THIS LEVEL', 3, 250, 90, PAL.text); txc(c, 'IN YOUR LIBRARY', 3, 250, 102, PAL.dim); }
      const li = S.libInfo; tx(c, k.spells.length + '/' + MAXSP + ' KNOWN', 4, 169, PAL.dim);
      txr(c, li.state === 'ok' ? 'SPELLS.TXT +' + li.added + (li.skipped ? ' (' + li.skipped + ' SKIPPED)' : '') : 'BUILT-IN ONLY', 252, 169, li.skipped ? PAL.gold : PAL.dim);
    };
    cx.top = c => {
      const it = focusItem();
      if (it) { if (o.lastF !== it.name) { o.lastF = it.name; o.scroll = 0; } return spellTop(c, it.name, it.sp, o); }
      R(c, 0, 0, W, H, PAL.bg); txc(c, 'ADD SPELLS', 0, W, 20, PAL.gold, 2); const li = S.libInfo;
      ['PICK A LEVEL, THEN PRESS A ON A SPELL TO ADD OR REMOVE IT. THE FULL TEXT SHOWS HERE.', 'LIBRARY: ' + S.lib.size + ' SPELLS (' + STARTER.length + ' BUILT IN' + (li.state === 'ok' ? ', ' + li.added + ' FROM SPELLS.TXT' : '') + ').', 'TO ADD MORE, EDIT /DATA/POCKETSHEET/SPELLS.TXT ON A PC. THE FILE EXPLAINS THE FORMAT.']
        .flatMap(p => wrap(p, 40).concat([''])).forEach((l, i) => tx(c, l, 8, 50 + i * 11, i < 3 ? PAL.text : PAL.dim));
      if (S.toast) strip(c, S.toast.text, S.toast.col);
    };
    cx.onKeys = kk => {
      if (kk.down.B) { popOv(); return true; }
      if (kk.rep.L) { o.scroll = Math.max(0, o.scroll - 2); return true; }
      if (kk.rep.R) { o.scroll = Math.min(o.maxScroll || 0, o.scroll + 2); return true; }
      return L.keys(kk);
    };
    return cx;
  };
  OV.srest = o => {
    const k = ch(), left = k.level - k.hdUsed, con = mod(k.ab[2]), btns = [];
    const B = (id, x, y, w, h, label, f, opt) => { const b = plain({ id, x, y, w, h }, label, opt || {}); b.act = f; btns.push(b); return b; };
    B('srn-', 120, 36, 36, 22, '-', () => { o.n = Math.max(0, o.n - 1); }, { sc: 2 });
    B('srn+', 216, 36, 36, 22, '+', () => { if (o.n < left) o.n++; else toast('NO HIT DICE LEFT', PAL.gold); }, { sc: 2 });
    [['srh-10', 3, 46, '-10', -10], ['srh-1', 51, 40, '-1', -1], ['srh+1', 165, 40, '+1', 1], ['srh+10', 207, 46, '+10', 10]].forEach(([id, x, w, l, d]) => B(id, x, 82, w, 24, l, () => { o.hp = clamp(o.hp + d, 0, 999); }));
    const avg = Math.max(0, Math.floor(o.n * ((k.hitDie + 1) / 2 + con)));
    B('sravg', 3, 112, 250, 20, o.n ? 'USE AVERAGE: ' + avg + ' HP' : 'USE AVERAGE (CHOOSE DICE FIRST)', () => { if (o.n) o.hp = avg; }, o.n ? {} : { fg: PAL.dim });
    const apply = () => {
      const before = k.hp; k.hdUsed += o.n; k.hp = Math.min(k.hpMax, k.hp + o.hp); if (before === 0 && k.hp > 0) { k.ds = 0; k.df = 0; }
      k.feats.forEach(f => { const lf = S.flib.get(f.name); if (lf && lf.reset === 'SHORT') f.used = 0; else if (lf && lf.reset === 'SHORT1') f.used = Math.max(0, f.used - 1); });
      S.dirty = true; S.ov = []; addFx({ k: 'fire', t: 80, sub: 'HP ' + before + ' → ' + k.hp + (o.n ? '   ' + o.n + ' HIT ' + (o.n > 1 ? 'DICE' : 'DIE') + ' SPENT' : '') }); restSave('RESTED BY THE FIRE: HP ' + before + ' → ' + k.hp);
    };
    B('srcancel', 12, 146, 110, 26, 'CANCEL', () => popOv());
    B('srok', 134, 146, 110, 26, 'REST', apply, { bg: PAL.gold, fg: PAL.ink });
    const cx = { key: 'sr' + o.uid, btns, def: 'srn+', footer: [['A', 'PRESS'], ['ST', 'REST'], ['B', 'CANCEL']] };
    cx.bot = c => {
      R(c, 0, 0, W, 16, PAL.panel); tx(c, 'SHORT REST', 6, 5, PAL.gold); txr(c, 'HIT DICE ' + left + '/' + k.level + '  D' + k.hitDie, 250, 5, PAL.dim);
      tx(c, 'EACH DIE: D' + k.hitDie + ' ' + sg(con) + ' (CON). ROLL, THEN ENTER.', 4, 22, PAL.dim);
      tx(c, 'DICE TO SPEND', 6, 44, PAL.text); R(c, 158, 36, 56, 22, PAL.panel); txc(c, String(o.n), 158, 56, 40, PAL.gold, 2);
      tx(c, 'HP REGAINED', 6, 70, PAL.text); R(c, 93, 82, 70, 24, PAL.panel); txc(c, String(o.hp), 93, 70, 87, PAL.gold, 2);
    };
    cx.top = c => overviewTop(c, 'SHORT REST: ' + o.n + ' HIT DICE   HP ' + k.hp + ' → ' + Math.min(k.hpMax, k.hp + o.hp));
    cx.onKeys = kk => { if (kk.down.B) { popOv(); return true; } if (kk.down.START) { apply(); return true; } return false; };
    return cx;
  };

  function ctxNow() { const o = S.ov[S.ov.length - 1]; return o ? OV[o.type](o) : V[S.view](); }

  // ---------- frame loop (60 Hz fixed step, like swiWaitForVBlank) ----------
  function step() {
    S.frame++; const k = scan(); if (S.lid) return;
    if (S.toast && --S.toast.t <= 0) S.toast = null;
    if (S.fx.length) S.fx = S.fx.filter(f => --f.t > 0);
    if (S.view === 'boot') { S.t++; if (S.t === 50) startup(); return; }
    const cx = ctxNow();
    if (touchStep(cx)) return;
    if (cx.onKeys && cx.onKeys(k)) return;
    for (const d of ['UP', 'DOWN', 'LEFT', 'RIGHT']) if (k.rep[d]) { const id = nav(cx.btns, curFocus(cx), d); if (id) S.focus[cx.key] = id; break; }
    if (k.down.A) act(focused(cx)); else if (k.down.Y) { const b = focused(cx); if (b && b.onY) b.onY(); }
  }
  function render() {
    if (S.lid) { R(T, 0, 0, W, H, '#000'); R(Bc, 0, 0, W, H, '#000'); return; }
    const cx = ctxNow(); cx.top(T);
    R(Bc, 0, 0, W, H, PAL.bg); if (cx.bot) cx.bot(Bc);
    const fid = cx.btns.length ? curFocus(cx) : null;
    for (const b of cx.btns) { const p = !!(S.press && S.press.id === b.id && S.press.inside && S.press.ctx === cx.key); b.draw(Bc, { f: b.id === fid && b.nav !== false, p }); }
    if (cx.footer) footer(Bc, cx.footer, cx.edit);
  }
  let raf, last = performance.now(), acc = 0;
  function loop(now) { acc += Math.min(100, now - last); last = now; let n = 0; while (acc >= 1000 / 60 && n < 4) { step(); acc -= 1000 / 60; n++; } render(); raf = requestAnimationFrame(loop); }
  raf = requestAnimationFrame(loop);

  return {
    key(k, down) { if (down) { hw[k] = true; latch[k] = true; } else hw[k] = false; },
    touch(t, x, y) { tq.push({ t, x, y }); },
    setStorage(m) { fs.mode = m; },
    corrupt(which) {
      const o = fs._get(); let n = 0;
      for (const p of which === 'both' ? [SAV, BAK] : [SAV]) if (o[p]) { const b = dec(o[p]); b[HDR + 3] ^= 0x5A; o[p] = enc(b); n++; }
      fs._put(o); return n;
    },
    // simulates copying a file onto the microSD from a PC
    setTheme(id) { if (THEMES[id]) Object.assign(PAL, THEMES[id]); },
    getText(n) { const p = n === 'features.txt' ? FEATURES_TXT : SPELLS_TXT, o = fs._get(); return o[p] ? decodeText(dec(o[p])) : (n === 'features.txt' ? FEATURES_TEMPLATE : SPELLS_TEMPLATE); },
    putText(n, text) { const p = n === 'features.txt' ? FEATURES_TXT : SPELLS_TXT, o = fs._get(); o[p] = enc(new TextEncoder().encode(text)); fs._put(o); },
    getSpellsTxt() { const o = fs._get(); return o[SPELLS_TXT] ? decodeText(dec(o[SPELLS_TXT])) : SPELLS_TEMPLATE; },
    putSpellsTxt(text) { const o = fs._get(); o[SPELLS_TXT] = enc(new TextEncoder().encode(text)); fs._put(o); },
    putPortrait(i, bytes) { const o = fs._get(); o[PORTRAIT(i)] = enc(bytes); fs._put(o); pcache = {}; },
    removePortrait(i) { const o = fs._get(); delete o[PORTRAIT(i)]; fs._put(o); pcache = {}; },
    erase() { const o = fs._get(); Object.keys(o).filter(p => p.startsWith(DIR)).forEach(p => delete o[p]); fs._put(o); pcache = {}; },
    reboot() { fresh(); },
    setLid(closed) {
      if (closed === S.lid) return;
      if (closed) { S.lid = true; S.press = null; tq = []; S.lidSave = (S.dirty && S.session === 'sd' && S.view !== 'boot') ? saveAll() : null; }
      else { S.lid = false; const r = S.lidSave; S.lidSave = null; if (r) { if (r.ok) toast('SAVED BEFORE SLEEP ✓'); else pushNotice({ ...saveFailNotice(r), title: 'SAVE BEFORE SLEEP FAILED' }); } }
    },
    status() {
      const o = fs._get();
      const files = Object.keys(o).filter(p => p.startsWith(DIR)).sort().map(p => {
        const b = dec(o[p]), name = p.slice(DIR.length);
        if (name === 'features.txt') { const r = parseFeaturesTxt(decodeText(b)); return { name, size: b.length, state: 'valid · ' + r.feats.length + ' entries' + (r.skipped ? ', ' + r.skipped + ' skipped' : '') }; }
        if (name === 'spells.txt') { const r = parseSpellsTxt(decodeText(b)); return { name, size: b.length, state: 'valid · ' + r.spells.length + ' spells' + (r.skipped ? ', ' + r.skipped + ' skipped' : '') }; }
        if (name.endsWith('.bmp')) { const r = parseBMP(b); return { name, size: b.length, state: r.ok ? 'valid' : 'invalid' }; }
        const r = parse(b); return { name, size: b.length, state: r.ok ? (r.migrated ? 'valid (old)' : 'valid') : r.reason.toLowerCase() };
      });
      return { view: S.view, session: S.session, dirty: S.dirty, mode: fs.mode, lid: S.lid, files, slots: S.slots.map(k => k ? k.name : null) };
    },
    destroy() { cancelAnimationFrame(raf); },
  };
}
