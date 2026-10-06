# Content pipeline

All game content is data. The C++ definitions live in `Source/HowToSpearfish/Data/SpearfishDefinitions.h`
(plus the terrain parameters in `World/SpearfishTerrain.h`). The content itself lives in
`Content/Data/Source/*.json`.

| File | Struct | Rows today |
|---|---|---|
| `Fish.json` | `FSpearfishFishSpeciesDef` | 20 species |
| `Equipment.json` | `FSpearfishEquipmentDef` | 31 items in 8 slots |
| `Recipes.json` | `FSpearfishRecipeDef` | 17 dishes |
| `Customers.json` | `FSpearfishCustomerDef` | 5 guest types |
| `Regions.json` | `FSpearfishRegionDef` | 3 regions |
| `Loot.json` | `FSpearfishLootDef` | 10 finds |
| `Upgrades.json` | `FSpearfishRestaurantUpgradeDef` | 8 upgrades |
| `Events.json` | `FSpearfishEventDef` | 6 events |

Each file is a JSON array in Unreal's DataTable layout: one object per row, and `Name` is the row ID.
Field names are the C++ property names, and enums are written by name (`"Archetype": "Schooler"`).
`FText` fields are plain strings.

## Workflow

1. Edit or add rows in the JSON.
2. Run `python3 Tools/validate_data.py`. It parses the USTRUCT/UENUM definitions straight from the headers and reports:
   - unknown fields (typos that `FJsonObjectConverter` would silently ignore);
   - bad enum values and wrong value types;
   - missing references: species in recipes, region spawns and events; loot in regions and events;
     prerequisites; events listed by regions; biome names.
3. In a running editor or PIE session, enter `Spearfish.ReloadData` in the console. It reloads the
   registry without restarting. Fish and orders created before the reload keep their old values.
4. Commit the JSON. It is text, so diffs and merges work.

The registry logs `Data: N rows from X.json` for each file and reports every row it could not convert.

## Adding things

**A fish**
- Add a row to `Fish.json` with `Archetype`, `Biomes`, depth range, size range, `Behavior` and `Visual`
  (body plan and colours drive the procedural body).
- Add it to a region's `Spawns` with a group count.
- Optionally use it in a recipe.
- The journal and shop pick it up automatically.
- Legendary fish are usually left out of `Spawns` and brought in by an event instead.

**Gear**
- Add a row to `Equipment.json` with `Slot`, `Tier`, `Unlock` (price, reputation, day, species caught,
  prerequisite item) and the `Stats` the slot uses.
- Its `Color` tints the visible gear on the diver.
- Starter items (`bStarter`) are owned from day one and never sold.

**A recipe**
- Add a row to `Recipes.json`.
- Each ingredient is either a species or a category (`ReefFish`, ...), with a count and an optional
  minimum length and quality.
- `Steps` is the station chain; every step must be one of Fillet, Chop, Season, Grill, Fry, Simmer, Plate.
- `MinReputation` and `RequiredUpgrade` gate it, and `Tags` match guest favourites.

**A region**
- Add a row to `Regions.json` with `Seed`, `Terrain` params (island size, depth profile, reef density,
  caves, wreck), a colour `Palette` for water and fog, `DepthZones`, `Spawns`, a weighted `Loot` table and
  the `Events` it can roll.
- Gate it through `Unlock`.
- Generation is deterministic: the same seed always produces the same seabed. Change `Seed` to explore
  layouts. `Terrain.Playable` in the native tests checks that a layout is playable: a dry island, a dock,
  a mooring in diveable water, the wreck depth band and no impassable steps.

**An event**
- Add a row to `Events.json` with a `Type`:
  - `LegendaryVisitor` or `SpeciesBloom` (uses `SpeciesId`, `Count`);
  - `TreasureCache` (`LootId`, `Count`);
  - `BaitBall` (bait `SpeciesId`; a region predator joins);
  - `CriticVisit`.
- Set `Chance`, `MinDay` and the guest `Rumor` text, and list it in a region's `Events`.

**A guest type**
- Add a row to `Customers.json` with patience, tip multiplier, quality expectation, reputation weight,
  spawn weight, a minimum reputation, favourite tags, dishes per visit, and whether they share rumors.

## DataTables (optional)

JSON is the source of truth for the slice. Teams that prefer editing in the editor can convert it:
1. Run `Tools/Editor/bootstrap_content.py --datatables` in the editor. It creates `/Game/Data/DT_*`.
2. Assign the tables under *Project Settings > Game > How to Spearfish > Data*.

When a table is assigned, the registry reads the table instead of the JSON file (per file, so you can
convert gradually). Re-export to JSON before committing balance changes if the JSON stays authoritative.
Pick one source of truth per file.

## Art hooks

Every placeholder visual has an art hook:

| Content | Hook |
|---|---|
| Fish | `Visual.Mesh` (soft reference). When set, the fish body uses the mesh instead of procedural parts. Colours still tint it. |
| Loot | `Mesh` soft reference. When set, it replaces the placeholder parts. |
| Equipment | `Mesh` is reserved. The diver body still builds gear from parts tinted by `Color`. Mesh support per slot is on the plan. |
| Placeholder material | `M_Spearfish_Base` (parameters `Color`, `Emissive`), or any material with those parameters set in the settings. |
| Ocean | `M_Spearfish_OceanSurface` and `M_Spearfish_OceanUnderside` (parameter `Tint`). The region palette supplies the tint. |
| Underwater look | `M_Spearfish_UnderwaterPP`, blended by depth together with colour grading, fog and marine snow driven by the region palette. |

Binary assets go through **Git LFS**. `.gitattributes` already covers `.uasset`, `.umap`, source art,
textures and audio. Keep files under `Content/` and never commit `Saved/` or `Intermediate/`.

## Balancing notes

- **Oxygen:** the `Oxygen` struct in the settings. Base consumption, a depth factor per metre, exertion,
  the over-depth penalty, the sting multiplier and breath-hold time.
- **Service:** the `Service` struct. The quality-to-price curve, patience weight, maximum tip, and
  reputation per serve or walkout.
- **Day length:** `DaySchedule.RealSecondsPerGameHour` (75 s) and `NightTimeScale`.
- **Economy:** `StartingMoney`, `RescueFee`, `PassOutFee`, `OvernightSpoilage`, `BaseSeats`,
  `MaxActiveDishes`, guest arrival interval, and fish respawn time.
- **Rule tests:** `Tools/NativeCheck/run_rules_tests.sh` pins the intended feel. For example, the starter
  tank allows a few minutes at reef depth, and perfect minigame play scores high while lazy play scores
  low. After retuning, update the expectations deliberately.
