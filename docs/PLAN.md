# Pocket Sheet: build plan

This is the plan for porting the design handoff in [`docs/handoff/`](handoff/) to a native DS-mode `.nds`, written before implementation as [`agent.md`](agent.md) asks. The behavioral source of truth is [`handoff/prototype/pocket-sheet-sim-v2.js`](handoff/prototype/pocket-sheet-sim-v2.js); [`handoff/README.md`](handoff/README.md) is the final spec. Where they disagree, the JS wins.

## Constraints that shape the plan

- **C11, BlocksDS + libnds**, ARM9-only ROM with the default ARM7. No NitroFS; assets are compiled in.
- **This PC can't build it.** It has no C compiler, and drive C: has well under 1 GB free, which rules out installing Wonderful Toolchain + BlocksDS (a few hundred MB) for now. So **GitHub Actions is the build machine**: it compiles the `.nds` in BlocksDS's Docker image and runs the host unit tests with gcc. A local install is still the plan for melonDS testing once there's disk space (see the README).
- **Testable without hardware.** Everything except video/FAT/key-reading is plain C with no libnds includes, so it compiles on a PC. That covers the rules, the save format, the library parsers, the BMP loader, the storage cascade, the UI state machine and the software renderer.

## Architecture

The rule from `agent.md`: UI, business logic, persistence and platform code stay apart, and each module has one job. Dependencies only point downward in this list.

| Layer | Folder | Depends on | Job |
|---|---|---|---|
| Platform | `source/nds/` | everything below, libnds | `main.c`: video setup, frame loop, DMA to VRAM, lid. `fs_fat.c`: the storage interface on `fat:/data/pocketsheet/` (the only place a path prefix is written, so nothing can reach `sd:/` or `nand:/`) |
| App / UI | `source/ui/` | core, gfx | Session state, overlay stack, focus and geometric D-pad navigation, touch press/drag/release machine, every screen and overlay. Never touches files directly: it calls `storage` |
| Rendering | `source/gfx/` | nothing | Software drawing into RAM surfaces (16-bit top, 8-bit bottom): rects, boxes, 5×7 text at 1–4×, icons, the AC shield, dimming. Palettes/themes |
| Core | `source/core/` | nothing (C11 + libc) | Business rules and data: character model, rules math, save v5 + v1–v4 migration, CRC-32, spell and feature libraries (built-ins + `.txt` parsers), portrait BMP decoder, storage procedures over an abstract file interface, sheet actions (damage, heal, rests, slots, uses) |

Core modules:

| Module | Responsibility |
|---|---|
| `character` | `Character`/`Weapon` structs, name tables (abilities, skills, damage types, masteries), `character_new`, validation |
| `rules` | mod, proficiency bonus, skill/save/passive, spell DC/attack, weapon ability/to-hit/damage text, mastery tags and text, feature max uses |
| `text` | charset checks (`NAME_OK`, `TEXT_OK`), `clean_text`, word wrap, signed number formatting |
| `crc32` | CRC-32 (poly 0xEDB88320) |
| `save` | serialize v5 (3,760 bytes), parse v1–v5 field by field with byte helpers, migration, every range check |
| `spells` / `features` | built-in tables (generated from the prototype by `tools/gen_builtins.mjs`), `spells.txt` / `features.txt` parsers, lookups, rest resets |
| `portrait` | 48×48 24-bit / 8-bit BMP → RGB555, validation reasons |
| `fs` + `storage` | file interface; tmp → verify → bak → rename writes; load cascade (sav → bak → fresh/temp; first run) |
| `actions` | the state changes behind buttons (damage/heal/temp HP, death saves, short/long rest, slot spend/restore, feature uses, prepare/add/remove spells) |
| `demo` | first-run characters (Jedediah Graves, Maren Oakholt), Jedediah's portrait BMP, the library file templates |

## Verification

- **Unit tests** (`tests/`, plain C, `make -C tests`): every core module, plus the touch machine and D-pad navigation. Built with `-std=c11 -Wall -Wextra -Wpedantic -Werror` and AddressSanitizer + UBSan.
- **Lint**: `cppcheck` over `source/` and the strict host warning set above.
- **ROM build**: `make` in the BlocksDS Docker image; CI uploads `pocketsheet.nds` as an artifact.
- **Screenshots**: a host runner (`tests/shots.c`) drives the real app code with scripted input and writes both screens as PNGs, so screens can be compared with the prototype without an emulator.
- **Not covered here**: melonDS and hardware. Those need the toolchain installed locally (or the CI artifact) plus melonDS; physical testing needs a flashcart confirmed for the user's DSi XL.

## Milestones

1. **Foundation**: repo, CI, docs; all core modules with tests; renderer; UI framework; boot → character select → top sheet → SKILLS tab; Start menu; notices for every storage state; save/load on the SD.
2. **FIGHT**: keypad, damage/heal/temp HP with effects, inspiration, death saves, short/long rest overlays and scenes, stepper, keyboard, lid save.
3. **MAGIC and FEATS**: slot strip and maximums, spell list, details, picker; trackers, 2-column feature list, picker; hover summary card and X full page.
4. **ARMS and EDIT**: weapon cards, details, editor; EDIT INFO / ABILITIES / SKILLS.
5. **Emulator pass**: melonDS with DLDI folder sync through the README test plan, then hand over for hardware testing.

## Status

As of 2026-10-22, milestones 1–4 are written and pass every check in CI: unit and UI tests, cppcheck, and a warning-free ROM build. Ten screens are pixel-identical to the prototype. Milestone 5 (melonDS, then hardware) hasn't started.

Each step is reported separately in the [README](../README.md#status).
