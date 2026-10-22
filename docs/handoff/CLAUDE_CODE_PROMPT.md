# Prompt for Claude Code

Build **Pocket Sheet**, a DS-mode homebrew `.nds` for a Nintendo DSi XL, from the design handoff in this folder.

**Read first**
1. `README.md` (final behavior, layout, tokens, save format v5).
2. `prototype/pocket-sheet-sim-v2.js`, the reference implementation. Port it faithfully, screen by screen. Its coordinates, copy and colors are authoritative.
3. `docs/Native Handoff (v0.1 base).md` for the toolchain, emulator and flashcart notes.

**Constraints**
- C11, BlocksDS + libnds, official ARM9-only template with the **default ARM7**. Windows, Wonderful Toolchain.
- Before installing anything: check for existing tools (`where.exe make`, existing MSYS2 or Wonderful installs), report download and installed sizes, and avoid VMs or duplicate SDKs.
- DS mode only. Storage: `fatInitDefault()` with explicit `fat:/data/pocketsheet/` paths. No NitroFS. Never write to `sd:/` or `nand:/`.
- Graphics:
  - Top: 16bpp bitmap BG3 on the main engine. Bottom: 8bpp bitmap BG3 on the sub engine.
  - Software rendering into RAM back buffers, DMA to VRAM in VBlank only when a screen changed.
  - Font from `assets/font5x7.h`, icons from `assets/icons.h`.
- Input:
  - `scanKeys()` once per frame; `keysSetRepeat(15,4)` with `keysDownRepeat()` for navigation.
  - At most one action per frame. Touch fires on release inside the pressed control; opening an overlay cancels the press.
- Keep modules separate: data/rules, input, ui (focus/nav, overlays, list scroller), draw, screens, save, storage, spells, features, portrait.
- Save format v5 exactly as specified, with v1–v4 migration, tmp → verify → bak → rename writes, and the load cascade. Check every I/O return value and never report a save that didn't succeed.
- Effects (shake, flash, count, floating numbers, sparkles, rest scenes) are frame-counted and never block input. Handle the lid as specified.

**Deliver**
- Source, Makefile, and a README with the exact tool versions used, Windows build steps and the output path.
- Run `make` and report the real compiler output. Don't claim a build that didn't happen.
- Then test in melonDS with DLDI folder sync using `sd-card/data/pocketsheet/`, and work through the README test plan.
- Report separately: **source written / build successful / emulator tested**. Physical-DSi testing is the user's job once a compatible flashcart is confirmed.
