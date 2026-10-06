"""Runs automatically when the editor starts (PythonScriptPlugin). Generates starter content once."""
import unreal

_handle = None


def _first_tick(delta_seconds):
    global _handle
    unreal.unregister_slate_post_tick_callback(_handle)
    if unreal.EditorAssetLibrary.does_asset_exist("/Game/Maps/BossArena"):
        return
    unreal.log("BossArena: starter content missing - running setup_boss_arena.run()")
    import setup_boss_arena
    setup_boss_arena.run()


_handle = unreal.register_slate_post_tick_callback(_first_tick)
