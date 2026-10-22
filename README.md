# Pocket Sheet

An offline 5e-style character sheet **reference** for the Nintendo DSi XL, built as DS-mode homebrew (`.nds`) in C11 with [BlocksDS](https://blocksds.skylyrac.net/) + libnds. The top screen is always the character sheet; the bottom screen holds tabs for skills, features, spells, weapons, combat and editing. It stores 3 characters on the flashcart's microSD. There's no dice rolling. Rules terms come from SRD 5.1/5.2 (CC BY 4.0).

The design handoff it's built from is in [`docs/handoff/`](docs/handoff/) (spec, browser prototype, assets). The build plan and architecture are in [`docs/PLAN.md`](docs/PLAN.md), and the development rules in [`docs/agent.md`](docs/agent.md).

## Status

Reported separately, as the handoff asks. Nothing below is inferred from an earlier step.

| Step | State |
|---|---|
| Prototype demonstrated | Yes (browser simulation, in `docs/handoff/prototype/`) |
| Native source written | In progress: core logic and renderer written; UI screens next (see [PLAN.md](docs/PLAN.md#milestones)) |
| Unit tests | Run in CI on every push (see the Actions tab) |
| Native build (`make`) | Not yet: the CI ROM build is added with the first screens |
| Emulator tested (melonDS) | No |
| Physical DSi tested | No. Needs a Slot-1 flashcart confirmed for your DSi XL's exact system version |

## Repository layout

```
source/core/   rules, character model, save format v5, spell/feature libraries, BMP portraits, storage (plain C11, no libnds)
source/gfx/    software renderer: palettes, rects, 5x7 text, icons (plain C11)
source/ui/     app state, navigation, screens and overlays (plain C11)
source/nds/    libnds platform code: video, input, FAT storage, frame loop
tests/         host unit tests (make -C tests)
tools/         generators (built-in spells/features/icons from the prototype)
assets/        font, icons, palette, banner icon (CC0)
sd-card/       files to copy to the microSD: data/pocketsheet/
docs/          plan, agent rules, original handoff
```

## Building

### In CI (current)

Every push runs [`.github/workflows/ci.yml`](.github/workflows/ci.yml): host unit tests with gcc (`-std=c11 -Werror`, AddressSanitizer + UBSan) and cppcheck.

### Unit tests on any PC with gcc and make

```bash
make -C tests
```

### Regenerating the built-in tables

The built-in spells, features, icons and Jedediah's portrait are generated from the prototype so they match it exactly:

```bash
node tools/gen_builtins.mjs
```

## SD card files

Copy `sd-card/data/pocketsheet/` to the root of the flashcart's microSD (so it's `/data/pocketsheet/`). It holds Jedediah's homebrew spell and feature entries (`spells.txt`, `features.txt`, edited in any text editor on a PC) and his portrait (`portrait1.bmp`, 48×48 24-bit). The app creates `sheet.sav` on first run.

## Known limits

- FAT writes aren't safe against sudden power loss. The app keeps `sheet.bak` from the previous save, which only reduces the risk.
- The DSi power button resets the console without saving. Closing the lid saves first.
- Out of scope: dice rolling, inventory, multiclassing, sounds.

## Licenses

- Font, icons, palette and banner icon: CC0 1.0 (see `assets/LICENSE.txt`).
- Rules terms: System Reference Document 5.1 and 5.2 by Wizards of the Coast LLC, CC BY 4.0.
- Jedediah Graves's portrait and paper sheet (`docs/handoff/reference/`, `sd-card/data/pocketsheet/portrait1.bmp`) are user-supplied and not part of the CC0 set.
- No Nintendo or Wizards of the Coast artwork, logos, fonts or sounds.
