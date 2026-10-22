# Handoff: Pocket Sheet, a D&D-style character sheet for the Nintendo DSi XL (DS-mode homebrew)

## Overview
Pocket Sheet is an offline character sheet **reference** for 5e-style tabletop play. Rules terms come from SRD 5.1/5.2 (CC BY 4.0).
- It runs as a DS-mode `.nds` homebrew app, launched from a Slot-1 flashcart on a DSi XL.
- The **top screen** is always the character sheet.
- The **bottom screen** (touch + buttons) holds tabs for skills, features, spells, weapons, combat and editing.
- It stores 3 characters, optional 48×48 portraits, and user-editable spell and feature libraries on the flashcart's microSD.
- There is no dice rolling. The app is reference-first.

Target hardware and constraints:
- DSi XL in **DS mode**, launched from a Slot-1 flashcart.
- The user has **no flashcart yet**, and their **system version is unknown**.
- No custom firmware, no writes to NAND or the DSi SD, no DSi-only features.

**Current status (report each one separately; don't infer a later step from an earlier one):**
- Prototype demonstrated: **yes**, browser simulation only.
- Native source written: **no**.
- Native build successful: **no**.
- Emulator tested: **no**.
- Physical DSi tested: **no**. This needs a flashcart confirmed compatible with the user's exact DSi XL version.

## About the design files
The files in `prototype/` are a **design reference built in HTML/JS**: a browser simulation that shows the intended look and behavior. They are **not production code**. The job is to **rebuild the app natively in C11 with BlocksDS + libnds**.

`prototype/pocket-sheet-sim-v2.js` is the **behavioral source of truth**. It's written to mirror a native structure:
- font and drawing;
- data and rules;
- the binary save format;
- storage;
- input with repeat and the touch state machine;
- screens as button lists with geometric D-pad navigation;
- an overlay stack;
- a 60 Hz fixed-step loop.

Port it screen by screen. Every coordinate in this README and the spec matches that file. Where they disagree, the JS is correct.

To run the prototype, open `prototype/Pocket Sheet Prototype.dc.html` (from a local web server, so the ES module loads).

## Fidelity
**High fidelity, pixel-exact.**
- Both screens are designed at native 256×192, with an original 5×7 bitmap font, original pixel icons and RGB555-exact palettes.
- Rebuild pixel for pixel. Larger previews use integer scaling only.

## Recommended native architecture
- **Toolchain**: BlocksDS (official ARM9-only template, **default ARM7**) on Windows through Wonderful Toolchain (`wf-pacman -S blocksds-toolchain`).
  - Check for existing installs first (`where.exe make`, `C:\msys64`).
  - Skip `blocksds-docs` to save disk space.
  - Report download sizes before installing.
- **Video**:
  - Top: main engine, mode 5, BG3, `BgType_Bmp16` 256×192 in VRAM A. It's 16-bit so portraits and themes need no palette juggling.
  - Bottom: sub engine, BG3, `BgType_Bmp8` in VRAM C.
  - Software-render into RAM back buffers (96 KB + 48 KB). `DC_FlushRange`, then `dmaCopy` in VBlank, only when a screen changed.
  - No sprites or tiles.
- **Loop**: `swiWaitForVBlank()` → `scanKeys()` → at most one action per frame → draw.
  - `keysSetRepeat(15, 4)` with `keysDownRepeat()` for D-pad and stepper repeat. Everything else uses `keysDown()`.
- **Storage**:
  - `fatInitDefault()` with explicit `fat:/data/pocketsheet/` paths (`mkdir`, ignore `EEXIST`).
  - **No NitroFS**: assets are compiled in (`assets/font5x7.h`, `assets/icons.h`).
  - Never write to `sd:/` or `nand:/`.
- **Lid**: on `keysDown() & KEY_LID`, save right away if there are unsaved changes, and show the result after waking. libnds sleeps automatically; confirm in melonDS and on hardware that the save finishes first.
- **Power button** (DSi): the default ARM7 resets without saving. Document this; don't work around it.
- **Modules**:
  - `data` (Character struct, rules), `input`, `ui` (focus/nav, overlay stack, list scroller), `draw` (rect, text 1–4×, icons, dim);
  - `screen_*`, `save` (v5 + migration), `storage`, `spells` (built-ins + `spells.txt` parser), `features` (built-ins + `features.txt` parser, trackers), `portrait` (BMP loader).

## Screens / views

### Top screen: the sheet (always on during a character session)
Order of importance: identity → survival → abilities and saves → secondary numbers → status.

| Element | Rect / position (px) | Notes |
|---|---|---|
| Portrait | 1,1 48×48, 1 px frame | Loaded from `portraitN.bmp`. If there's no file, initials at 2×. A bad file shows a red frame and "BAD FILE" |
| Name | 55,4 at 2× | Drops to 1× if wider than 199 px |
| Class line | 55,24 | `CLASS, SUBCLASS` (text) then `LEVEL n` (gold) 8 px after it. Truncated to fit |
| Species / background | 55,36 dim | `HUMAN (HALF-ELF), WAYFARER` |
| Hit points box | 1,52 120×42 panel | Heart mini-icon + `HIT POINTS` (dim) at 5,56 (`DOWN` in red at 0 HP). Temp HP right-aligned at 117,56 in blue with a mini shield. Current HP at 3× at 5,68, `/max` at 2× dim. No bar |
| AC | Heater shield 122,52 48×42 | Dim outline, panel fill inset 2 px. Value 3× centered at y=56; `AC` dim at y=79 |
| INIT / SPEED | 172,52 and 172,74, 83×20 | Bolt (gold) / boot icon at +4,+5; label at 188; value 2× right-aligned at 250 |
| Abilities + saves | 6 tiles 41×45 at x=43·i, y=97 | Name (dim) at +4,+4; score right-aligned at +37; modifier 2× centered at +13. Save band at +30, h15 (panel2): `SAVE +n`, value gold when proficient |
| Secondary | 0,145 256×31 panel | y=149: `PROF +2`, eye icon + `PASSIVE PERCEPTION 13`. y=162: spark + `SPELL DC n  ATK +n`, then slot gems (4×7, blue = left, outline = spent), wrapping to y=171 |
| Status strip | 0,178 256×14 | Star + `INSPIRED` / `NO INSPIRATION`, `HIT DICE n/n`. At 0 HP: death saves, with failures as skulls. Right side: `SAVED` (green) / `UNSAVED` (gold) / `TEMP SESSION` (blue). Toasts and banners replace this strip (110 frames, ink text on a colored fill) |
| **Hover summary card** | 0,97 256×79 | Replaces the ability and secondary rows when a spell row, feature row/chip or weapon card has focus. See below |

**Hover summary card**:
- 2 px colored stripe on the left: blue for spells, gold for weapons, the category color for features.
- y=101: icon, then the name 4 px after the icon. Tag right-aligned at 251:
  - spells: `● PREPARED` / `NOT PREPARED` / `CANTRIP`;
  - weapons: `+7 TO HIT`;
  - features: `n OF m LEFT`.
- y=112, dim: meta line.
- Rule at y=122, then up to 4 description lines (41 columns, 10 px leading, from y=126). The last line ends in `...` if there's more.
- y=167: `MORE ON THE FULL PAGE` (or `MORE...` if there's no room), and the hint right-aligned: `X: FULL SCREEN`, or `A: USE  X: FULL SCREEN` on tracker chips.

**Expanded page (X)**: the same card design across the whole top screen.
- Panel 0,0 256×178 with the stripe. Icon + name at 6,5; tag at 251.
- Meta at y=16, optional second meta line at y=26, then a rule.
- Full description scrolls with L/R.
- y=167: scroll marks, `L/R SCROLL`, and `X: CLOSE`.
- The status row shows the character name.

### Bottom screen: chrome
- **Tab bar** (y=0, h=16), in this order: **SKILLS** (page icon), **FEATS** (book, gold), **MAGIC** (spark, blue), **ARMS** (sword), **FIGHT** (heart, red), **EDIT** (21 px, quill icon only, at the right).
  - Labeled tab width = 10 + label width + 9; spare pixels are spread across the labeled tabs.
  - Active tab: gold fill (EDIT: blue) with ink icon and label.
- **Footer** (0,180 256×12): button-glyph hints, a 9 px dim box with the key letter in ink, then the label. Blue in edit contexts.
- **Focus**: 2 px gold outline + 1 px ink inner line. **Pressed**: fill becomes the text color, label becomes ink.

| Tab | Layout |
|---|---|
| SKILLS | 18 skill rows 124×16 (drawn 15 tall), columns at x=3 / x=129, y=17+16r. Glyph ○ none (line), ● proficient (gold), ◆ expertise (blue) at +3; name at +11; modifier right-aligned to +120 (blue for expertise). Breakdown strip 3,163 250×15: `DEX +3 + EXPERTISE +4 (2X PROF) = +7` |
| FEATS | Up to 4 **tracker chips** 124×24 (2 per row from y=18): name, reset rule (dim) and gold pips; A spends one, Y gives one back. Below: a **2-column list** of features (cells 112×16, 2 px gap, filled row by row, scrolling, ▲/▼ page buttons 22 px wide at x=231). Each cell has a 2 px category stripe, a truncated name and `n/max` if tracked. `+ ADD` at 181,164 72×14 opens the feature picker |
| MAGIC | **Slot strip** y=18 h=28: one 25 px chip per level that has slots (level digit + 4×8 gems); A spends, Y restores. `SET` (34 px) opens slot maximums. `+ ADD` at 205,18 48×28. **Spell list**: rows 226×16 from y=49, 7 visible, with a level badge (C/1–9), ● prepared, ◆ concentration, R ritual. Y toggles prepared; A opens spell details. Legend at y=169 |
| ARMS | Up to 4 weapon cards 250×36 at y=18+38i. Row 1: sword/arrow icon, name, ◆ MASTERY (blue), range right-aligned. Row 2: to-hit 2× gold + `HIT`, damage 2× + short type, `NOT PROF` if it applies. `+ ADD WEAPON` row below |
| FIGHT | HP readout 3,19 168×22 (heart icon). `INSPIRED`/`UNINSPIRED` chip 174,19 79×22 (star). Amount display 3,44 91×20. Keypad 29×22 at x=3/34/65, y=67/91/115/139 (`7 8 9 / 4 5 6 / 1 2 3 / CLR 0 DEL`). DAMAGE 100,44 153×24 (red, sword). HEAL 100,70 153×24 (green, heart). SET TEMP HP 100,96 153×18 (blue, shield); muted until an amount is entered. Death saves (skull) label at 100,118; SAVES 100,127 75×18; FAILS 178,127 75×18. SHORT REST 100,150 75×24 (animated campfire); LONG REST 178,150 75×24 (moon). `HIT DICE n/n Dx` at 4,167 |
| EDIT | Page buttons INFO / ABILITIES / SKILLS, 82×14 at y=17. INFO: scrolling rows 226×16 (9 visible): name, class, subclass, species, background, level, max HP, hit die, AC, speed, spell ability. ABILITIES: score row 3,y 168×21 + SAVE PROF 174,y 79×21, y=34+24i. SKILLS: skill grid from y=32 (rows 15 tall); A cycles none → prof → expertise, Y toggles expertise |

### Overlays (an overlay stack on the bottom screen; the top screen previews or dims)
- **Boot**: 2× sword with sparkles, `POCKET SHEET` at 3×, ◆ divider, `MOUNTING SD CARD...`.
- **Character select**:
  - Bottom: 3 slot cards 240×40 (portrait-less, heart HP and shield AC), plus `DELETE...` 8,158 100×18.
  - Top: preview with portrait.
- **Start menu**: RESUME, SAVE NOW, SHORT REST, LONG REST, EDIT SHEET, CHARACTERS. Rows 240×17 at y=20+19i; description at y=142.
- **Stepper**:
  - Value 4× at y=36; buttons −big/−step/+step/+big 58×26 at y=96; CANCEL / OK at y=140.
  - ◀▶ ±step and ▲▼ ±big, with repeat.
  - The top-screen strip previews the change.
- **Keyboard** (5 rows):
  - Keys 23×22 at x=3+25c, y=18+24r: `1234567890 / QWERTYUIOP / ASDFGHJKL' / ZXCVBNM.,/ / -&()+!?:`.
  - CANCEL / SPACE / DEL / OK at y=139; echo strip at y=164.
  - The top screen shows the field at 2× with a counter and errors.
  - B deletes (closes when empty); Start = done.
- **Notice/confirm**:
  - Title bar (red for errors), wrapped body, 1–2 buttons at y=136.
  - The top screen dims (software darken) with a title card.
  - Destructive confirms default to KEEP or CANCEL.
- **Short rest**: dice to spend ± (capped at the hit dice left), HP regained −10/−1/+1/+10, `USE AVERAGE` (n × ((die+1)/2 + CON)), CANCEL / REST. Start = rest.
- **Long rest** (asks first): full HP, all slots, all hit dice, all feature uses except MANUAL ones; temp HP and death saves cleared; inspiration set if any feature has `inspire`.
- **Spell details**:
  - PREPARED toggle; USE A n-LEVEL SLOT; scroll; previous / next; REMOVE... (asks first); BACK.
  - Top: full spell card.
- **Spell picker / feature picker**: level or category filter row, library list (✓ = on the sheet; A adds or removes), with the focused entry's full text on top (L/R scroll). Limits: 24 spells, 16 features per character.
- **Slot maximums**: 9 rows 124×28, stepper 0–4.
- **Weapon details**:
  - EDIT WEAPON, previous / next, summary, BACK.
  - Top: full weapon card (to-hit 3×, damage, chips, mastery and properties text).
- **Weapon editor**: scrolling rows: NAME, ABILITY (STR/DEX/FINESSE/SPELL), PROFICIENT, HIT BONUS (−5…+10), DAMAGE DICE (1–9), DIE SIZE (D4–D12), DAMAGE BONUS (−10…+30), DAMAGE TYPE (13), MASTERY (NONE + 8), RANGE (≤7), PROPERTIES (≤28). Plus DELETE... and DONE.

## Interactions & behavior
- **Buttons**:
  - D-pad: geometric focus move to the nearest control in that direction, with repeat. Lists scroll to keep focus visible; ▲ from the top of a list goes to the controls above it.
  - A: activate. B: back, cancel, or close expanded.
  - **X**: expand the focused spell, feature or weapon to the full-screen page; X or B closes it. When nothing expandable has focus, X adds or opens the picker.
  - Y: secondary action (prepare spell, restore a use or slot, toggle expertise).
  - L/R: previous / next tab in the order above, wrapping; they scroll the text while a page is expanded.
  - Start: menu. Select: jump to EDIT and back.
- **Expanded mode**: ▲▼ still move through the bottom list and the page follows. The bottom screen doesn't change.
- **Touch** (one point): press highlights the control; dragging off un-highlights it; releasing on the same control fires **once**. Opening an overlay cancels any press in progress. Only one action per frame.
- **Motion** (frame-counted, never blocks input):
  - Damage: HP box shakes ±2 px for 12 frames, red border flash for 14, the number counts down over 18 frames, and a red `-N` at 2× rises 16 px over 40 frames.
  - Heal: green flash, count up, a rising `+N`, sparkles for 36 frames.
  - Temp HP: blue flash and a rising `+N`.
  - Inspiration on: gold sparkles.
  - Long rest: 90-frame "A NIGHT PASSES..." scene (3× moon, twinkling stars, fade in/out).
  - Short rest: 80-frame "YOU REST BY THE FIRE" scene (3× flickering campfire).
  - Cursor blink: every 32 frames.
- **Copy** (≤42 characters): `OUCH! -7  HP 13/20`, `JEDEDIAH IS DOWN!`, `BACK ON YOUR FEET!`, `+5 HP  GOOD AS NEW!`, `A WARD OF 5 TEMP HP SHIMMERS`, `INSPIRED!`, `A NIGHT PASSES. FULLY RESTORED`.
- **Rules**:
  - mod = ⌊(score−10)/2⌋; prof = 2 + ⌊(level−1)/4⌋.
  - skill = mod + prof × (0/1/2); save = mod + prof if proficient; passive perception = 10 + Perception.
  - spell DC = 8 + prof + mod; spell attack = prof + mod.
  - weapon to-hit = ability mod (FINESSE = better of STR/DEX; SPELL = spell ability) + prof if proficient + hit bonus.
  - weapon damage = `NdX + damage bonus`, exactly as entered: no automatic ability modifier.
  - Temp HP absorbs damage first and doesn't stack (keep the higher value). Healing from 0 HP clears death saves.
  - Mastery numbers: GRAZE = max(0, ability mod); TOPPLE DC = 8 + ability mod + prof.
- **Errors and storage states** (never claim a save that didn't happen):
  - No SD: an error with TRY AGAIN / NO-SAVE MODE. Temp sessions say saving is off.
  - Write failure: `SAVE FAILED (code)`; the old file is kept and the data stays in RAM; RETRY.
  - Damaged save: restore from backup with a notice, or if both are bad, START FRESH / TEMP SESSION. The damaged file is kept as `sheet.bad`.
  - Library files: invalid entries are skipped and counted in a toast.

## State management
- **Session**: `view` (boot/select/sheet), `session` (sd/temp), `slots[3]`, `cur`, `tab`, per-context `focus`, list `scroll` offsets, `fpeek` (expanded page: `{kind: spell|feat|weapon, name|idx, scroll}`), `amt` (keypad), `dirty`, `toast`, `fx[]` (effects), overlay stack, lid state, spell and feature libraries.
- **Character**:
  - name, class, subclass, species, background, level;
  - ab[6], saveProf bitmask, skill[18] (0/1/2);
  - hp / hpMax / temp, ac, speed, death saves;
  - slotMax[9] / slotUsed[9], spellAb, inspiration, hitDie, hdUsed;
  - weapons[≤4], spells[≤24] {name, prepared}, feats[≤16] {name, used}.
- **Saving**: autosave when leaving to the character list, on lid close and after rests. Manual SAVE NOW is in the menu. Everything else marks the session dirty (`UNSAVED`).

## Save format v5 (`fat:/data/pocketsheet/sheet.sav`, 3,760 bytes, little-endian)
Read and write field by field with byte helpers; never cast to a struct.

**Header** (16 bytes):
- `PKSH`, u16 version = 5, u8 slots = 3, u8 last slot.
- u32 CRC-32 (poly 0xEDB88320) over the whole file with these 4 bytes zeroed.
- 4 reserved.

**Slot record** (1,248 bytes at 16 + 1,248·i):

| Offset | Field |
|---|---|
| 0 | used |
| 1 | name[16] |
| 17 | class[12] |
| 29 | level |
| 30 | ab[6] |
| 36 | saveProf |
| 37 | skill[18] |
| 55 | s16 hp |
| 57 | u16 hpMax |
| 59 | u16 temp |
| 61 | ac |
| 62 | speed |
| 63 | death save successes |
| 64 | death save failures |
| 65 | slotMax[9] |
| 74 | slotUsed[9] |
| 83 | spellAb (255 = none) |
| 84 | inspiration |
| 85 | subclass[16] |
| 101 | hit die |
| 102 | hit dice spent |
| 103–111 | reserved |
| 112 | weapons 4 × 56 bytes (below) |
| 336 | spells 24 × 22: name[20], flags (bit0 used, bit1 prepared), reserved |
| 864 | features 16 × 22: name[20], u8 used (≤20), u8 flags (1 = used) |
| 1216 | species[16] |
| 1232 | background[16] |

**Weapon record** (56 bytes): 0 used; 1 name[12]; 13 flags (bits 0–1 ability, bit 2 proficient); 14 s8 hit bonus; 15 dice; 16 die; 17 type; 18 range[7]; 25 mastery; 26 s8 damage bonus; 27 props[28]; 55 reserved.

**Charset**:
- Names: `A–Z 0–9 space ' . - & / ( )`.
- Property text also allows `, + ! ? :`.

**Validation**: magic, version, size, slot count, CRC, every range, and no duplicate spell or feature names.

**Migration**: v1–v4 are read and upgraded (older layouts are in `docs/Spec (change log).md`). v3/v4 weapons copy the old magic bonus into the damage bonus. Migrated sessions are marked unsaved.

**Write procedure**:
1. Write `sheet.tmp`, checking `fwrite`, `fflush` and `fclose`.
2. Read it back and compare.
3. Delete `sheet.bak`; rename `sheet.sav` → `sheet.bak`.
4. Rename `sheet.tmp` → `sheet.sav`.

**Load procedure**:
1. Valid `.sav` → use it.
2. Otherwise a valid `.bak` → restore it, with a notice.
3. Otherwise → START FRESH or TEMP.
4. No files at all → first run: create the demo characters.

FAT is not safe against sudden power loss; say so in the README.

## Library and portrait files (on the SD card, edited on a PC)
- **`spells.txt`**:
  - `@NAME` (≤20 characters), then `level:` (C or 1–9, required), `school:`, `time:`, `range:`, `components:`, `duration:`, `tags:` (concentration / ritual).
  - Then the description; a blank line starts a paragraph; `#` lines are comments.
  - Limits: ≤300 spells, ≤1,500 characters each. Same-name entries replace built-ins.
- **`features.txt`**:
  - `@NAME`, then `category:` (class / subclass / species / background / feat / item / other), `uses:` (1–20, PB, or STR…CHA), `reset:` (long / short / short1 / none) and `tags: inspire`.
  - Then the description.
  - A feature whose name matches a weapon property (e.g. `REACH`, `BARREL`) is shown in that weapon's details.
- **Built-ins**: 45 spells and 11 features, as original short paraphrases, are in the JS `STARTER` and `FSTARTER` tables. Port them as const C tables.
- **`portrait1.bmp` – `portrait3.bmp`**:
  - Uncompressed BMP, exactly 48×48, 24-bit or 8-bit indexed; bottom-up or top-down rows.
  - Converted to RGB555 when loaded. A bad file shows a placeholder.
- **First-run content**: Slot 1 is **Jedediah Graves** (Ranger 2, from `reference/jedediah_paper_sheet.png`; HP 20/20 and level 2 were inferred). Slot 2 is **Maren Oakholt** (Cleric 5).
  - Ready-to-copy SD files: `sd-card/data/pocketsheet/features.txt` and `spells.txt` (CRLF line endings).
  - `portrait1_48x48.png` must be saved as a 24-bit BMP named `portrait1.bmp` (the prototype embeds this BMP).

## Design tokens
- **Palettes** (each channel a multiple of 8, so they're RGB555-exact).
  - Roles: bg, panel, panel2, line, text, dim, gold = focus/accent, red, green, blue = spells/edit/temp, ink = text on fills.
  - Default is **Torchlight**; the user prefers **Night**, so consider making it the default.

| Role | Torchlight | Grimoire | Elven | Parchment | Night |
|---|---|---|---|---|---|
| bg | #181010 | #200810 | #101810 | #E8D8B0 | #181020 |
| panel | #282018 | #381020 | #182818 | #D8C898 | #282038 |
| panel2 | #383028 | #501830 | #283828 | #C8B080 | #383050 |
| line | #585040 | #703048 | #405840 | #A08860 | #504868 |
| text | #F0E0C0 | #F8E8C8 | #E8F0D0 | #281810 | #F8F0D8 |
| dim | #A89878 | #C8A090 | #98B090 | #584830 | #A098A8 |
| gold | #F0A830 | #E8C050 | #D8B048 | #903010 | #F8B840 |
| red | #D84830 | #F06048 | #E06048 | #A82818 | #E04848 |
| green | #78A848 | #80C068 | #88D068 | #386820 | #58C068 |
| blue | #7898D0 | #88A8E8 | #78B0E0 | #284880 | #5890E8 |
| ink | #181010 | #200810 | #101810 | #F8F0D8 | #181020 |

- **Type**: one original 5×7 bitmap font (uppercase, digits, punctuation, and glyphs ♥ ✓ ✗ ▶ ◀ ▲ ▼ ● ○ ◆ →).
  - 6 px advance, 10 px line height.
  - Scales 1×/2×/3×/4× by pixel doubling only. Lowercase is drawn as uppercase.
- **Spacing**: 3 px outer margins on the bottom screen; 2 px gaps between adjacent controls; touch targets about 16–28 px tall.
- **Radii and shadows**: none. Square pixel UI. The only "shape" is the AC heater shield.

## Assets (all original, CC0 unless noted; see `assets/LICENSE.txt`)
- `assets/font5x7.h` (C table, ready to compile) and `font5x7.png` (editable source; 16×7 grid of 8×8 cells, magenta background).
- `assets/icons.h` (20 icons as row strings: `.` clear, `#` color 0, `+` color 1, `=` color 2) and `icons.png` (preview sheet).
- `assets/icon32.png`: the 32×32 `.nds` banner icon source. Convert it to the format the BlocksDS template expects.
- `assets/palette.txt`: the Night palette with RGB15 values. Other themes are in the table above and the JS `THEMES` table.
- `reference/jedediah_paper_sheet.png` and Jedediah's portrait: **user-supplied** material, not part of the CC0 set.
- Rules terms: SRD 5.1 / 5.2, CC BY 4.0 (attribution on the boot screen). There are no Nintendo or WotC assets and no sounds.

## Files
- `prototype/Pocket Sheet Prototype.dc.html`: the simulator shell (screens, hardware buttons, SD-card simulator, theme picker). Keyboard: arrows = D-pad, X = A, Z = B, S = X, A = Y, Q/W = L/R, Enter = Start, Shift = Select.
- `prototype/pocket-sheet-sim-v2.js`: **reference implementation** (≈110 KB).
- `prototype/support.js`: runtime for the HTML shell only.
- `docs/Spec (change log).md`: detailed spec written as a running log (v0.1 → v0.5.3). **Later sections override earlier ones.** This README reflects the final state.
- `docs/Native Handoff (v0.1 base).md`: the original toolchain, emulator and flashcart notes (still valid), plus the earlier Claude Code prompt.
- `CLAUDE_CODE_PROMPT.md`: the prompt to start the build.

## Test plan (emulator first, then hardware)
1. **melonDS**: Emu settings → DLDI → folder sync to an empty folder. Copy in `sd-card/data/pocketsheet/`, then run `pocketsheet.nds`.
2. **Core flow**:
   - Check every top-screen number for Jedediah against the rules above.
   - Hover-summary and X-expand on a spell, a feature and a weapon.
   - Spend and restore slots and feature uses; damage with temp HP; heal from 0 HP; short and long rest.
   - Edit identity, scores and proficiencies; add and remove a weapon, a spell and a feature.
   - Save, reboot and compare.
3. **Buttons only**: every flow works without touch.
4. **Repeat**: a held D-pad waits 15 frames, then repeats every 4; a held A fires once.
5. **Touch**: drag off → nothing happens; drag back and release → one action; a dialog appearing mid-press → no action.
6. **Storage**: no SD; write failure (read-only folder); a damaged `.sav` → backup; both damaged; malformed library entries skipped and reported; bad and missing portraits.
7. **Lid**: close with unsaved changes → saves, then sleeps; result shown on wake.
8. **Hardware**: only after the user confirms a flashcart compatible with their DSi XL system version.

## Remaining requirements and limits
- The user must check their DSi XL version (System Settings, top right) and buy a Slot-1 flashcart that lists support for it. Don't assume any "R4" works.
- Copying the `.nds` to the DSi's own SD card won't make it launch, and it won't install as DSiWare.
- FAT writes can corrupt on power loss; the backup only reduces the risk. The DSi power button resets without saving.
- Out of scope: dice rolling, inventory, multiclassing, sounds.
