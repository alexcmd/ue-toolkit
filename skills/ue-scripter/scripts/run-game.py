import unreal
subsys = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsys.editor_request_begin_play()
print("PIE started")
