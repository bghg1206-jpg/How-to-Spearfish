# Development plan

## Where the project stands

The **first playable vertical slice** is implemented in code: every system in the
[game design](GAME_DESIGN.md) exists end to end, from diving to cooking to the nightly swap, in solo and
co-op. It runs without authored assets.

What has **not** happened yet is the step that matters most next: compiling it with UnrealBuildTool and
playing it in the editor. The environment the slice was written in has no engine, so verification so far
is native. Be precise about what that covers:

| Level | Covered by | Status |
|---|---|---|
| Gameplay rules behave as designed | `run_rules_tests.sh`: 35 cases, about 3,100 checks | Verified (clang and gcc) |
| Static world identical on every machine | `Terrain.GoldenLayout` fingerprint, `lint_random.py` | Verified for clang and gcc at -O0/-O2. MSVC is checked by the automation test on the first Windows build |
| Content is well-formed and consistent | `validate_data.py` | Verified |
| Code is internally consistent (types, signatures, includes, const-correctness) | `run_module_check.sh` (every source and header, `-Wall -Wshadow -Werror`) | Verified against the stubs |
| Engine API usage | Same check, against hand-written UE 5.5 declaration stubs | Plausible: only as correct as the stubs |
| UnrealHeaderTool rules | `gen_uht.py` (subset of UHT rules) | Partially verified |
| Shadowing rules UE5 enforces (C4458) | `lint_shadow.py` | Verified for known engine members |
| Runtime behavior (physics feel, AI, replication under latency, visuals, UI layout) | Nothing yet | **Needs editor play-testing** |

## Milestones

### M0: vertical slice in code (done)
- [x] Project foundation:
  - `.uproject` and config;
  - asset-free bootstrap;
  - Git LFS and ignore rules.
- [x] Rules layer and native tests, shared with UE automation.
- [x] Data definitions, JSON content (20 fish, 31 gear, 17 recipes, 5 guests, 3 regions, 10 finds,
  8 upgrades, 6 events) and the validator.
- [x] Diving:
  - swimming, oxygen and blackout;
  - bag, gear stats and visible gear;
  - interaction.
- [x] Speargun and physical line: drag, snapping, wrapping; fish, partner, props and grapple.
- [x] Fish AI: archetypes, schools, perception, fleeing, predators, fights, legendary fish, ambient schools.
- [x] World:
  - deterministic terrain, scenery, wreck, caves, island and dock;
  - ocean surface, underwater view, sky and lighting;
  - daily loot, clams and crates.
- [x] Boat:
  - helm and anchor;
  - stations, bunks and the kitchen.
- [x] Restaurant:
  - guests, orders, 7 minigames;
  - serving, auto-chef, rumors.
- [x] Framework:
  - game mode day and night flow, roles, saves, travel;
  - controller, HUD panels, main menu, session flow.
- [x] Native module check, UHT rule checks, shadow lint.

### M1: first real build and play-test (next)
1. Open the project in UE 5.5 and fix compile errors. Start with the low-confidence API assumptions below.
2. Run the `Spearfish.Rules.All` automation test in the editor. It must pass like the native run.
3. Solo play-test of one full day: dive, catch, deposit, auto-chef serving, bunk, summary, next day.
   Then check:
   - air budget and blackout feel;
   - line feel (drag, snap);
   - fish spook distances;
   - minigame difficulty;
   - day length.
4. Co-op play-test (PIE listen server with 2 players, then two machines):
   - role banner and the nightly swap;
   - partner disconnect and rejoin;
   - tethering the partner;
   - radio and voice;
   - joining mid-day.
5. Run `Tools/Editor/bootstrap_content.py` and judge the ocean and underwater look. Tune the region
   palettes.
6. Turn every finding into either a fix or a native test, so it stays fixed.

### M2: look and feel
- Real fish meshes with simple swim animation (vertex-offset material). Use the `Visual.Mesh` hook.
- Gear meshes per slot (the equipment `Mesh` hook is reserved), diver arms and speargun viewmodel.
- Ocean:
  - Water plugin or a custom gerstner surface;
  - caustics in the underwater post-process;
  - god rays from the sun light shafts;
  - better marine snow.
- Audio:
  - underwater ambience;
  - breathing that tracks the air level;
  - reel clicks, the line under tension, gun shots;
  - the kitchen, guests and radio squelch.
- Billboard world labels (station names, guest moods) toward the local camera.
- UMG front end that reuses `SpearfishUI::BuildRows`, with controller-friendly focus.

### M3: content and balance
- More species per region (target 40 to 50), more recipes (target 40), regional guests.
- Tuning passes with telemetry from play sessions, guarded by the rule tests.
- Achievements and journal rewards (completion bonuses).

### M4: online and platform
- Steam/EOS sessions and invites instead of direct IP. Only `USpearfishGameInstance` changes.
- Host migration policy, or save-and-reload when the host leaves.
- Consider server-side validation of minigame scores if public matchmaking is ever added. Today scores
  are client-authoritative, which is acceptable between friends.

### M5: production polish
- Settings menu (graphics, audio, input remapping), accessibility (subtitles for radio, colour-blind
  palettes for the minigames, hold-to-press options).
- Performance budget pass: fish counts, instancing, Lumen and VSM settings for mid-range GPUs.
- Packaging and CI: build with UBT, run automation tests, and run the native checks on every push.

## Low-confidence API assumptions (check first on the first build)

These calls match the stubs, but they are the ones most likely to differ in the real UE 5.5 headers:

- `AActor::SetNetUpdateFrequency`, `SetMinNetUpdateFrequency` and `SetNetCullDistanceSquared` (the 5.5
  accessor style).
- `UEnhancedInputComponent::BindAction` with an extra payload argument (quick-comm keys 1 to 6).
- `UExponentialHeightFogComponent::SetFogInscatteringColor` (the property was renamed to luminance in 5.1).
- `UDirectionalLightComponent::SetAtmosphereSunLight` / `SetAtmosphereSunLightIndex`, and
  `ULightComponent::SetEnableLightShaftBloom`.
- `USkyLightComponent::bRealTimeCapture` set directly, and `SourceType = SLS_CapturedScene`.
- `UCharacterMovementComponent` overrides `CanAttemptJump`, `UpdateCharacterStateBeforeMovement` and
  `GetPredictionData_Client` with `ClientPredictionData`, plus the compressed-flags sprint.
- `FTimerDelegate::BindWeakLambda` in the night transition; `FAutoConsoleCommandWithWorld` for
  `Spearfish.ReloadData`.
- `UGameplayStatics::OpenLevel` options and the `LocalMapOptions=?game=Menu` boot path.
- `EAutomationTestFlags` combination in the automation test (an enum class in 5.5).

## Determinism risk to watch

The static world is generated locally on each machine, so it must come out identical everywhere.
Random-draw ordering is handled (see ARCHITECTURE.md). The remaining risk is floating-point code generation:
UE builds MSVC with fast floating point. Sub-millimetre height differences are harmless, but a value that
lands exactly on a placement threshold (for example a wreck site's relief check) could flip a decision on
one platform.

`Terrain.GoldenLayout` running inside the editor on Windows is the tripwire. If it ever fails there,
quantize the inputs to those decisions, or replicate the POI list from the server instead of regenerating
it.

## Known limitations of the slice

- All art is placeholder geometry. Without the bootstrap materials the ocean surface is opaque.
- World text (station labels, guest moods) faces a fixed direction rather than the camera.
- There is no audio.
- Saving restarts the current day from the morning (money, gear, journal and cooler are kept).
- There is no dedicated-server support. Listen server only, 2 players maximum.
- Minigame scores are trusted from the cook's client.
- Equipment `Mesh` references are not used yet.

## Working agreements

- Rules first: new gameplay logic goes into `Rules/` with native tests where it can.
- Content is data: no hard-coded species, items or recipes in C++.
- Server-authoritative state. Clients get presentation, prediction and input.
- Before pushing, run:
  - `Tools/NativeCheck/run_rules_tests.sh`
  - `python3 Tools/validate_data.py`
  - `Tools/NativeCheck/run_module_check.sh`
- Binary assets go through Git LFS. Never commit `Saved/`, `Intermediate/`, credentials or local config.
