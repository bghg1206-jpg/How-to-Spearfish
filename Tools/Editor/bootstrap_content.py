"""
Creates the project's starter content in the Unreal Editor (Tools > Execute Python Script, or
`UnrealEditor-Cmd HowToSpearfish.uproject -run=pythonscript -script=Tools/Editor/bootstrap_content.py`).

The game runs without any of these assets (placeholder materials fall back to the engine's
BasicShapeMaterial and an opaque ocean), so this script only upgrades the look:

  /Game/Materials/M_Spearfish_Base             opaque lit material, parameters Color (vector) + Emissive (scalar)
  /Game/Materials/M_Spearfish_OceanSurface     translucent sea surface seen from above, parameter Tint
  /Game/Materials/M_Spearfish_OceanUnderside   bright rippling surface seen from below, parameter Tint
  /Game/Materials/M_Spearfish_UnderwaterPP     post-process wobble + tint blended in underwater
  /Game/Data/DT_*  (optional, --datatables)    DataTables imported from Content/Data/Source/*.json

Paths match the defaults in USpearfishSettings. The script is idempotent: existing assets are kept unless
--force is passed. Written against the UE 5.5 Python API; it has not been run in this repository's CI
(there is no editor there), so check the Output Log the first time you run it.
"""
import os
import sys

import unreal

MATERIAL_DIR = '/Game/Materials'
DATA_DIR = '/Game/Data'
FORCE = '--force' in sys.argv
WITH_DATATABLES = '--datatables' in sys.argv

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary


def log(message):
    unreal.log(f'[Spearfish bootstrap] {message}')


def new_material(name):
    path = f'{MATERIAL_DIR}/{name}'
    if EAL.does_asset_exist(path):
        if not FORCE:
            log(f'{path} exists, skipping (use --force to rebuild)')
            return None
        EAL.delete_asset(path)
    return ASSET_TOOLS.create_asset(name, MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew())


def expr(material, cls, x, y):
    return MEL.create_material_expression(material, cls, x, y)


def vector_param(material, name, value, x, y):
    node = expr(material, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('default_value', value)
    return node


def scalar_param(material, name, value, x, y):
    node = expr(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('default_value', value)
    return node


def constant(material, value, x, y):
    node = expr(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property('r', value)
    return node


def multiply(material, a, b, x, y):
    node = expr(material, unreal.MaterialExpressionMultiply, x, y)
    MEL.connect_material_expressions(a, '', node, 'A')
    MEL.connect_material_expressions(b, '', node, 'B')
    return node


def finish(material, name):
    MEL.layout_material_expressions(material)
    MEL.recompile_material(material)
    EAL.save_asset(f'{MATERIAL_DIR}/{name}')
    log(f'created {MATERIAL_DIR}/{name}')


def build_base():
    name = 'M_Spearfish_Base'
    material = new_material(name)
    if not material:
        return
    color = vector_param(material, 'Color', unreal.LinearColor(0.8, 0.8, 0.8, 1.0), -600, 0)
    emissive = scalar_param(material, 'Emissive', 0.0, -600, 220)
    MEL.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(multiply(material, color, emissive, -300, 220), '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(constant(material, 0.65, -300, 420), '', unreal.MaterialProperty.MP_ROUGHNESS)
    finish(material, name)


def fresnel(material, exponent, x, y):
    node = expr(material, unreal.MaterialExpressionFresnel, x, y)
    node.set_editor_property('exponent', exponent)
    node.set_editor_property('base_reflect_fraction', 0.04)
    return node


def build_ocean_surface():
    name = 'M_Spearfish_OceanSurface'
    material = new_material(name)
    if not material:
        return
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE)
    tint = vector_param(material, 'Tint', unreal.LinearColor(0.05, 0.35, 0.45, 1.0), -700, 0)
    MEL.connect_material_property(tint, '', unreal.MaterialProperty.MP_BASE_COLOR)
    # Clear water straight down, more reflective (opaque) at grazing angles.
    opacity = expr(material, unreal.MaterialExpressionLinearInterpolate, -300, 240)
    MEL.connect_material_expressions(constant(material, 0.55, -500, 200), '', opacity, 'A')
    MEL.connect_material_expressions(constant(material, 0.95, -500, 260), '', opacity, 'B')
    MEL.connect_material_expressions(fresnel(material, 4.0, -500, 320), '', opacity, 'Alpha')
    MEL.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    MEL.connect_material_property(constant(material, 0.04, -300, 420), '', unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(constant(material, 1.0, -300, 480), '', unreal.MaterialProperty.MP_SPECULAR)
    finish(material, name)


def build_ocean_underside():
    name = 'M_Spearfish_OceanUnderside'
    material = new_material(name)
    if not material:
        return
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('two_sided', True)
    tint = vector_param(material, 'Tint', unreal.LinearColor(0.35, 0.75, 0.85, 1.0), -900, 0)
    # A slow shimmer: brightness ripples with world position and time (no textures needed).
    world = expr(material, unreal.MaterialExpressionWorldPosition, -1100, 220)
    mask = expr(material, unreal.MaterialExpressionComponentMask, -950, 220)
    mask.set_editor_property('r', True)
    mask.set_editor_property('g', False)
    mask.set_editor_property('b', False)
    MEL.connect_material_expressions(world, '', mask, '')
    time = expr(material, unreal.MaterialExpressionTime, -1100, 340)
    wave_input = expr(material, unreal.MaterialExpressionAdd, -800, 280)
    MEL.connect_material_expressions(multiply(material, mask, constant(material, 0.004, -950, 300), -900, 260), '', wave_input, 'A')
    MEL.connect_material_expressions(multiply(material, time, constant(material, 0.8, -950, 380), -900, 360), '', wave_input, 'B')
    sine = expr(material, unreal.MaterialExpressionSine, -650, 280)
    MEL.connect_material_expressions(wave_input, '', sine, '')
    shimmer = expr(material, unreal.MaterialExpressionAdd, -500, 280)
    MEL.connect_material_expressions(multiply(material, sine, constant(material, 0.25, -650, 360), -580, 330), '', shimmer, 'A')
    MEL.connect_material_expressions(constant(material, 1.6, -650, 420), '', shimmer, 'B')
    MEL.connect_material_property(multiply(material, tint, shimmer, -300, 120), '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(constant(material, 0.7, -300, 300), '', unreal.MaterialProperty.MP_OPACITY)
    finish(material, name)


def build_underwater_pp():
    name = 'M_Spearfish_UnderwaterPP'
    material = new_material(name)
    if not material:
        return
    material.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
    material.set_editor_property('blendable_location', unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_DOF)
    # Gentle refraction wobble: offset the screen UV by a sine of time and height.
    uv = expr(material, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
    v_mask = expr(material, unreal.MaterialExpressionComponentMask, -1050, 120)
    v_mask.set_editor_property('r', False)
    v_mask.set_editor_property('g', True)
    MEL.connect_material_expressions(uv, '', v_mask, '')
    time = expr(material, unreal.MaterialExpressionTime, -1200, 240)
    phase = expr(material, unreal.MaterialExpressionAdd, -900, 160)
    MEL.connect_material_expressions(multiply(material, v_mask, constant(material, 40.0, -1050, 200), -950, 140), '', phase, 'A')
    MEL.connect_material_expressions(multiply(material, time, constant(material, 1.3, -1050, 300), -950, 260), '', phase, 'B')
    sine = expr(material, unreal.MaterialExpressionSine, -750, 160)
    MEL.connect_material_expressions(phase, '', sine, '')
    offset = expr(material, unreal.MaterialExpressionAppendVector, -600, 160)
    MEL.connect_material_expressions(multiply(material, sine, constant(material, 0.0015, -750, 240), -680, 200), '', offset, 'A')
    MEL.connect_material_expressions(constant(material, 0.0, -750, 300), '', offset, 'B')
    shifted = expr(material, unreal.MaterialExpressionAdd, -450, 60)
    MEL.connect_material_expressions(uv, '', shifted, 'A')
    MEL.connect_material_expressions(offset, '', shifted, 'B')
    scene = expr(material, unreal.MaterialExpressionSceneTexture, -300, 60)
    scene.set_editor_property('scene_texture_id', unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    MEL.connect_material_expressions(shifted, '', scene, 'UVs')
    MEL.connect_material_property(scene, 'Color', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    finish(material, name)


DATATABLES = {
    'Fish.json': ('DT_Fish', 'SpearfishFishSpeciesDef'),
    'Equipment.json': ('DT_Equipment', 'SpearfishEquipmentDef'),
    'Recipes.json': ('DT_Recipes', 'SpearfishRecipeDef'),
    'Customers.json': ('DT_Customers', 'SpearfishCustomerDef'),
    'Regions.json': ('DT_Regions', 'SpearfishRegionDef'),
    'Loot.json': ('DT_Loot', 'SpearfishLootDef'),
    'Upgrades.json': ('DT_Upgrades', 'SpearfishRestaurantUpgradeDef'),
    'Events.json': ('DT_Events', 'SpearfishEventDef'),
}


def build_datatables():
    source_dir = os.path.join(unreal.Paths.project_content_dir(), 'Data', 'Source')
    for json_file, (asset_name, struct_name) in DATATABLES.items():
        path = f'{DATA_DIR}/{asset_name}'
        if EAL.does_asset_exist(path) and not FORCE:
            log(f'{path} exists, skipping')
            continue
        row_struct = unreal.load_object(None, f'/Script/HowToSpearfish.{struct_name}')
        if not row_struct:
            unreal.log_error(f'[Spearfish bootstrap] row struct {struct_name} not found - build the C++ module first')
            continue
        factory = unreal.DataTableFactory()
        factory.set_editor_property('struct', row_struct)
        table = EAL.load_asset(path) if EAL.does_asset_exist(path) else ASSET_TOOLS.create_asset(asset_name, DATA_DIR, unreal.DataTable, factory)
        if unreal.DataTableFunctionLibrary.fill_data_table_from_json_file(table, os.path.join(source_dir, json_file)):
            EAL.save_asset(path)
            log(f'imported {json_file} -> {path} (point Project Settings > How to Spearfish > Data at it to use it)')
        else:
            unreal.log_error(f'[Spearfish bootstrap] import of {json_file} failed')


def main():
    for build in (build_base, build_ocean_surface, build_ocean_underside, build_underwater_pp):
        try:
            build()
        except Exception as error:  # keep going: each asset is independent
            unreal.log_error(f'[Spearfish bootstrap] {build.__name__} failed: {error}')
    if WITH_DATATABLES:
        build_datatables()
    log('done')


main()
