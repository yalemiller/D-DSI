# Pocket Sheet: native build handoff and README

## Status (each one is separate)
- Prototype demonstrated: **yes**, browser simulation only (`Pocket Sheet Prototype v2.dc.html`; v0.1 kept for reference).
- Native source written: **no**.
- Native build successful: **no**. This design environment can't compile native code.
- Emulator tested: **no**.
- Physical DSi tested: **no**. Needs a compatible flashcart.

## Hardware you still need
1. **Your DSi XL system version.** Open System Settings; the version (for example `Ver 1.4.5U`) is in the top-right corner.
2. **A Slot-1 flashcart and a microSD card.**
   - The seller must explicitly list DSi XL support for your exact version, or a kernel or firmware update that adds it.
   - Many "R4" carts are unrelated clones, so don't buy by name alone. Check:
     - the exact cart model and the website printed on its label;
     - that its current kernel supports DLDI auto-patching (most do);
     - that the update tools are still available.
   - Send me the model and label details and I'll check them against your version.
   - Using a flashcart doesn't modify the console's firmware or internal storage.
3. **A microSD card** formatted FAT32. Any small card is plenty; the app's save file is 304 bytes.

What won't work: copying the `.nds` to the DSi's own SD card won't make it appear in the system menu, and this app won't install as a DSiWare icon.

## Native stack
- C (C11) with **BlocksDS + libnds**.
- ARM9-only project using BlocksDS's **default ARM7**.
- No other libraries, no NitroFS, no network.

### Why these choices
- **Graphics: one bitmap background per screen.**
  - Top (main engine, VRAM A): **16-bit** `BgType_Bmp16`, 256×192, 96 KB. This lets a full-color portrait sit next to the UI with no palette juggling.
  - Bottom (sub engine, VRAM C): **8-bit** `BgType_Bmp8`, using the 10 colors in `assets/palette.txt`.
  - The UI is flat rectangles plus the bitmap font. Draw into RAM back buffers (96 KB + 48 KB) and `dmaCopy` each one to VRAM in VBlank only when its screen changed. Flush the data cache first.
  - Dialog dimming on the top screen: darken the back buffer in software (`((px >> 1) & 0x3DEF) | 0x8000` per pixel).
  - No sprites or tiles. Total RAM use is about 160 KB.
- **No NitroFS.** The font is a C header. The only runtime files are the save and the user's optional portraits on the SD card. This avoids the need for a loader that passes `argv`, which some flashcart menus don't do.
- **Storage.**
  - Call `fatInitDefault()` and use explicit `fat:/data/pocketsheet/...` paths.
  - On a DSi the default drive can be `sd:/`, and the DSi SD slot isn't available in DS mode anyway.
  - Create the folders with `mkdir` and ignore `EEXIST`.
  - Never write to `sd:/` or `nand:/`.
- **Frame loop.**
  - Call `swiWaitForVBlank()`, then `scanKeys()`, once per frame.
  - Use `keysDown()` for presses and `keysSetRepeat(15,4)` + `keysDownRepeat()` for D-pad navigation.
  - Each frame: one touch state-machine step, at most one action, then draw.
- **Lid.**
  - libnds puts the console to sleep when the lid stays closed for a few frames.
  - On `keysDown() & KEY_LID`, save right away if there are unsaved changes, and remember the result to show after waking.
  - Confirm in melonDS and on hardware that the save finishes before sleep. If it doesn't, change the handling (see the libnds sleep APIs).
- **Portraits:** parse 48×48 uncompressed BMPs (24-bit or 8-bit) with `fread`, validate them as described in the spec, and convert to RGB555. A bad file shows a placeholder and never stops the app.

## Source layout to write
```
pocketsheet/
  Makefile                (from the BlocksDS ARM9-only template, default ARM7)
  assets/font5x7.h        (provided, generated, CC0)
  assets/icon32.png       (provided; convert to the icon format the template expects)
  source/main.c           init video and storage, frame loop
  source/data.h/.c        Character struct (incl. subclass, 4 weapons), to-hit/damage rules, SK/AB tables, rules (mod, pb, skill/save mods)
  source/input.h/.c       key and repeat wrappers, touch press/drag/release/cancel machine
  source/draw.h/.c        back buffers, rect/box/text (scale 1–4), palette, present/dim
  source/ui.h/.c          focus + geometric D-pad nav, button lists, overlay stack, toast
  source/screen_*.c       boot, select, overview (top), sheet, combat, weapons, weapon editor, spells, edit, menu, stepper, keyboard, notice
  source/save.h/.c        serialize/parse/validate, CRC-32 (format v4; reads and migrates v1–v3)
  source/spells.h/.c      built-in starter table + spells.txt parser (template on first run), lookup by name
  source/features.h/.c    built-in feature table + features.txt parser, trackers (uses/reset), rest resets
  source/portrait.h/.c    48x48 BMP loader (24/8-bit) to RGB555, validation, per-slot cache
  source/storage.h/.c     mount, mkdir, tmp→verify→bak→rename, load cascade, temp session
```

## Windows setup (native, no virtual machine)
Check what you already have first. In PowerShell, run `where.exe make` and look for an existing `C:\msys64` or Wonderful install, so you don't install a duplicate SDK.

1. Install **Wonderful Toolchain** for Windows: https://wonderful.asie.pl/wiki/doku.php?id=getting_started
2. Open **Wonderful Toolchain Shell** from the Start menu and run:
   ```
   wf-pacman -Syu wf-tools
   wf-config repo enable blocksds
   wf-pacman -Syu
   wf-pacman -S blocksds-toolchain
   ```
   Skip `blocksds-docs` to save space; the docs are online. If `make` is missing, run `pacman -S make`.
3. **Disk space:** I couldn't confirm exact sizes, so check the totals `wf-pacman` prints before you confirm. Expect a few hundred MB for the toolchain. melonDS is roughly tens of MB.
4. Copy the ARM9-only template from `/opt/wonderful/thirdparty/blocksds/core/templates/`. Confirm its exact folder name in your install.
5. Build with `make` from the project folder. The output is `pocketsheet.nds` in the project root.

## Emulator testing (melonDS)
1. Download melonDS for Windows from https://melonds.kuribo64.net. DS mode works without BIOS dumps.
2. Turn on DLDI emulation: **Config → Emu settings → DLDI**. Point "folder sync" at an empty folder, for example `C:\pocketsheet-sd\`. melonDS patches DLDI automatically.
3. Load `pocketsheet.nds` and run the test list below.
4. Use the lid hotkey to close and open the lid.
5. To test damaged saves, edit `C:\pocketsheet-sd\data\pocketsheet\sheet.sav` with a hex editor between runs.

## Copying to your flashcart (once a compatible cart is confirmed)
1. Set up the cart's microSD exactly as its maker says (kernel or menu files in the root).
2. Copy `pocketsheet.nds` anywhere, for example `/homebrew/pocketsheet.nds`.
3. Insert the cart, start the DSi, open the cart from the system menu, then launch Pocket Sheet from the cart's menu.
4. On first run the app creates `/data/pocketsheet/sheet.sav`. To back up your characters, copy that folder to your PC.

## Test list (emulator, then hardware)
- **Core flow:** open the sample and check every top-screen value against the spec math. Edit name/class/subclass/level, scores, save and skill proficiencies. Take damage with temp HP, heal from 0 HP, spend and restore slots, long rest, save, reboot, and check the data.
- **Weapons:** add 4 (a fifth is refused with a message). Check to-hit and damage for STR, DEX, FINESSE and SPELL with and without proficiency and magic bonus. Cycle every mastery and check the GRAZE/TOPPLE numbers. Delete one; save, reboot and compare.
- **Spells:** add, prepare, remove; scroll a long description; use a slot from the details view. Edit spells.txt with a bad entry and confirm it is skipped and reported.
- **Rests:** a short rest spends hit dice and adds HP; a long rest restores everything; both save.
- **Portraits:** a valid 24-bit and a valid 8-bit 48×48 BMP both display; a wrong size or a truncated file shows "BAD FILE"; with no file you see initials.
- **Buttons only:** complete every flow above, plus rename and new character, without touching the screen.
- **Repeat:** holding the D-pad moves once, waits 15 frames, then repeats every 4. Holding A triggers once.
- **Stylus:** press, drag off and release → nothing happens. Drag back on and release → one action. Pressing while a dialog opens → no action.
- **Storage:** no SD (start without a DLDI folder) → error, then Try again or No-save mode. Write fail (read-only folder) → SAVE FAILED with the old file kept. Damaged `.sav` → backup restored. Both damaged → Start fresh or Temp.
- **Lid:** close it with unsaved changes → the save happens, then the console sleeps; on open you see the result.
- **Readability:** check on the real screens at 1×. Make sure the dim text is readable at arm's length.

## Prompt for Claude Code
> Build "Pocket Sheet", a DS-mode homebrew `.nds` using C11, BlocksDS and libnds, on Windows with Wonderful Toolchain.
>
> **Setup:** Before installing anything, check for existing tools and report sizes.
>
> **Source of truth:**
> - Behavior, layout, coordinates, copy and colors: `pocket-sheet-sim-v2.js` (the browser reference) and `docs/Pocket Sheet Spec.md`.
> - Port them faithfully, screen by screen.
>
> **Project:**
> - Start from the official BlocksDS ARM9-only template with the default ARM7.
> - Use the module layout in `docs/Native Handoff.md`.
> - Keep data and rules, input, screens and navigation, drawing, and save/storage in separate files.
>
> **Graphics:**
> - 16bpp bitmap BG3 on the top (main) engine, 8bpp on the bottom (sub).
> - Software rendering into RAM back buffers, copied to VRAM with DMA in VBlank only when changed.
> - Font from `assets/font5x7.h`; icons from `assets/icons.h` (multi-color masks, colors from the palette).
> - Effects (shake, flash, count, floating numbers, sparkles, rest scenes) are frame-counted and never block input.
>
> **Storage:**
> - `fatInitDefault()` with explicit `fat:/data/pocketsheet/` paths. No NitroFS.
> - Never write to `sd:/` or `nand:/`.
> - Save format v5 (with v1–v4 migration: weapons with hit/damage bonus and properties, hit dice, known spells, features, species, background), the 48×48 BMP portrait loader, the write procedure and the load cascade, exactly as specified.
> - Check every I/O return value and never report a save that didn't succeed.
>
> **Input:** `keysSetRepeat(15,4)`; one action per frame; touch fires on release inside the pressed control.
>
> **Lid:** Handle lid close as specified.
>
> **Delivery:**
> - Write the README with the exact tool versions you used.
> - Build with `make`, then report the build result honestly with the compiler output.
> - Produce `pocketsheet.nds` and give its path.
> - Then run it in melonDS with DLDI enabled and work through the test list.
> - Report these statuses separately: source written / build successful / emulator tested. Physical-DSi testing is the user's job.

## Remaining limitations
- No native code has been written or built yet.
- A flashcart compatible with your DSi XL's exact version is required to run the app on the console.
- FAT saving can still corrupt files on sudden power loss; the backup only reduces the risk.
- The DSi power button resets the console through the default ARM7 without saving, so save before powering off. (Closing the lid is safe.)
- The lid save-before-sleep timing has to be confirmed on hardware.
- Rules coverage is intentionally small: no dice rolling (removed in v0.2), spell lists, inventory, multiclassing or hit dice.
- Portraits are matched to slot numbers; moving a character to another slot means renaming its BMP.
