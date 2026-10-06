#!/usr/bin/env python3
"""
Validates Content/Data/Source/*.json against the C++ definition structs and checks cross references.

- Every JSON key must be a UPROPERTY of the target USTRUCT (parsed from the headers), so typos that
  FJsonObjectConverter would silently ignore are caught.
- Enum values must exist in the UENUM.
- Ids referenced across files (species, loot, equipment prerequisites, upgrades, events) must exist.
Usage: python3 Tools/validate_data.py   (exit code 1 on problems)
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODULE = os.path.join(ROOT, 'Source', 'HowToSpearfish')
DATA = os.path.join(ROOT, 'Content', 'Data', 'Source')
HEADERS = [os.path.join(MODULE, 'Data', 'SpearfishDefinitions.h'),
           os.path.join(MODULE, 'Rules', 'SpearfishRulesTypes.h'),
           os.path.join(MODULE, 'Rules', 'FishRules.h'),
           os.path.join(MODULE, 'World', 'SpearfishTerrain.h')]

FILES = {
    'Fish.json': 'FSpearfishFishSpeciesDef',
    'Equipment.json': 'FSpearfishEquipmentDef',
    'Recipes.json': 'FSpearfishRecipeDef',
    'Customers.json': 'FSpearfishCustomerDef',
    'Regions.json': 'FSpearfishRegionDef',
    'Loot.json': 'FSpearfishLootDef',
    'Upgrades.json': 'FSpearfishRestaurantUpgradeDef',
    'Events.json': 'FSpearfishEventDef',
}
BIOMES = {'Reef', 'Sand', 'Seagrass', 'Cave', 'Wreck', 'OpenWater', 'Kelp', 'Wall'}
SCALARS = {'float', 'double', 'int32', 'int64', 'uint8', 'bool', 'FName', 'FText', 'FString'}

problems = []


def problem(message):
    problems.append(message)


def parse_headers():
    structs, enums = {}, {}
    for path in HEADERS:
        text = open(path, encoding='utf-8').read()
        for match in re.finditer(r'UENUM\([^)]*\)\s*enum\s+class\s+(\w+)\s*:\s*\w+\s*\{(.*?)\};', text, re.S):
            values = []
            for line in match.group(2).split('\n'):
                line = re.sub(r'//.*', '', line).strip()
                name = re.match(r'(\w+)', line)
                if name:
                    values.append(name.group(1))
            enums[match.group(1)] = set(values)
        for match in re.finditer(r'USTRUCT\([^)]*\)\s*struct\s+(\w+)(?:\s*:\s*public\s+\w+)?\s*\{(.*?)\n\};', text, re.S):
            fields = {}
            for prop in re.finditer(r'UPROPERTY\((?:[^()]|\([^()]*\))*\)\s*([\w<>:, ]+?)\s+(\w+)\s*(?:=[^;]*)?;', match.group(2)):
                fields[prop.group(2)] = prop.group(1).strip()
            structs[match.group(1)] = fields
    return structs, enums


def check_value(value, type_name, path, structs, enums):
    array = re.match(r'TArray<\s*(\w+)\s*>', type_name)
    if array:
        if not isinstance(value, list):
            problem(f'{path}: expected array')
            return
        for index, item in enumerate(value):
            check_value(item, array.group(1), f'{path}[{index}]', structs, enums)
        return
    if type_name.startswith('TSoftObjectPtr'):
        if not isinstance(value, str):
            problem(f'{path}: expected asset path string')
        return
    if type_name == 'FLinearColor':
        if not isinstance(value, dict) or set(value.keys()) - {'R', 'G', 'B', 'A'}:
            problem(f'{path}: expected {{R,G,B,A}} colour')
        return
    if type_name in enums:
        if value not in enums[type_name]:
            problem(f'{path}: "{value}" is not a value of {type_name} ({", ".join(sorted(enums[type_name]))})')
        return
    if type_name in structs:
        if not isinstance(value, dict):
            problem(f'{path}: expected object for {type_name}')
            return
        check_object(value, type_name, path, structs, enums)
        return
    if type_name in SCALARS:
        if type_name == 'bool' and not isinstance(value, bool):
            problem(f'{path}: expected bool')
        elif type_name in ('float', 'double', 'int32', 'int64', 'uint8') and (isinstance(value, bool) or not isinstance(value, (int, float))):
            problem(f'{path}: expected number')
        elif type_name in ('FName', 'FText', 'FString') and not isinstance(value, str):
            problem(f'{path}: expected string')
        return
    problem(f'{path}: validator does not know type {type_name}')


def check_object(obj, struct_name, path, structs, enums, top_level=False):
    fields = structs[struct_name]
    for key, value in obj.items():
        if key == 'Name' and top_level:
            continue
        if key not in fields:
            problem(f'{path}: unknown field "{key}" for {struct_name}')
            continue
        check_value(value, fields[key], f'{path}.{key}', structs, enums)


def main():
    structs, enums = parse_headers()
    data = {}
    for file, struct in FILES.items():
        rows = json.load(open(os.path.join(DATA, file), encoding='utf-8'))
        names = [row.get('Name') for row in rows]
        if len(names) != len(set(names)):
            problem(f'{file}: duplicate Name values')
        for row in rows:
            if not row.get('Name'):
                problem(f'{file}: row without Name')
            check_object(row, struct, f'{file}:{row.get("Name")}', structs, enums, top_level=True)
        data[file] = {row['Name']: row for row in rows if row.get('Name')}

    fish, equipment, recipes = data['Fish.json'], data['Equipment.json'], data['Recipes.json']
    regions, loot, upgrades, events = data['Regions.json'], data['Loot.json'], data['Upgrades.json'], data['Events.json']
    categories = {row['Category'] for row in fish.values()}

    for name, row in fish.items():
        for biome in row.get('Biomes', []):
            if biome not in BIOMES:
                problem(f'Fish {name}: unknown biome {biome}')
        if row['MinLengthCm'] >= row['MaxLengthCm']:
            problem(f'Fish {name}: MinLengthCm must be < MaxLengthCm')
        if row['MinDepthM'] > row['MaxDepthM']:
            problem(f'Fish {name}: MinDepthM must be <= MaxDepthM')
        for prey in row['Behavior'].get('PreyCategories', []):
            if prey not in categories:
                problem(f'Fish {name}: prey category {prey} matches no species')

    starters = {}
    for name, row in equipment.items():
        prereq = row['Unlock'].get('Prerequisite')
        if prereq and prereq not in equipment:
            problem(f'Equipment {name}: unknown prerequisite {prereq}')
        if row.get('bStarter'):
            starters.setdefault(row['Slot'], []).append(name)
    for slot in enums['ESpearfishEquipmentSlot'] - {'Count'}:
        if len(starters.get(slot, [])) != 1:
            problem(f'Equipment: slot {slot} needs exactly one starter item, has {starters.get(slot, [])}')

    for name, row in recipes.items():
        if not row['Ingredients']:
            problem(f'Recipe {name}: no ingredients')
        if not row['Steps']:
            problem(f'Recipe {name}: no steps')
        for ingredient in row['Ingredients']:
            species, category = ingredient.get('SpeciesId'), ingredient.get('Category')
            if species and (species not in fish or not fish[species].get('bCatchable', True)):
                problem(f'Recipe {name}: ingredient species {species} missing or not catchable')
            if category and category not in categories:
                problem(f'Recipe {name}: ingredient category {category} matches no species')
            if species and ingredient.get('MinLengthCm', 0) > fish.get(species, {}).get('MaxLengthCm', 1e9):
                problem(f'Recipe {name}: {species} can never be {ingredient["MinLengthCm"]} cm')
        if row.get('RequiredUpgrade') and row['RequiredUpgrade'] not in upgrades:
            problem(f'Recipe {name}: unknown upgrade {row["RequiredUpgrade"]}')

    if not any(row.get('bStarter') for row in regions.values()):
        problem('Regions: no starter region')
    for name, row in regions.items():
        prereq = row['Unlock'].get('Prerequisite')
        if prereq and prereq not in regions:
            problem(f'Region {name}: unknown prerequisite {prereq}')
        for spawn in row['Spawns']:
            if spawn['SpeciesId'] not in fish:
                problem(f'Region {name}: unknown species {spawn["SpeciesId"]}')
        for entry in row['Loot']:
            if entry['Id'] not in loot:
                problem(f'Region {name}: unknown loot {entry["Id"]}')
        for event in row['Events']:
            if event not in events:
                problem(f'Region {name}: unknown event {event}')
        zones = sorted(row['DepthZones'], key=lambda z: z['MinDepthM'])
        for first, second in zip(zones, zones[1:]):
            if abs(first['MaxDepthM'] - second['MinDepthM']) > 0.01:
                problem(f'Region {name}: depth zones are not contiguous ({first["DisplayName"]} -> {second["DisplayName"]})')
        # Every recipe whose fish live here should be completable in at least one region.
    for name, row in loot.items():
        for biome in row.get('Biomes', []):
            if biome not in BIOMES:
                problem(f'Loot {name}: unknown biome {biome}')
    for name, row in upgrades.items():
        prereq = row['Unlock'].get('Prerequisite')
        if prereq and prereq not in upgrades:
            problem(f'Upgrade {name}: unknown prerequisite {prereq}')
    for name, row in events.items():
        if row.get('SpeciesId') and row['SpeciesId'] not in fish:
            problem(f'Event {name}: unknown species {row["SpeciesId"]}')
        if row.get('LootId') and row['LootId'] not in loot:
            problem(f'Event {name}: unknown loot {row["LootId"]}')

    # Every recipe must be cookable from some region's spawn table (or an event species).
    spawnable = {spawn['SpeciesId'] for region in regions.values() for spawn in region['Spawns']}
    spawnable |= {row['SpeciesId'] for row in events.values() if row.get('SpeciesId')}
    for name, row in recipes.items():
        for ingredient in row['Ingredients']:
            if ingredient.get('SpeciesId') and ingredient['SpeciesId'] not in spawnable:
                problem(f'Recipe {name}: {ingredient["SpeciesId"]} never spawns anywhere')

    for message in problems:
        print('DATA:', message)
    counts = ', '.join(f'{len(rows)} {file[:-5].lower()}' for file, rows in data.items())
    print(f'validate_data: {counts}; {len(problems)} problem(s)')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
