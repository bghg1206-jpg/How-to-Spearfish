# Architecture

One runtime module, `HowToSpearfish` (C++20, UE 5.5 build settings, `IncludeOrderVersion.Latest`).
The code is layered so that the most important logic can be tested without the engine:

```
Rules/            pure gameplay rules: CoreMinimal only, no UObjects, no world   <- native + UE automation tests
Data/             USTRUCT content definitions + the registry that loads them (JSON or DataTables)
World/Terrain     deterministic terrain/layout generation (also engine-light, tested natively)
Components        server-authoritative gameplay state on actors (oxygen, bag, speargun, restaurant, ...)
Actors            character, boat, stations, fish, harpoon, customers, loot, region builder, sky, ocean
Framework         game mode / state, player controller / state, game instance, subsystems
UI                Canvas HUD, panel view-model, Slate main menu
```

The rule namespaces (`SpearfishOxygen`, `SpearfishInventory`, `SpearfishOrders`, `SpearfishMinigame`,
`SpearfishCooking`, `SpearfishRoles`, `SpearfishDay`, `SpearfishEconomy`, `SpearfishLine`, `SpearfishFish`,
`SpearfishEventRules`) take plain structs and return results. Components own the state and call the rules,
so every important decision ("does this fish fit?", "does the line snap?", "who dives tomorrow?") is one
unit-tested function.

## Folder map

| Folder | Key types |
|---|---|
| `Core/` | `ASpearfishGameMode` (session director), `ASpearfishGameState` (shared crew state, notices, radio), `ASpearfishPlayerState` (role, seat, bed), `ASpearfishPlayerController` (input contexts, panels, minigames, RPCs), `USpearfishGameInstance` (solo/host/join/menu), `ASpearfishMenuGameMode`, `USpearfishSettings`, `USpearfishInputConfig` |
| `Data/` | `SpearfishDefinitions.h` (all content structs), `USpearfishDataRegistry` (game-instance subsystem) |
| `Rules/` | engine-free rules (see above) |
| `Roles/` | `USpearfishRoleSubsystem`: seats, join/leave, nightly swap, auto-chef decision |
| `DayNight/` | `USpearfishDayCycleComponent` (clock, phases), `ASpearfishSkyController` (sun, moon, atmosphere, fog) |
| `Diving/` | `ASpearfishCharacter`, `USpearfishMovementComponent` (swimming), `USpearfishOxygenComponent`, `USpearfishDiverBodyComponent` (visible gear, bubbles) |
| `Speargun/` | `USpeargunComponent` (line simulation), `ASpearfishHarpoon` |
| `FishAI/` | `ASpearfishFish`, `USpearfishFishBodyComponent`, `USpearfishFishSubsystem` (registry, schools, spawning, noise), `ASpearfishAmbientSchool` |
| `Inventory/` | dive bag, cooler, `SpearfishCatch` (all catch and reward bookkeeping in one place) |
| `Equipment/` | `USpearfishEquipmentComponent`: loadout to derived diver stats and appearance |
| `Interaction/` | `ISpearfishInteractable`, `USpearfishInteractionComponent` (focus trace, server validation) |
| `Boat/` | `ASpearfishBoat` (driving, anchor, layout, cooler/restaurant/auto-chef), `ASpearfishStation` |
| `Restaurant/`, `Orders/`, `Customers/` | `USpearfishRestaurantComponent`, `USpearfishAutoChefComponent`, order and dish types, `ASpearfishCustomer` |
| `Progression/` | money, reputation, gear, upgrades, regions; journal; `SpearfishEventDirector` |
| `SaveSystem/` | `FSpearfishCampaignData`, `USpearfishSaveSubsystem` |
| `World/` | `FSpearfishTerrain`, `ASpearfishRegionBuilder`, `USpearfishOceanSubsystem`, `ASpearfishOceanSurface`, `USpearfishUnderwaterViewComponent`, `USpearfishVisualSubsystem` |
| `Loot/` | `ASpearfishLootPickup`, `ASpearfishGiantClam`, `ASpearfishPullable` |
| `UI/` | `ASpearfishHUD`, `SpearfishUI` view-model, `SSpearfishMainMenu` |

## Asset-free bootstrap

The slice must run from a fresh clone with no `.uasset` files.

**Maps and game modes**
- Every session runs on the engine's empty `/Engine/Maps/Entry`.
- The URL's game mode alias picks the mode: `?game=Spearfish` (gameplay) or `?game=Menu`.
- `GlobalDefaultGameMode` is the gameplay mode, so Play-In-Editor starts a session in any net mode. A
  standalone PIE runs solo; a listen-server PIE runs co-op.
- Packaged builds boot with `LocalMapOptions=?game=Menu`. If a session URL ever arrives without
  `?mode=` (a plain boot, or the engine's fallback after a disconnect), `ASpearfishGameMode` hands
  over to the menu itself.

**World:** `ASpearfishRegionBuilder` generates the seabed, scenery, wreck, caves and island from data (see
below).

**Input:** `USpearfishInputConfig` creates the Enhanced Input actions and mapping contexts in code. Authored
`UInputAction` assets can replace them later without touching the bindings.

**Visuals:** `USpearfishVisualSubsystem` builds placeholder geometry from the engine basic shapes.
- It caches dynamic material instances by colour.
- It prefers `/Game/Materials/M_Spearfish_Base` and falls back to `/Engine/BasicShapes/BasicShapeMaterial`
  (parameter `Color`).
- The ocean and post-process materials are optional too; `Tools/Editor/bootstrap_content.py` creates them.
- Fish and loot definitions carry optional `TSoftObjectPtr` mesh references that replace the placeholder parts.

**Data:** `USpearfishDataRegistry` loads `Content/Data/Source/*.json` at startup through
`FJsonObjectConverter`.
- If a DataTable is assigned in the settings, it is used instead.
- The JSON folder is staged into packaged builds (`DirectoriesToAlwaysStageAsUFS`).
- `Spearfish.ReloadData` reloads it live.

**UI:** the Canvas HUD and the Slate main menu need no widget blueprints.

**Cooking:** `DefaultGame.ini` explicitly cooks the Entry map and `/Engine/BasicShapes`, because nothing
else references them.

## Networking model

The game uses a listen server with up to 2 players. The server is authoritative for all gameplay state;
clients predict only their own movement.

| Concern | Approach |
|---|---|
| Static world | Not replicated. The region builder replicates only `RegionId` and `Seed`; every machine generates the identical seabed, scenery, island and habitat locally from the same deterministic `FSpearfishTerrain`. This saves bandwidth and join time, but determinism must hold **across compilers**: random draws are always sequenced (one per statement or via `SpearfishRandom`, enforced by `lint_random.py`), and the `Terrain.GoldenLayout` test pins a quantized fingerprint of a generated world that gcc, clang and MSVC must all reproduce. |
| Dynamic world | Fish, loot, clams, crates, customers, harpoons, the boat and stations are server-spawned replicated actors. |
| Fish | Compact `FSpearfishFishNetState` (position, yaw, mind state, speed) at a low rate, with client-side smoothing. Species and size replicate once (`COND_InitialOnly`), and bodies are generated locally. Net cull distance is 90 m. |
| Diver movement | `UCharacterMovementComponent` flying mode with custom buoyancy and a surface ceiling. Sprint is predicted through a custom saved move (`FLAG_Custom_0`). External forces from the line and currents are applied as acceleration on the server and owning client. |
| Speargun | Fire, reel and release are server RPCs. The harpoon replicates its launch data once and each client simulates the flight locally. The server alone sweeps for hits. The line state (`FSpearfishLineNetState`: target, wrap points, length, tension, stress) replicates for drawing and the HUD. |
| Boat | Server-simulated. Driver input goes through an unreliable RPC, and the compact state is smoothed on clients. Characters standing on deck are based on it. |
| Customers | Replicated identity and state only. Each client animates the walk to the seat locally from the boat-relative layout. |
| Restaurant | Orders, dishes and kitchen needs are replicated arrays on the boat's component. Minigames run on the cook's client and the score is submitted by RPC; the server validates the dish, the step index and the lock owner. IDs are single-use, so a dish cannot be paid twice. |
| Messages | `ASpearfishGameState` multicasts team notices and quick radio messages. Private notices go through `ClientNotice`. |
| Interaction | The client traces for focus; `ServerInteract` revalidates the range and `CanInteract` on the server. |
| Relevancy | The two characters, the boat and the region builder are always relevant. |
| Joining | Players can join mid-session. The role subsystem gives them the free role, and they spawn on deck at their seat. |

## Session and day flow (server)

1. `InitGame`:
   - parse `?mode=solo|coop` and `?new=1`;
   - load or create the campaign in the save subsystem;
   - set `MaxPlayers`;
   - configure the role subsystem with the saved seats.
2. `InitGameState` loads progression and the journal into the replicated game state.
3. `PostLogin` registers the player's role before the pawn spawns. Pawns spawn only after the session is
   set up (`PlayerCanRestart`).
4. `StartPlay` and `SetupSession`:
   - spawn the region builder, which builds the static scene;
   - spawn the boat at the region's mooring and spawn its stations;
   - restore the cooler;
   - start the day: restaurant, fish population, daily loot, events;
   - spawn the waiting players on deck.
5. Every 0.5 s `TickSleepCheck` tests:
   - everyone in a bunk after 17:30, then a normal night;
   - 02:00, then a pass-out night.
6. Night:
   - fade out and close the restaurant;
   - apply pass-out penalties and spoilage, and build the summary;
   - swap the roles;
   - optionally sail to a new region, rebuilding the world: fish despawned, builder replaced, boat moved;
   - restock for the next day;
   - wake up: start the day clock, leave the bunks, refill air, apply the role kits, show the role banner;
   - save.

## Procedural world

`FSpearfishTerrain` (World/SpearfishTerrain.*) is deterministic and tested natively. Given the
region's `FSpearfishTerrainParams` and seed it produces:

- a height field with:
  - an island with a beach;
  - a depth profile from the shallows to the drop-off and the blue trench;
  - reef ridges and noise detail;
- points of interest:
  - the dock and the boat mooring;
  - a wreck on a flat band at about 15 m;
  - caves, clam beds and reef heads;
- habitat samples labelled by biome (Reef, Sand, Seagrass, Kelp, Wall, Cave, Wreck, OpenWater) and depth,
  used to spawn fish and loot.

`ASpearfishRegionBuilder` turns this into:
- a 4-section `UProceduralMeshComponent` seabed with collision;
- hierarchical-instanced scenery (rocks with collision, coral that blocks only the line, seagrass, kelp);
- the wreck, caves and island built from parts.

**Triangle winding.** Unreal renders triangle (A, B, C) front-facing toward `(C − A) × (B − A)`, the
opposite of `(B − A) × (C − A)`. Two engine sources agree:
- `UKismetProceduralMeshLibrary::GenerateBoxMesh` builds its +Z face from (−x,+y), (+x,+y), (+x,−y),
  (−x,−y) via `ConvertQuadToTris` (0,1,3 / 1,2,3);
- `CalculateTangentsForMesh` computes the face normal as `(P1 − P2) ^ (P0 − P2)`.

`SpearfishMeshWinding` (in `World/SpearfishTerrain.h`) encodes the rule, and the native test
`Terrain.Winding` checks it against both references. The builder then orients every seabed triangle
toward +Z regardless of the order the grid was walked in. (An earlier draft had this inverted, which would
have back-face culled the seabed when seen from above. If a real build ever shows the seabed inside-out,
the rule and its test are the single place to change.)

## Saving

`USpearfishSaveSubsystem` (game-instance subsystem) stores one `USpearfishSaveGame` per mode
(`Spearfish_Solo_0`, `Spearfish_Coop_0`). It holds a versioned `FSpearfishCampaignData`:
- day, money, reputation;
- owned gear and loadout, upgrades, regions;
- the journal and loot records;
- the cooler contents;
- role seats and the next item ID.

The host saves every morning, when the campaign is created, and on quit. PIE uses a fresh campaign and
never writes the slot (configurable).

## Settings

`USpearfishSettings` (*Project Settings > Game > How to Spearfish*, stored in `DefaultGame.ini`) holds:
- campaign constants and fees;
- all rule tuning structs (day schedule, oxygen, service);
- data and material references;
- PIE behaviour.

## Verification tooling

| Tool | Notes |
|---|---|
| `Tools/NativeCheck/run_rules_tests.sh` | Compiles `Rules/*.cpp`, the terrain and the shared test cases against `stub/CoreMinimal.h`, which provides real containers, case-insensitive `FName`, `FMath` and the engine's `FRandomStream` algorithm. It runs the tests. The same cases are registered as the UE automation test `Spearfish.Rules.All`. |
| `Tools/NativeCheck/gen_uht.py` | Writes stand-ins for `*.generated.h` (the `GENERATED_BODY` contents, RPC `_Implementation`/`_Validate` declarations) and enforces UHT rules: generated header last, unique header names, `ReplicatedUsing` targets are UFUNCTIONs, every replicated property has `DOREPLIFETIME`, RPC shape, no nested containers or unsupported property types. |
| `Tools/NativeCheck/stub/UEStub/*.h` | Declaration-level mirrors of the UE 5.5 APIs the module uses: UObject, Engine, Enhanced Input, Slate, JSON, procedural mesh. Signatures follow the engine headers so that call-site mistakes surface. They are not a substitute for the engine: if a stub signature is wrong, the check can be wrong. |
| `Tools/NativeCheck/run_module_check.sh` | `clang -fsyntax-only` on every source, and on every header alone (self-containment), with UBT-like warnings as errors. It then runs `lint_shadow.py`, which uses clang's AST to find locals and parameters hiding engine base-class members, an error in UE5 builds that clang alone does not report. |
| `Tools/NativeCheck/lint_random.py` | Rejects statements with more than one random draw. Argument evaluation order is unspecified in C++, so such statements produce different worlds with different compilers. This was a real bug found with gcc vs clang. |
| `Tools/validate_data.py` | Parses the USTRUCT/UENUM definitions from the headers and validates every JSON file: field names, enum values, types and cross-references. |

## Extension points

- **Art:** fish species carry `FSpearfishFishVisual` (with an optional mesh), loot carries an optional
  mesh, and equipment reserves one.
  The visual subsystem is the single place placeholder geometry comes from.
- **Content:** add rows to the JSON, or move to DataTables (see CONTENT_PIPELINE.md).
- **Online:** replace direct IP in `USpearfishGameInstance` with an online-subsystem session (Steam/EOS)
  without touching gameplay code.
- **UI:** the HUD only draws what `SpearfishUI::BuildRows` and the controller expose. A UMG front end can
  reuse the same view-model.
