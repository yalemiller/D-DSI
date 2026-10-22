# Pocket Sheet

An offline 5e-style character sheet **reference** for the Nintendo DSi XL, built as DS-mode homebrew (`.nds`) in C11 with [BlocksDS](https://blocksds.skylyrac.net/) + libnds. The top screen is always the character sheet; the bottom screen holds tabs for skills, features, spells, weapons, combat and editing. It stores 3 characters, optional 48×48 portraits, and user-editable spell and feature libraries on the flashcart's microSD. There's no dice rolling. Rules terms come from SRD 5.1/5.2 (CC BY 4.0).

The design handoff it's built from is in [`docs/handoff/`](docs/handoff/) (spec, browser prototype, assets). The build plan and architecture are in [`docs/PLAN.md`](docs/PLAN.md), and the development rules in [`docs/agent.md`](docs/agent.md).

## Status

Reported separately, as the handoff asks. Nothing below is inferred from an earlier step.

| Step | State |
|---|---|
| Prototype demonstrated | Yes (browser simulation in `docs/handoff/prototype/`) |
| Native source written | **Yes.** Every screen and overlay in the handoff: boot, character select, the top-screen sheet with hover summaries and full pages, SKILLS / FEATS / MAGIC / ARMS / FIGHT / EDIT, Start menu, notices, stepper, keyboard, short and long rests, spell and feature pickers, slot maximums, weapon details and editor. Save format v5 with v1–v4 migration, library files, portraits, lid handling |
| Unit and UI tests | **Pass in CI**: 629 checks on host gcc (`-std=c11 -Wpedantic -Werror`, AddressSanitizer + UBSan); cppcheck clean |
| Pixel comparison with the prototype | **10 screens identical** on both screens at RGB555 (select, all six tabs, the three EDIT pages, menu); see [below](#pixel-comparison) |
| Native build successful | **Yes, in CI**: BlocksDS v1.24.0, arm-none-eabi-gcc 16.2.0, no warnings. `pocketsheet.nds` is 256 KB |
| Emulator tested (melonDS) | **No.** Not run yet (see [Testing in melonDS](#testing-in-melonds)) |
| Physical DSi tested | **No.** Needs a Slot-1 flashcart confirmed for your DSi XL's exact system version |

## Getting the ROM

Every push to `main` builds it. Open the repo's **Actions** tab, pick the latest green **CI** run, and download the **pocketsheet-nds** artifact. It contains `pocketsheet.nds` and the build log.

## Repository layout

```
source/core/   rules, character model, save format v5, spell/feature libraries, BMP portraits, storage, sheet actions (plain C11)
source/gfx/    software renderer: themes, rects, 5x7 text, icons, the AC shield, dimming (plain C11)
source/ui/     app state, focus and touch handling, every screen and overlay (plain C11)
source/nds/    the only libnds code: video, input, lid, FAT storage limited to fat:/data/pocketsheet/
tests/         host unit tests, UI tests and the screenshot runner (make -C tests)
tools/         gen_builtins.mjs (built-in tables from the prototype), compare/ (pixel comparison)
assets/        font, icons, palette, banner icon (CC0)
sd-card/       files to copy to the microSD: data/pocketsheet/
docs/          plan, agent rules, original handoff
```

## Building

### In CI

[`.github/workflows/ci.yml`](.github/workflows/ci.yml) runs on every push:

- **host**: unit and UI tests, cppcheck, then `tests/shots.c` renders 48 scripted screens to PNG (the **screenshots** artifact).
- **nds**: `make` in the `skylyrac/blocksds:slim-v1.24.0` container (the **pocketsheet-nds** artifact).

### On Windows (not set up on this PC yet)

This follows the handoff's native notes. The PC it was written on had under 1 GB free on C:, so nothing was installed.

1. Check for existing tools first: `where.exe make`, and look for `C:\msys64` or a Wonderful install.
2. Install [Wonderful Toolchain](https://wonderful.asie.pl/wiki/doku.php?id=getting_started). In its shell, run:
   ```
   wf-pacman -Syu wf-tools
   wf-config repo enable blocksds
   wf-pacman -Syu
   wf-pacman -S blocksds-toolchain
   ```
   Check the download size `wf-pacman` prints before confirming. Skip `blocksds-docs`.
3. Run `make` in this folder. The output is `pocketsheet.nds`. Our CI uses BlocksDS 1.24.0.

### Unit tests on any machine with gcc and make

```bash
make -C tests
```

`make -C tests shots` also writes the screenshots to `shots/`.

### Regenerating the built-in tables

The built-in spells, features, icons and Jedediah's portrait are generated from the prototype so they match it exactly. The same run checks that `assets/font5x7.h` still matches the prototype's font.

```bash
node tools/gen_builtins.mjs
```

## SD card files

Copy `sd-card/data/pocketsheet/` to the root of the flashcart's microSD, so it ends up at `/data/pocketsheet/`. It contains:

- `spells.txt` and `features.txt`: Jedediah's homebrew entries. Edit them in any text editor on a PC; each file explains its format.
- `portrait1.bmp`: his portrait, 48×48 24-bit.

The app creates `sheet.sav` on first run, along with a template library file if one is missing. Portraits `portrait1.bmp` to `portrait3.bmp` match the slot numbers.

## Testing in melonDS

1. Download melonDS for Windows from https://melonds.kuribo64.net. DS mode works without BIOS dumps.
2. Go to **Config → Emu settings → DLDI**, turn on DLDI and point folder sync at an empty folder, for example `C:\pocketsheet-sd\`. Copy `sd-card/data/pocketsheet/` into it.
3. Load `pocketsheet.nds` and work through the test plan in [`docs/handoff/README.md`](docs/handoff/README.md#test-plan-emulator-first-then-hardware). Use the lid hotkey for the lid tests.

Two things need confirming there first:

- **Lid**: the app turns libnds's automatic sleep off while it runs. When the lid closes, it saves any unsaved changes, then turns sleep back on, and the default ARM7 puts the console to sleep. When the lid opens, it shows the result of that save.
- **Speed**: screens are redrawn in software only after input, during animations, or every 8th frame for the blinking cursor and campfire.

## Pixel comparison

`tools/compare/` runs the browser prototype and the native renderer on exactly the same frames and diffs every pixel at RGB555.

1. Download the **screenshots** artifact from a CI run and unzip it.
2. Run `node tools/compare/prepare.mjs PATH/TO/SHOTS`.
3. Serve `tools/compare/www/` with any static server, open it, and press **Run all**.

The last run reported "all screens match".

## Deliberate differences from the prototype

- **Keyboard**: names, classes and ranges refuse `, + ! ? :`. The prototype accepted them, but the save format's own range check then rejected the file on the next load. Weapon properties still allow them.
- **Damaged save notices**: these say "kept as SHEET.BAD" only when that rename actually succeeded.
- **Default theme**: Night, the user's preference. The prototype defaults to Torchlight.
- **Boot screen**: shows this build's version (V0.1) instead of the prototype's V0.3.
- **Prototype quirk, not copied**: the prototype captures the proficiency-ring colors before a theme switch, so in Night its empty rings use Torchlight's line color. The native app uses the current theme's colors.

## Known limits

- FAT writes aren't safe against sudden power loss. The app keeps `sheet.bak` from the previous save, which only reduces the risk.
- The DSi power button resets the console without saving. Closing the lid saves first.
- There's no way to pick a theme on the device yet; it's a compile-time default (`THEME_DEFAULT`).
- Out of scope: dice rolling, inventory, multiclassing, sounds.

## Licenses

- Font, icons, palette and banner icon: CC0 1.0 (see `assets/LICENSE.txt`).
- Rules terms: System Reference Document 5.1 and 5.2 by Wizards of the Coast LLC, CC BY 4.0.
- Jedediah Graves's portrait and paper sheet (`docs/handoff/reference/`, `sd-card/data/pocketsheet/portrait1.bmp`) are user-supplied and not part of the CC0 set.
- No Nintendo or Wizards of the Coast artwork, logos, fonts or sounds.
