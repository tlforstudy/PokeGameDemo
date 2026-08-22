import unreal


MENU_MAP = "/Game/NetBattle/BattleMenuMap"
LEGACY_MENU_MAP = "/Game/NetBattle/NetBattleMenuMap"
MENU_GAME_MODE = "/Script/MyPokemonDemo.NetBattleMenuGameMode"


def create_menu_map():
    if unreal.EditorAssetLibrary.does_asset_exist(LEGACY_MENU_MAP):
        unreal.EditorAssetLibrary.delete_asset(LEGACY_MENU_MAP)

    if unreal.EditorAssetLibrary.does_asset_exist(MENU_MAP):
        unreal.EditorAssetLibrary.delete_asset(MENU_MAP)

    if not unreal.EditorLevelLibrary.new_level(MENU_MAP):
        raise RuntimeError("Unable to create NetBattleMenuMap")

    world = unreal.EditorLevelLibrary.get_editor_world()
    world_settings = world.get_world_settings()
    menu_game_mode = unreal.load_class(None, MENU_GAME_MODE)
    if not menu_game_mode:
        raise RuntimeError("NetBattleMenuGameMode class is unavailable; compile the project first")

    world_settings.set_editor_property("default_game_mode", menu_game_mode)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log("Created /Game/NetBattle/BattleMenuMap")


create_menu_map()
