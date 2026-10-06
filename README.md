# How to Spearfish

*Working title.* A first-person co-op game about free-diving for fish and running a floating restaurant.
One player dives with a speargun; the other runs the kitchen on the boat. Every night, after both players
go to bed, the roles swap. In solo you are always the diver and an automated kitchen cooks what you catch.

Built with **Unreal Engine 5.5+** in C++. The first playable vertical slice runs **without any authored
assets**: the world, boat, fish, UI and menus are built in code from data files, so the repository opens
and plays straight after cloning.

> **Status: vertical slice, never built with Unreal yet.** All code is type-checked natively against an
> engine API stub, and the gameplay rules have native unit tests (see [Verification](#verification)). It
> has not yet been compiled by UnrealBuildTool or play-tested in the editor, because no engine exists in
> the environment it was written in. Expect a round of fixes on the first real build.
> [Docs/DEVELOPMENT_PLAN.md](Docs/DEVELOPMENT_PLAN.md) lists what is verified and what is not.

## What is in the slice

**Diving**
- 3D swimming with buoyancy and a hard water surface.
- Breath-hold and tank air. Blacking out costs you the dive bag.
- Depth zones, wetsuit depth limits, stings and boundary currents.

**Fishing**
- Speargun with a physical line: reel, drag and payout, snapping, wrapping around rocks.
- The line can tow you, drag physics props and hook your partner.

**Fish**
- 20 species with schooling, fleeing, ambushers, predators that hunt other fish, and stinging drifters.
- Hooked fish fight and run for cover.

**Restaurant**
- Guests with patience, multi-fish recipes and seven cooking minigames (fillet, chop, season, grill, fry, simmer, plate).
- Quality, tips and reputation.

**Co-op**
- Asymmetric information: the chef's tablet shows the orders; the diver hears quick radio calls and push-to-talk voice.
- Roles swap nightly.
- A missing partner hands the kitchen to the auto-chef.

**Progression**
- 31 pieces of visible gear across 8 slots (speargun, reel, tank, wetsuit, mask, fins, bag, light).
- 8 boat and restaurant upgrades.
- 3 regions unlocked with money and reputation, sailed to overnight.
- A field journal of species and finds.

**World**
- A deterministic seabed per region: reefs, kelp, a wreck, caves and an island hub with a dock shop.
- Day and night lighting with Lumen and volumetric fog, plus underwater post-processing.
- Daily loot: giant clams with pearls, salvage crates, rare events such as a legendary fish or a storm chest.

## Quick start

1. Install **Unreal Engine 5.5** (or newer 5.x) with a C++ toolchain (Visual Studio 2022, Xcode or clang).
2. Install Git LFS (`git lfs install`). It is needed once binary assets are added.
3. Clone the repository and open `HowToSpearfish.uproject`. Accept the prompt to build the module.
4. Press **Play**:
   - **Standalone PIE** starts a solo session immediately.
   - **Co-op:** set *Play > Number of Players* to 2 and *Net Mode* to *Play As Listen Server*.
   - To see the main menu instead, untick *Project Settings > Game > How to Spearfish > Auto Start Session In PIE*.
5. Optional, for a better look: run `Tools/Editor/bootstrap_content.py` from the editor (*Tools > Execute
   Python Script*). It creates the base material, the translucent ocean surface and underside, and the
   underwater post-process material. Pass `--datatables` to also import the JSON data as DataTables.

### Co-op outside the editor

The host picks *Host co-op* in the main menu, which listens on port 7777. The partner enters the host's
address under *Join*. Voice chat is push-to-talk on **V**. The slice uses direct IP over the Null online
subsystem; Steam or EOS sessions can replace `USpearfishGameInstance::JoinGame` later.

## Controls

| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move / swim (where you look) | W A S D | Left stick |
| Look | Mouse | Right stick |
| Up / down | Space / C or Ctrl | A / B |
| Sprint (burns air) | Shift | Left stick click |
| Shoot | Left mouse | Right trigger |
| Reel / pull on the line | Hold right mouse | Left trigger |
| Release the line | R | Y |
| Use, bag a fish, climb, cook | E | X |
| Dive light | L | D-pad down |
| Tablet (chef anywhere, diver on deck) | Tab | View |
| Journal | J | D-pad up |
| Quick radio messages | 1 to 6 | |
| Push to talk | V | |
| Help / menu | F1 or H / Esc | Menu |

In menus and minigames: W/S or arrows to select, A/D to switch tabs, E/Space/left mouse to confirm, Esc to go back.

### Development console commands

Available in non-shipping builds:
- `SpearfishMoney 500`
- `SpearfishHour 18`
- `SpearfishEndDay`
- `SpearfishSpawnFish GoldenTrevally`
- `SpearfishRole Chef`
- `SpearfishEvent StormTreasure`
- `Spearfish.ReloadData`

## Repository layout

```
HowToSpearfish.uproject      Engine 5.5, plugins: EnhancedInput, ProceduralMeshComponent, OnlineSubsystemNull, ...
Config/                      Engine, game, input and editor settings (no secrets; local overrides are gitignored)
Content/Data/Source/*.json   All game content: fish, gear, recipes, guests, regions, loot, upgrades, events
Source/HowToSpearfish/
  Rules/                     Engine-independent gameplay rules (oxygen, inventory, orders, cooking, line, fish, ...)
  Core/                      Game mode, game state, player state/controller, game instance, settings, input
  Data/                      Content definitions (USTRUCTs) and the data registry
  Diving/  Speargun/  FishAI/  Inventory/  Equipment/  Interaction/
  Boat/  Restaurant/  Orders/  Customers/  Roles/  DayNight/  Progression/  SaveSystem/  Loot/
  World/                     Terrain generation, region builder, ocean, underwater view, placeholder visuals
  UI/                        Canvas HUD, panel view-model, Slate main menu
  Tests/                     Rule tests shared by UE automation and the native runner
Tools/
  NativeCheck/               Native rule tests, engine API stubs, UHT stand-ins, module type-check, shadow lint
  Editor/bootstrap_content.py  Creates materials (and optionally DataTables) inside the editor
  validate_data.py           Cross-checks the JSON content against the C++ definitions
Docs/                        Design, architecture, content pipeline, development plan
```

## Verification

None of the tools below need Unreal Engine, only `clang++` (or `g++` for the rule tests) and Python 3.

| Command | What it proves |
|---|---|
| `Tools/NativeCheck/run_rules_tests.sh` | The gameplay rules behave as designed: 35 test cases, about 3,100 checks covering oxygen and blackout, bag limits, order matching, payouts, all seven minigames, role swaps and reconnects, day phases, the economy, line physics, fish decisions, terrain generation, a golden world fingerprint that must match across compilers, mesh winding and the event rolls. The same cases run inside the editor as the `Spearfish.Rules.All` automation test. |
| `python3 Tools/validate_data.py` | Every JSON field and enum matches the C++ structs, and every cross-reference resolves (species in recipes, regions, events, ...). |
| `Tools/NativeCheck/run_module_check.sh` | Every `.cpp` and every header compiles with `clang -fsyntax-only -Wall -Wshadow -Werror`. It checks against declaration-level stubs of the UE 5.5 APIs the module uses, and against generated-header stand-ins that also enforce UnrealHeaderTool rules. Two lints run as well. One reports locals that hide engine base-class members (C4458 is an error in UE5). The other rejects statements with several random draws, whose order differs between compilers and would build different worlds per machine. |

The module check verifies that the code is consistent with itself and with the engine API **as the stubs
describe it**. Where a stub disagrees with the real engine, only a real build will tell. Behavior beyond
the rule layer (physics feel, networking under latency, visuals) needs play-testing in the editor.

## Documentation

- [Docs/GAME_DESIGN.md](Docs/GAME_DESIGN.md): the game. Loop, roles and the nightly swap, systems, progression, design decisions.
- [Docs/ARCHITECTURE.md](Docs/ARCHITECTURE.md): how the code is organised, networking model, the asset-free bootstrap, procedural world.
- [Docs/CONTENT_PIPELINE.md](Docs/CONTENT_PIPELINE.md): adding fish, gear, recipes, regions and events; JSON vs DataTables; art hooks.
- [Docs/DEVELOPMENT_PLAN.md](Docs/DEVELOPMENT_PLAN.md): milestones, verification status, known risks and next steps.

## Asset hygiene

Unreal binaries (`.uasset`, `.umap`, ...) and source art and audio are tracked with **Git LFS** (see
`.gitattributes`). Build output, `Saved/`, `Intermediate/`, IDE files, local config overrides
(`Config/*.local.ini`) and secret-like files are gitignored. Never commit credentials. The game needs
none: online play uses the Null subsystem.
