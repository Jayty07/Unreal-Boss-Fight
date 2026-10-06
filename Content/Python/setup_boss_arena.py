"""
Generates the BossArena starter content. Safe to re-run (existing assets are kept unless force=True).

    Tools > Execute Python Script... > Content/Python/setup_boss_arena.py
    or in the Python console:  import setup_boss_arena; setup_boss_arena.run(force=False)

Creates:
  /Game/BossArena/Materials/M_Telegraph      translucent unlit material (Color, Opacity params)
  /Game/BossArena/Data/DT_BossAoE            boss AoE data table imported from BossAoE.csv
  /Game/BossArena/Blueprints/BP_*            Blueprint children of the C++ classes
  /Game/BossArena/Animation/DA_WarriorAnimSet  empty montage set to fill with your animations
  /Game/Maps/BossArena                       circular arena map (blockout, lights, 8 player starts)
"""
import math
import os

import unreal

ROOT = "/Game/BossArena"
MAP_PATH = "/Game/Maps/BossArena"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def _native(name):
    return unreal.load_class(None, "/Script/BossArena." + name)


def _create(name, folder, asset_class, factory, force):
    path = "{}/{}".format(folder, name)
    if eal.does_asset_exist(path):
        if not force:
            unreal.log("BossArena: keeping existing " + path)
            return eal.load_asset(path), False
        eal.delete_asset(path)
    return asset_tools.create_asset(name, folder, asset_class, factory), True


def make_telegraph_material(force=False):
    mat, created = _create("M_Telegraph", ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew(), force)
    if not created:
        return mat
    mel = unreal.MaterialEditingLibrary
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)

    color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.25, 0.05, 1.0))

    boost = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, 150)
    boost.set_editor_property("r", 2.5)

    emissive = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 50)
    mel.connect_material_expressions(color, "", emissive, "A")
    mel.connect_material_expressions(boost, "", emissive, "B")

    opacity = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 300)
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 0.35)

    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


def make_data_table(force=False):
    csv_path = os.path.join(unreal.Paths.project_content_dir(), "BossArena", "Data", "BossAoE.csv")
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.BossAoEShapeRow.static_struct())
    table, created = _create("DT_BossAoE", ROOT + "/Data", unreal.DataTable, factory, force)
    if table and (created or force):
        if unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table, csv_path):
            unreal.log("BossArena: DT_BossAoE filled from " + csv_path)
        else:
            unreal.log_warning("BossArena: failed to import " + csv_path)
        eal.save_loaded_asset(table)
    return table


def make_blueprint(name, parent, force=False):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    bp, _ = _create(name, ROOT + "/Blueprints", unreal.Blueprint, factory, force)
    if bp:
        eal.save_loaded_asset(bp)
    return bp


def _bp_class(bp):
    return unreal.EditorAssetLibrary.load_blueprint_class(bp.get_path_name().split(".")[0]) if bp else None


def make_blueprints(force=False):
    bp_warrior = make_blueprint("BP_Warrior", _native("WarriorCharacter"), force)
    bp_boss = make_blueprint("BP_Boss", _native("BossCharacter"), force)
    make_blueprint("BP_BossAdd", _native("BossAddCharacter"), force)
    bp_gm = make_blueprint("BP_BossArenaGameMode", _native("BossArenaGameMode"), force)

    gm_class = _bp_class(bp_gm)
    if gm_class:
        cdo = unreal.get_default_object(gm_class)
        cdo.set_editor_property("default_pawn_class", _bp_class(bp_warrior))
        cdo.set_editor_property("boss_class", _bp_class(bp_boss))
        eal.save_loaded_asset(bp_gm)

    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", _native("WarriorAnimSet"))
    anim_set, _ = _create("DA_WarriorAnimSet", ROOT + "/Animation", None, factory, force)
    if anim_set:
        eal.save_loaded_asset(anim_set)
    return gm_class


def _spawn(cls, location, rotation=unreal.Rotator(0, 0, 0)):
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return subsystem.spawn_actor_from_class(cls, location, rotation)


def make_map(game_mode_class=None, force=False):
    if eal.does_asset_exist(MAP_PATH) and not force:
        unreal.log("BossArena: keeping existing " + MAP_PATH)
        return
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if eal.does_asset_exist(MAP_PATH):
        eal.delete_asset(MAP_PATH)
    level_subsystem.new_level(MAP_PATH)

    blockout = _spawn(_native("BossArenaBlockout"), unreal.Vector(0, 0, 0))
    blockout.set_actor_label("BossArenaBlockout")

    # Player starts mirror ABossArenaBlockout::GetPlayerSpawnTransform (south arc, facing the boss).
    count = blockout.get_editor_property("num_player_spawns")
    radius = blockout.get_editor_property("player_spawn_radius")
    arc = blockout.get_editor_property("player_spawn_arc")
    for i in range(count):
        alpha = i / float(count - 1) if count > 1 else 0.5
        angle = 180.0 + (-arc * 0.5 + arc * alpha)
        rad = math.radians(angle)
        loc = unreal.Vector(math.cos(rad) * radius, math.sin(rad) * radius, 110.0)
        start = _spawn(unreal.PlayerStart, loc, unreal.Rotator(0.0, 0.0, angle + 180.0))
        start.set_actor_label("PlayerStart_{}".format(i))

    sun = _spawn(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(0.0, -50.0, 35.0))
    sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sun_comp.set_editor_property("intensity", 8.0)
    sun_comp.set_editor_property("atmosphere_sun_light", True)
    sun_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)

    sky = _spawn(unreal.SkyLight, unreal.Vector(0, 0, 1500))
    sky_comp = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_comp.set_editor_property("real_time_capture", True)
    sky_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)

    _spawn(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    fog = _spawn(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -200))
    fog.get_component_by_class(unreal.ExponentialHeightFogComponent).set_editor_property("fog_density", 0.01)

    ppv = _spawn(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_editor_property("unbound", True)

    if game_mode_class:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property("default_game_mode", game_mode_class)

    level_subsystem.save_current_level()
    unreal.log("BossArena: created " + MAP_PATH)


def run(force=False):
    with unreal.ScopedSlowTask(4, "Generating BossArena starter content") as task:
        task.make_dialog(True)
        task.enter_progress_frame(1, "Telegraph material")
        make_telegraph_material(force)
        task.enter_progress_frame(1, "AoE data table")
        make_data_table(force)
        task.enter_progress_frame(1, "Blueprints")
        gm_class = make_blueprints(force)
        task.enter_progress_frame(1, "Arena map")
        make_map(gm_class, force)
    unreal.log("BossArena: setup complete. Open /Game/Maps/BossArena, set Number of Players = 2, Net Mode = Play As Listen Server, press Play.")


if __name__ == "__main__":
    run()
