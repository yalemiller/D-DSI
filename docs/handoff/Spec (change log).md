# Pocket Sheet: design spec v0.2

A **character sheet reference** for 5e-style tabletop play, using rules terms from SRD 5.1. It's built as DS-mode homebrew for a DSi XL launched from a Slot-1 flashcart. It works offline with no account, and stores 3 characters plus optional portraits on the flashcart's microSD.

The reference implementation is `pocket-sheet-sim-v2.js` (prototype `Pocket Sheet Prototype v2.dc.html`). Every coordinate below matches that file. If this doc and the code disagree, the code wins.

v0.1 was roll-focused (`pocket-sheet-sim.js`). In v0.2, dice rolling was removed so the app is reference-first.

## First-version features
- **The top screen is always the full sheet.** It never changes between tabs:
  - portrait, name, class, subclass, level, speed;
  - HP (and temp HP), AC, initiative, proficiency bonus, passive perception;
  - all 6 ability modifiers and scores, all 6 saves with proficiency;
  - spell DC, spell attack, remaining slots, inspiration or death saves, save status.
- **SHEET tab (home):** all 18 skills with proficiency marks and modifiers. A strip at the bottom shows the focused skill's math.
- **COMBAT tab:** HP readout, inspiration toggle, keypad with Damage / Heal / Set temp HP, death saves.
- **WEAPONS tab:** up to 4 weapons. Each card shows proficiency, name, ability used, to-hit, damage dice + modifier, damage type and range.
  - To-hit = ability mod + proficiency (if proficient) + magic bonus.
  - Damage = NdX + ability mod + magic bonus.
  - Ability options: STR, DEX, FINESSE (the better of STR/DEX), or SPELL (your spellcasting ability).
  - A edits a weapon; X or "+ ADD WEAPON" adds one (the name comes first, on the keyboard).
  - **Weapon mastery** (optional, one per weapon): CLEAVE, GRAZE, NICK, PUSH, SAP, SLOW, TOPPLE or VEX.
    - Set it only for weapons your character has mastered.
    - The card shows a blue ◆ tag with any numbers worked out: `GRAZE 2` (the ability modifier, minimum 0) and `TOPPLE DC 13` (8 + ability mod + proficiency bonus).
    - When a weapon with a mastery has focus, the top-screen strip shows a one-line effect summary.
- **SPELLS tab:** spell DC and attack, slot gems for levels 1–9 (tap to spend or restore), SET SLOTS mode to change the maximums.
- **EDIT tab:** three pages.
  - INFO: name, class, subclass, level, max HP, AC, speed, spell ability.
  - ABILITIES: 6 scores plus save proficiency for each.
  - SKILLS: none → proficient → expertise.
- **Start menu:** Resume, Save now, Long rest (asks first), Edit sheet, Characters.
- **Character select:** 3 slots with a portrait preview, new character, delete (asks first).
- **Saving:** versioned file with a CRC and a backup. v1 files are read and upgraded. Clear messages for a missing SD card, a failed write, or a damaged save.

## Hardware rules applied
- Two 256×192 screens, designed at 1×. Previews use whole-number scaling only.
- Only the bottom screen takes touch input; actions fire on release.
- Nothing crosses the hinge.
- DS mode only.

## Visual system
- **Font**: original 5×7 bitmap font, CC0 (`assets/font5x7.h`, `.png`).
  - 6 px advance; large numbers at 2×/3×/4× by pixel doubling.
- **Palette** (`assets/palette.txt`), RGB555-exact:

  | Name | Hex | Name | Hex |
  |---|---|---|---|
  | bg | #181020 | panel | #282038 |
  | panel2 | #383050 | line | #504868 |
  | text | #F8F0D8 | dim | #A098A8 |
  | gold | #F8B840 | red | #E04848 |
  | green | #58C068 | blue | #5890E8 |

- **Focus**: 2 px gold outline with a 1 px ink line inside it.
- **Pressed**: the control fills with the text color and its label switches to ink.
- **Edit contexts** (EDIT tab, SET SLOTS) use a blue footer and blue active tab.
- **Status strip** (top screen, y=178–191) carries toasts (110 frames) and stepper previews. Nothing else animates except the cursor blink.

## Top screen: the sheet (x, y, w, h)
The hierarchy goes: who the character is → how they survive (HP, AC) → abilities with saves → secondary numbers → status.

| Tier | Element | Rect / position |
|---|---|---|
| 1 | Portrait | 1,1,48,48, 1 px frame. Placeholder: initials at 2×. Bad file: red frame, "BAD FILE" |
| 1 | Name | 55,4 at 2× (1× if wider than 199 px) |
| 2 | Class + level | 55,24: class (text) then LEVEL n (gold), 8 px gap |
| 3 | Subclass | 55,36 (dim) |
| 1 | Hit points | Box 1,52,120,42. Label HIT POINTS (or DOWN in red) at 5,56; temp right-aligned at 117,56 (blue). Current 3× at 5,68; /max 2× dim, baseline-aligned. No bar |
| 1 | AC | Heater shield 122,52,48,42: dim outline, panel fill inset 2 px. Flat top with 2 px/1 px clipped corners; straight sides for the top third, then each side insets by round(23·p^1.8) to a point (p = 0→1 over the remaining rows). Value 3× centered at y=56; "AC" (dim) centered at y=79 |
| 2 | INIT / SPEED | Boxes 172,52,83,20 and 172,74,83,20. Label 1× at +5,+7; value 2× right-aligned at 250 |
| 2 | Abilities + saves | 6 tiles 41×45 at x=43·i, y=97. Name (dim) at +4,+4; score right-aligned at +37 (dim). Modifier 2× centered at +13. Save band +30, h15, panel2: "SAVE +n" centered, with the value gold when proficient (the label drops if it doesn't fit) |
| 3 | Secondary | Panel 0,145,256,31. y=149: PROF, PASSIVE PERCEPTION. y=162: SPELL DC, ATK, then slot gems 4×7 per level (wraps to y=171). Labels dim, values text |
| 3 | Status strip | 0,178,256,14: inspiration + hit dice (or death saves at 0 HP); SAVED / UNSAVED / TEMP SESSION right-aligned. Replaced by toasts and banners |

## Bottom screen
- **Tabs**: 5 tabs, 51 px wide (the last one 52), 16 high at y=0: SHEET, COMBAT, WEAPONS, SPELLS, EDIT. Active tab is gold, or blue for EDIT.
- **Footer**: 0,180,256,12.

| Tab | Layout |
|---|---|
| SHEET | Skill rows 124×16 (drawn 15 tall), x=3 and x=129, y=17+16r, 9 rows. Glyph at +3: ○ none (line), ● proficient (gold), ◆ expertise (blue); expertise modifiers are also blue. Name at +11, modifier right-aligned to +120. Breakdown strip 3,163,250,15, e.g. `DEX +2 + EXPERTISE +6 (2X PROF) = +8` |
| COMBAT | HP readout 3,19,168,22. INSPIRATION chip 174,19,79,22. Amount 3,44,91,20. Keypad 29×22 at x=3/34/65, y=67/91/115/139. DAMAGE 100,44,153,28; HEAL 100,76,153,28; SET TEMP HP 100,108,153,22 (muted until an amount is entered). Death saves label at 100,135; SAVES 100,145,75,18; FAILS 178,145,75,18 |
| WEAPONS | Cards 250×36 at x=3, y=18+38i (up to 4).<br>Left: ● or ○ proficiency at +5,+6; name at +13,+6; ability line (dim) at +13,+15; damage at +13,+26 with the type in dim after it.<br>Mastery tag on the ability line: ◆ + tag in blue, starting 8 px after the ability text.<br>Right: to-hit 2× gold right-aligned at +244,+4, with a "TO HIT" label to its left; range right-aligned at +26.<br>"+ ADD WEAPON" outline row takes the next slot (36 tall if there are no weapons, else 22). Empty-state message at y=80–106 |
| SPELLS | SAVE DC 3,19,82,22; SPELL ATK 87,19,82,22; SET SLOTS / DONE 171,19,82,22. Slot rows 124×25, levels 1–5 at x=3 and 6–9 at x=129, y=44+27r. Gems 19×19 at row x+28+23i. Empty state panel 3,44,250,133 |
| EDIT | Page buttons 82×14 at x=3/87/171, y=17. INFO: rows 250×18 at y=33+18i (8 fields). ABILITIES: score 3,y,168,21 + SAVE PROF 174,y,79,21, y=34+24i. SKILLS: rows 124×15 at y=32+15r. A cycles none → proficient → expertise; **Y jumps straight to expertise** (or back to proficient). A toast confirms the new state and total. Legend at y=170: `○ NONE  ● PROF +3  ◆ EXPERTISE +6` (values follow the current proficiency bonus) |

### Overlays
- **Weapon editor**:
  - Title "EDIT WEAPON", n OF m.
  - Rows 250×16 at y=18+16i: NAME ▶, ABILITY (cycles), PROFICIENT (toggles), MAGIC BONUS ▶ (−5…+5), DAMAGE DICE ▶ (1–9), DIE SIZE (cycles D4–D12), DAMAGE TYPE (cycles 13 types), MASTERY (cycles NONE + 8), RANGE ▶ (≤7 chars, optional).
  - DELETE… 3,163,100,16 (asks first; defaults to KEEP) and DONE 153,163,100,16.
  - The top-screen strip previews e.g. `MACE  +5 TO HIT  1D6+2 BLUDGEONING`. When MASTERY has focus it shows that mastery's effect instead.
  - B / Start = done.
- **Menu**: 5 rows 240×19 at y=22+21i. The focused item's description shows at y=134+.
- **Stepper**:
  - Value at 4× (y=36), range at y=74.
  - Buttons −big/−step/+step/+big 58×26 at x=4/66/128/190, y=96. CANCEL 4,140,120,26; OK 132,140,120,26.
  - The top screen's status strip previews the change, e.g. `STR 14 → 15  MOD +2 → +2`.
- **Keyboard**:
  - Keys 23×24 at x=3+25c, y=20+26r: `1234567890 / QWERTYUIOP / ASDFGHJKL' / ZXCVBNM-./`.
  - CANCEL / SPACE / DEL / OK at y=126. Echo strip at y=157.
  - The top screen shows the text at 2× with a counter.
  - Subclass may be left empty.
- **Notice/confirm**:
  - Body text on the bottom; buttons at y=136.
  - The top screen dims (palette or software darken) and shows the title card 16,64,224,64.
  - Destructive confirms default to KEEP or CANCEL.

## Controls
| Input | Action |
|---|---|
| D-pad | Move focus to the nearest control in that direction. Repeats after 15 frames, then every 4 (`keysSetRepeat(15,4)`) |
| A | Activate: press a key, cycle a value, open a stepper or keyboard. Does nothing on SHEET skills (they're read-only there) |
| B | Back or cancel. COMBAT: clear the amount. Keyboard: delete (closes when empty) |
| Y | EDIT › SKILLS: toggle expertise |
| X | WEAPONS: add weapon. EDIT: next page. SPELLS: toggle SET SLOTS. Select screen: delete |
| L / R | Previous / next tab (wraps; press only) |
| Start | Menu. Keyboard: done |
| Select | Jump to EDIT and back to the previous tab |
| Stylus | Press highlights; dragging off cancels; releasing on the same control fires once. Opening a dialog cancels any press in progress |
| Lid | On close: save if there are unsaved changes, then sleep. Report the result on open |

## Save format v4 (`fat:/data/pocketsheet/sheet.sav`, 2272 bytes, little-endian)

**Header**, 16 bytes:
- `PKSH` magic.
- u16 version = 4.
- u8 slot count = 3.
- u8 last opened slot.
- u32 CRC-32 (poly 0xEDB88320) of the whole file, with these 4 bytes zeroed.
- 4 reserved bytes.

**Slot record ×3**, 752 bytes each, at offset 16 + 752·i:

| Off | Size | Field (range) |
|---|---|---|
| 0 | u8 | used 0/1 |
| 1 | 16 | name, `A–Z 0–9 space ' . - &`, zero-padded, 1–16 chars |
| 17 | 12 | class, 0–12 chars |
| 29 | u8 | level 1–20 |
| 30 | 6×u8 | STR..CHA, 1–30 |
| 36 | u8 | save proficiency bitmask, <64 |
| 37 | 18×u8 | skill proficiency: 0 none, 1 proficient, 2 expertise = 2 × prof bonus (alphabetical skill order) |
| 55 | s16 | HP 0–max |
| 57 | u16 | max HP 1–999 |
| 59 | u16 | temp HP 0–999 |
| 61 | u8 | AC 0–40 |
| 62 | u8 | speed 0–120 |
| 63/64 | u8 | death save successes / failures, 0–3 |
| 65 | 9×u8 | max slots per level 0–4 |
| 74 | 9×u8 | used slots, ≤ max |
| 83 | u8 | spell ability 0–5, 255 = none |
| 84 | u8 | inspiration 0/1 |
| 85 | 16 | **subclass** (new in v2), 0–16 chars |
| 101 | u8 | hit die size 6/8/10/12 (0 in v3 files = D8) |
| 102 | u8 | hit dice spent, ≤ level |
| 103 | 9 | reserved |
| 224 | 24×22 | **known spells** (v4): name[20] (library key), u8 flags (bit0 used, bit1 prepared), u8 reserved. Packed from index 0 |
| 112 | 4×28 | **weapons** (new in v3), see below |

**Weapon record**, 28 bytes, at slot offset 112 + 28·j:

| Off | Size | Field (range) |
|---|---|---|
| 0 | u8 | used 0/1 |
| 1 | 12 | name, 1–12 chars |
| 13 | u8 | bits 0–1: ability (0 STR, 1 DEX, 2 FINESSE, 3 SPELL); bit 2: proficient; other bits 0 |
| 14 | s8 | magic bonus, −5…+5 |
| 15 | u8 | number of dice, 1–9 |
| 16 | u8 | die size: 4, 6, 8, 10 or 12 |
| 17 | u8 | damage type 0–12: SLASHING, PIERCING, BLUDGEONING, FIRE, COLD, LIGHTNING, THUNDER, ACID, POISON, NECROTIC, RADIANT, FORCE, PSYCHIC |
| 18 | 7 | range text, 0–7 chars (charset adds `/`) |
| 25 | u8 | weapon mastery 0–8: NONE, CLEAVE, GRAZE, NICK, PUSH, SAP, SLOW, TOPPLE, VEX. (This was a reserved zero byte in early v3 files, so those read as NONE) |
| 26 | 2 | reserved |

Used weapons are packed from index 0 with no gaps.

**Older files** are accepted:
- v1: 96-byte records, 304 bytes.
- v2: 112-byte records, 352 bytes.

Missing fields load empty (no subclass, no weapons). The session is marked unsaved, so the next save writes v3.

Read and write field by field with byte helpers; never cast to a struct. Any failed check (magic, version, size, slot count, CRC, range) rejects the file.

**Write procedure:**
1. Write `sheet.tmp`, checking every return.
2. Read it back and compare byte for byte.
3. Delete `sheet.bak`, then rename `sheet.sav` → `sheet.bak`.
4. Rename `sheet.tmp` → `sheet.sav`.

Report any failure; the old file and the in-memory data are kept. **FAT is not safe against sudden power loss.**

**Load procedure:**
1. No files → first run: create the sample character and save.
2. Valid `.sav` → use it.
3. `.sav` invalid and `.bak` valid → restore the backup, keep the damaged file as `sheet.bad`, and tell the user.
4. Both invalid → keep `sheet.bad` and offer Start fresh or Temp session.

**No SD card:** show Try again or No-save mode. A temp session never claims it saved.

## Portraits
- **Files:** `fat:/data/pocketsheet/portrait1.bmp` … `portrait3.bmp`, matched to slot number.
  - Optional; deleting a character leaves its file on the card.
- **Accepted format:** uncompressed Windows BMP, exactly **48×48**.
  - 24-bit, or 8-bit indexed (palette ≤256).
  - Bottom-up or top-down rows. DIB header ≥40 bytes.
  - MS Paint "24-bit Bitmap" works. A 24-bit file is 6,966 bytes.
- **Validation:**
  - Signature `BM`, planes = 1, bpp ∈ {8, 24}, compression = 0.
  - Exact 48×48 size; pixel data and palette must fit inside the file; palette index < color count.
  - A failure shows the "BAD FILE" frame. The rest of the sheet still works.
- **Display:** pixels are converted to RGB555 at load (`>>3` per channel). The portrait is drawn into the top screen's 16-bit bitmap.
- **Loading:** a portrait loads when a character opens or a slot is previewed. It's cached until a reboot or a different slot is chosen. The file is about 7 KB, so the load is effectively instant.

## Navigation map
1. Boot (mount SD) → errors if needed → Character select.
2. Open a character → Sheet. Tabs: SHEET ⇄ COMBAT ⇄ WEAPONS ⇄ SPELLS ⇄ EDIT. A weapon card opens the weapon editor.
3. Start menu → Save, Long rest, Edit sheet, or Characters (autosaves, then returns to select).
4. New character → keyboard (name) → EDIT tab.

## Weapon mastery effect lines (top strip, ≤42 chars, original paraphrases)
| Mastery | Line |
|---|---|
| CLEAVE | `CLEAVE: ON HIT, ATTACK A 2ND FOE NEAR IT` |
| GRAZE | `GRAZE: ON A MISS, STILL DEAL n DAMAGE` (n = ability mod, min 0) |
| NICK | `NICK: LIGHT EXTRA ATTACK, NO BONUS ACTION` |
| PUSH | `PUSH: ON HIT, SHOVE LARGE OR SMALLER 10 FT` |
| SAP | `SAP: ON HIT, TARGET HAS DIS ON NEXT ATTACK` |
| SLOW | `SLOW: ON HIT, TARGET SPEED -10 FT` |
| TOPPLE | `TOPPLE: ON HIT, CON SAVE DC n OR PRONE` (n = 8 + ability mod + prof) |
| VEX | `VEX: ON HIT, ADV ON YOUR NEXT ATTACK` |

## v0.2 additions (latest)
- **Weapon damage** shows the dice plus the magic bonus only. The ability modifier is no longer added to damage; to-hit is unchanged.
- **Rests**:
  - COMBAT tab has SHORT REST (100,150,75,24) and LONG REST (178,150,75,24).
  - The right column is now DAMAGE y44 h24, HEAL y70 h24, SET TEMP HP y96 h18, death saves y127.
  - Hit dice show at 4,167 and in the top strip. The Start menu also has both rests.
  - Short rest overlay: dice to spend (− / value / +), HP regained (−10/−1/value/+1/+10), USE AVERAGE (n × ((die+1)/2 + CON)), CANCEL/REST. Start = rest.
  - Long rest (asks first): full HP, all slots, all hit dice; temp HP and death saves cleared. Both rests save.
  - The EDIT › INFO page gains a HIT DIE row (rows are now 16 px).
- **Spells tab is one screen** (slots + list):
  - **Slot strip** at y=18, h=28: one 25 px chip per level that has slots (gap 2), showing the level digit and 4×8 gems.
    - A or a tap spends a slot; Y restores one. The footer switches to SPEND/RESTORE while a chip has focus.
    - SET (34 wide, right after the chips) opens the **slot maximums** overlay: 9 rows 124×28, stepper 0–4, DONE.
    - "+ ADD" is at 205,18,48,28.
  - **List**: rows 226×16 at y=49+16r, 7 visible, with ▲/▼ page buttons at x=231.
    - Each row: level badge (C or 1–9), ● prepared, name (dim if unprepared), ◆ concentration, R ritual.
    - Legend at y=169 includes the prepared count. A opens the details; Y toggles prepared; X adds.
    - D-pad ▲ from the first row jumps to the slot strip.
  - The top screen stays the full sheet.
  - **Spell details**:
    - Top screen: name 2×, level and school, CONC/RITUAL tags, a TIME/RANGE/COMPONENTS/DURATION grid, then the description (40 columns, 11 lines from y=78, ▲▼ markers).
    - Bottom screen: PREPARED toggle, USE A n-LEVEL SLOT (spends one), scroll ▲/▼, previous/next spell, REMOVE… (asks first), BACK. L/R scroll the text.
  - **Add spells picker**:
    - Level filter ALL/C/1–9 (21×16 at x=3+23j).
    - Library list with ✓ on known spells; A adds or removes. The top screen previews the focused spell's full text (L/R scroll).
    - Up to 24 spells per character.
- **Spell library**:
  - 41 built-in starter spells (original short paraphrases with 2024-style numbers), plus optional `fat:/data/pocketsheet/spells.txt`, edited on a PC.
  - First run writes a commented template. A spell in the file with the same name as a built-in one replaces it.
  - **Format**:
    - Each spell starts with `@NAME` (≤20 chars).
    - Optional `level:` (C or 1–9, required), `school:`, `time:`, `range:`, `components:`, `duration:`, `tags:` (concentration/ritual).
    - Then description lines; a blank line starts a new paragraph. `#` lines are comments.
  - Invalid entries are skipped and counted (a toast reports them). Limits: 300 spells, 1,500 characters per description, ASCII only.
  - Spells are saved by name. A name missing from the library shows "NOT IN YOUR SPELL LIBRARY".

## Color schemes
There are five palettes; every channel is a multiple of 8, so each maps exactly to RGB555. On hardware this is a palette swap (sub screen) plus redraw (top). It could become a saved setting later.

| Role | Torchlight (default) | Grimoire | Elven | Parchment (light) | Night (original) |
|---|---|---|---|---|---|
| bg | #181010 | #200810 | #101810 | #E8D8B0 | #181020 |
| panel | #282018 | #381020 | #182818 | #D8C898 | #282038 |
| panel2 | #383028 | #501830 | #283828 | #C8B080 | #383050 |
| line | #585040 | #703048 | #405840 | #A08860 | #504868 |
| text | #F0E0C0 | #F8E8C8 | #E8F0D0 | #281810 | #F8F0D8 |
| dim | #A89878 | #C8A090 | #98B090 | #584830 | #A098A8 |
| gold (focus/accent) | #F0A830 | #E8C050 | #D8B048 | #903010 | #F8B840 |
| red | #D84830 | #F06048 | #E06048 | #A82818 | #E04848 |
| green | #78A848 | #80C068 | #88D068 | #386820 | #58C068 |
| blue | #7898D0 | #88A8E8 | #78B0E0 | #284880 | #5890E8 |
| ink (text on fills) | #181010 | #200810 | #101810 | #F8F0D8 | #181020 |

## Weapons tab v0.3 (latest, replaces the earlier card layout)
- **Card** 250×36 at y=18+38i.
  - Row 1 (y+5): NAME (text), then ◆ MASTERY (blue) 8 px after it; range right-aligned (dim).
  - Row 2 (y+17): to-hit 2× gold + "HIT" (dim, y+24); damage 2× (text) at x+76 + short type (dim). "NOT PROF" right-aligned when the weapon isn't proficient.
  - A or a tap opens **Weapon details**.
- **Weapon details**:
  - Top screen:
    - Name 2×; ability / proficiency / magic line.
    - TO HIT box 1,35,82,40 (value 3× gold). DAMAGE box 85,35,170,40 (dice 3×, or 2× if wide; type right-aligned).
    - ABILITY / PROF / RANGE chips at y=77.
    - ◆ MASTERY: NAME at y=99 with a full effect description (41 cols, up to 7 lines from y=111). The GRAZE damage and TOPPLE DC are filled in.
  - Bottom screen: EDIT WEAPON 3,22,250,28 (gold); ◀ PREVIOUS / NEXT ▶ at y=56; summary panel 3,88,250,62; BACK 153,156,100,20. L/R switch weapon; B goes back.
  - The editor keeps the details card on the top screen while you edit.
- The sample cleric has demo masteries (MACE SAP, LT CROSSBOW SLOW, WARHAMMER PUSH, not proficient), even though clerics don't normally get Weapon Mastery.

## v0.3: icons and game feel (latest)
- **Icons**: 20 original pixel icons (`assets/icons.png`, `assets/icons.h`, CC0). Each pixel is clear, color 0, 1 or 2, and the colors are picked from the palette at draw time.
  - Large (8–11 px): sword, arrow, heart (with plus), shield, skull, fire (2 frames), moon, star, bolt, boot, eye.
  - Mini (7 px): page, heart, sword, spark, quill, skull, star, shield.
- **Tabs** (icon + flavor name): ◻ SHEET, ♥ FIGHT, ⚔ ARMS, ✦ MAGIC, ✎ EDIT. Inactive icons tint red for FIGHT and blue for MAGIC.
- **Top screen icons**:
  - HP label: heart (a skull at 0 HP); temp HP: shield.
  - INIT: bolt (gold); SPEED: boot.
  - Passive perception: eye; spell DC: spark.
  - Status strip: star + INSPIRED / NO INSPIRATION. At 0 HP, failed death saves show as skulls.
- **FIGHT tab**:
  - Heart HP readout; INSPIRED/UNINSPIRED chip with a star.
  - DAMAGE with a sword, HEAL with a heart, SET TEMP HP with a shield. Icons keep their color when the button is muted.
  - Skull before DEATH SAVES; failed saves are skulls.
  - SHORT REST has an animated campfire; LONG REST has a moon.
- **ARMS**: a sword (melee) or arrow (range contains "/") icon at card x+5,y+4, with the name at x+20. The details card shows the icon at 2× at the top right.
- **Character select cards**: heart before HP, shield before AC. The boot splash has a 2× sword with sparkles and a ◆ divider.
- **Motion** (60 fps, never blocks input; on hardware it's drawn in software into the back buffer):
  - Damage: HP box shakes ±2 px for 12 frames, red border flash for 14 frames, the HP number counts down over 18 frames, and a red "−N" at 2× rises 16 px over 40 frames.
  - Heal: green flash, count up, a rising "+N", green sparkles in the HP box for 36 frames.
  - Temp HP: blue flash and a rising "+N".
  - Inspiration on: gold sparkles over the status strip.
  - Long rest: 90-frame "A NIGHT PASSES..." scene: 3× moon, twinkling stars, a fade in/out (master brightness or blend fade on hardware).
  - Short rest: 80-frame "YOU REST BY THE FIRE" scene: 3× flickering campfire, sparks, and an HP / hit-dice summary.
- **Copy** has a bit more personality while staying ≤42 chars:
  - "OUCH! −7  HP 24/38", "MAREN IS DOWN!", "BACK ON YOUR FEET!", "GOOD AS NEW!";
  - "A WARD OF 5 TEMP HP SHIMMERS", "INSPIRED!", "A NIGHT PASSES. FULLY RESTORED".

## v0.4: Jedediah Graves, the FEATS tab, and save v5 (latest)
- **First run** creates two characters:
  - Slot 1: **Jedediah Graves**, Ranger 2, Human (Half-Elf), Wayfarer, built from his paper sheet. HP 20/20 and level 2 are inferred: the sheet left them blank.
  - Slot 2: Maren.
  - In the simulation, Jed's portrait (the user's art, scaled to 48×48 24-bit) is written to `portrait1.bmp`. The preview is in `assets/jed_portrait_preview_4x.png`.
- **Top screen identity**:
  - Line 2: CLASS (+ ", SUBCLASS"), truncated to fit, followed by LEVEL n in gold, 8 px after the class.
  - Line 3 (dim): SPECIES, BACKGROUND.
- **Tabs**: five 47 px tabs with icon + label (SKILLS, FIGHT, ARMS, MAGIC, **FEATS** with a gold book icon), then EDIT as a 21 px icon-only quill tab at x=235 (blue when active). L/R wrap through all six; Select jumps to EDIT and back.
- **FEATS tab**:
  - **Trackers**: up to 4 chips (124×24, 2 per row from y=18) for features with uses. Each shows the name, reset rule (LONG REST / SHORT REST / 1 PER SHORT / MANUAL) and gold pips (or n/max above 8). A spends one; Y gives one back.
  - **List**: every feature (rows 226×16 from below the chips, scrolling), with a category badge (CLS/SUB/SPC/BKG/FT/ITM/OTH) and n/max for tracked ones. A opens **Feature details**:
    - Top screen: name, category, uses line, scrolling text.
    - Bottom screen: USE ONE, +1 BACK, RESET / RELOAD (manual trackers), scroll, previous/next, REMOVE…, BACK.
  - **+ ADD** (or X) opens the picker: a category filter and the library list (✓ = on the sheet). A adds or removes; the focused entry's full text previews on the top screen. Up to 16 per character.
- **Feature library**: 11 built-in SRD-style entries plus `fat:/data/pocketsheet/features.txt`.
  - Each entry: `@NAME`, then `category:`, `uses:` (1–20, PB, or STR…CHA), `reset:` (long / short / short1 / none) and `tags: inspire`, then the description.
  - Jed's homebrew lives in that file: FAMILIAR FRIEND, BARREL (6, manual), BLARING, FAN FIRE, MISFIRE, LANGUAGES, TOOLS, REACH, FINESSE. On hardware, copy `sd-card/data/pocketsheet/features.txt` and `spells.txt` from this project.
- **Rests**:
  - Long rest: resets every tracker except MANUAL ones, and sets the inspiration star if a feature has `inspire` (RESOURCEFUL).
  - Short rest: resets SHORT trackers and gives one use back to SHORT1 trackers.
- **Weapons**:
  - The editor has HIT BONUS (−5…+10: magic, Archery), DAMAGE BONUS (−10…+30, added to the dice as typed: no automatic ability modifier) and PROPERTIES (≤28 chars, comma-separated). The editor list scrolls (9 visible).
  - Details: properties show in gold. Any property whose name matches a library entry (e.g. BARREL 6 → BARREL) shows its text, so L/R scroll.
  - An arrow icon appears when the range contains "/" or the properties mention AMMUNITION or BARREL.
  - Jed: LUCINDA +7 to hit (DEX +3, prof +2, Archery +2), 1d6+3 piercing, VEX, BARREL 6, BLARING, FAN FIRE. WHIP +5 (finesse), 1d4+3 slashing, SLOW, FINESSE, REACH, 10 ft.
- **Spells**: built-ins add HUNTER'S MARK, GOODBERRY, ENSNARING STRIKE and FIND FAMILIAR (original paraphrases). SILVERED SHELL (the user's homebrew) is in spells.txt.
- **EDIT › INFO** scrolls (9 visible) and adds SPECIES and BACKGROUND.
- **Keyboard**: now 5 rows (keys 23×22, y=18+24r). Row 4 is `ZXCVBNM.,/` and row 5 is `-&()+!?:`. Specials at y=139; echo strip at y=164.

### Save format v5 (3,760 bytes; slot record 1,248 bytes)
- 0–111: same as v4. 101 = hit die, 102 = hit dice spent.
- **Weapons** 4×56 bytes at 112:
  - 0 used; 1 name[12]; 13 flags (ability bits 0–1, proficient bit 2); 14 s8 hit bonus; 15 dice; 16 die; 17 type; 18 range[7]; 25 mastery.
  - 26 s8 damage bonus; 27 properties[28]; 55 reserved.
- **Known spells** 24×22 bytes at 336 (as in v4).
- **Features** 16×22 bytes at 864: name[20], u8 used (≤20), u8 flags (1 = used slot).
- **Species**[16] at 1216, **background**[16] at 1232.
- **Migration**: v3/v4 weapons copy the old magic bonus into the damage bonus (it used to apply to both). Older files get empty species, background and features.

- **Reading a feature (v0.4.1)**: pressing A on a list row (or tapping it) no longer opens a separate screen.
  - The bottom screen stays on the list. The top screen shows that feature's full card.
  - While reading, ▲▼ moves through the list and the top screen follows. L/R scroll the text. B closes it.
  - Using, restoring and resetting uses happens on the tracker chips (A uses one, Y gives one back). Removing a feature happens in the + ADD picker.
- **FEATS list in 2 columns (v0.4.2)**:
  - Cells are 112×16 (2 px gap), filled row by row, and scroll by rows. ▲/▼ page buttons sit at x=231.
  - A 2 px stripe on the left of each cell shows the category: gold = class/subclass, green = species, dim = background, blue = feat, red = item, line = other.
  - The name is truncated with "." if needed; n/max appears right-aligned for tracked features.
  - The feature being read has a blue outline.
  - ◀▶ move between the columns; ▲▼ move by rows.
- The first tab is renamed **SKILLS** (it was SHEET).
- The FEATS tab keeps its name (ABILITIES was tried and reverted).
  - Tab widths now fit their labels: each is 10 + label width + 9 px, with the spare pixels spread across the five labeled tabs. EDIT stays a 21 px quill.

## v0.5: hover summaries (latest)
- When a **spell row** (MAGIC) or a **feature row or tracker chip** (FEATS) has focus, a summary card replaces the abilities + saves row and the secondary row on the top screen (y=97–175). Identity, HP, AC, INIT/SPEED and the status strip stay.
  - Card: panel 0,97,256,79, with a 2 px colored stripe on the left (blue for spells, the category color for features).
  - Line 1 (y=101): icon, name, and a tag right-aligned at 251: PREPARED / NOT PREPARED / CANTRIP for spells, "n OF m LEFT" for tracked features.
  - Line 2 (y=112, dim): level/school, time, range, CONC/RITUAL for spells; category and reset rule for features.
  - A rule at y=122, then up to 4 description lines (41 cols, from y=126, 10 px leading); the last line ends in "..." if the text continues.
  - Bottom row (y=167): "MORE ON THE FULL PAGE" (dim, only when the text was cut) and the hint right-aligned: "A: FULL TEXT", or "A: USE ONE" on a tracker chip.
- Pressing A still opens the full text (spells: the details screen; features: the full top-screen card, closed with B).
- Moving focus off a spell or feature brings the abilities and saves back.
- **Tab order (v0.5.1)**: SKILLS, FEATS, MAGIC, ARMS, FIGHT, then EDIT (quill). L/R follow this order and wrap. Internal tab ids are unchanged (0 skills, 1 fight, 2 arms, 3 magic, 4 feats, 5 edit).
- **X = full screen (v0.5.2)**: on MAGIC (spell rows), FEATS (rows and tracker chips) and ARMS (weapon cards), X expands the focused item's full page onto the top screen (spell card, feature card, weapon card). The bottom screen doesn't change.
  - While expanded, ▲▼ move through the list and the page follows; L/R scroll the text; X or B closes it.
  - The summary hint now reads "X: FULL SCREEN". X still adds or opens the picker when nothing expandable has focus. Adding is otherwise done with the "+ ADD" buttons.
- **Expanded page matches the hover card (v0.5.3)**: one data builder feeds both views. The page uses the card's design across the top screen:
  - Panel 0,0,256,178 with the 2 px colored stripe.
  - Icon + name at 6,5, with the tag right-aligned at 251.
  - Meta line at y=16 and a second meta line at y=26 when there is one: school/components/duration for spells; ability, prof and hit bonus for weapons; the uses formula for features.
  - A rule, then the description (41 cols, 10 px leading).
  - Bottom row y=167: ▲▼ scroll marks + "L/R SCROLL" (only if the text scrolls), and "X: CLOSE" right-aligned.
  - The status row shows the character name (or a toast).
- Weapons now get the hover card too on ARMS: tag = to-hit; meta = damage + range; the description is the mastery text plus the text of each matching property.
