import unreal
subsys = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsys.editor_request_end_play()
print("Game stopped")
