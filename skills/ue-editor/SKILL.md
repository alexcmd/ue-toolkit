---
name: ue-editor
description: Query, inspect, modify, and automate inside a running Unreal Editor. Use when the user asks to work with actors, assets, levels, properties, or viewports in the editor. For building, use ue-builder. For launching the editor, use ue-runner. For C++ code, use ue-coder.
---

# UE Editor

Orchestrate editor automation by writing Python scripts and executing them via the AgentBridge HTTP plugin.

## IMPORTANT — How to Execute Scripts

**Primary (MCP tools):** Use `ue_health`, `ue_execute`, `ue_screenshot`, `ue_scene_tree` MCP tools.

**Shell fallback (if MCP unavailable):** Use `ue-exec.sh` — it handles port detection, JSON encoding, and error handling:
```bash
# Health check
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --health

# Execute inline script
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --script 'import unreal; print("Hello")'

# Execute from file
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --file /tmp/my_script.py
```

**DO NOT use raw `curl` commands.** The `ue-exec.sh` script auto-detects the port, encodes JSON safely, and parses responses. If you must use curl, ALL endpoints are under `/agent/` prefix (e.g., `/agent/health`, `/agent/execute`). There is NO `/health`, NO `/api/v1/...` — those are different UE APIs not related to AgentBridge.

## AgentBridge HTTP Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/agent/health` | Health check |
| POST | `/agent/execute` | Execute Python: `{"script":"..."}` |
| POST | `/agent/play` | PIE control: `{"mode":"pie\|simulate\|stop"}` |
| GET | `/agent/logs` | Log stream: `?lines=100&filter=pattern&severity=error` |
| POST | `/agent/build` | Hot reload / live coding: `{"wait":true}` |
| GET | `/agent/devices` | List target devices |
| GET | `/agent/configs` | Build configurations |
| POST | `/agent/batch` | Batch execute: `{"scripts":[...]}` |

## CRITICAL — Niagara Particle Systems (see knowledge/niagara.md)

**Python cannot configure Niagara emitter internals** (spawn rate, velocity, lifetime, forces). Only user parameter overrides work (`set_float_parameter`, `set_vector_parameter` on `NiagaraComponent`).

**Correct approach**: Search the project for existing effects (`/Game/Effects/`, `/Game/FX/`, `/Game/VFX/`), duplicate, reposition. Tell user to tweak behavior in Niagara Editor UI.

**NEVER**: instantiate `NiagaraPythonEmitter` (SIGSEGV crash), explore Niagara API via `help()`/`dir()`, or iterate >3 times on Niagara property names.

## Writing Python Scripts

- `import unreal` — always first
- `unreal.get_editor_subsystem(unreal.SubsystemClass)` — access subsystems
- Common: `UnrealEditorSubsystem`, `EditorActorSubsystem`, `EditorAssetSubsystem`, `EditorLevelSubsystem`, `LevelEditorSubsystem`
- `print()` — only way to return data through the bridge
- `set_editor_property()` / `get_editor_property()` — NOT direct attribute access
- Look up API details via `ue_api_search` / `ue_api_type` MCP tools before writing scripts

## Static Scripts

All scripts accept parameters via `globals()`. Use `--file` for defaults or `--script` with variable overrides:
```bash
# Default usage:
ue-exec.sh --file ~/.claude/skills/ue-editor/scripts/camera-look-at.py

# Parameterized:
ue-exec.sh --script '__target_actor__="MyActor"; exec(open("'$HOME'/.claude/skills/ue-editor/scripts/camera-look-at.py").read())'
```

| Script | Purpose | Params |
|--------|---------|--------|
| `screenshot.py` | Viewport screenshot → PNG | `__screenshot_width__`, `__screenshot_height__`, `__screenshot_path__` |
| `scene-tree.py` | Print scene hierarchy | (none) |
| `camera-look-at.py` | Position camera to face target | `__cam_x/y/z__`, `__target_x/y/z__` or `__target_actor__` |
| `find-assets.py` | Search assets by type/keyword | `__asset_type__`, `__keyword__`, `__search_dirs__` |
| `spawn-mesh-actor.py` | Spawn static mesh actor | `__mesh_path__`, `__label__`, `__x/y/z__`, `__scale__`, `__material__` |
| `spawn-niagara-ring.py` | Spawn Niagara FX in a ring | `__system_path__`, `__center_x/y/z__`, `__radius__`, `__count__`, `__label_prefix__` |
| `cleanup-actors.py` | Remove actors by label pattern | `__label_contains__`, `__dry_run__` |
| `resize-image.sh` | Resize/crop images (shell) | CLI args |

## Guidelines

- Prefer `unreal.get_editor_subsystem()` over deprecated `unreal.EditorLevelLibrary`.
- Combine multi-step operations into a single script to minimize round-trips.
- Scripts execute on the game thread — avoid long-running loops.
- Use `print()` for all output.
- Timeout 30s default; use the `timeout` parameter for heavy operations.
- **Always save assets after changes.** Call `unreal.EditorAssetLibrary.save_asset('/Game/Path/AssetName')` for each modified/created asset, or `unreal.EditorAssetLibrary.save_directory('/Game/Path/')` to save all assets in a directory. Unsaved assets are lost on editor restart and texture/material references may break.
- **Never call `unreal.EditorAssetLibrary.delete_asset()`** — it opens a modal confirmation dialog that blocks the game thread and freezes the editor when called from AgentBridge.
- **Screenshots**: For viewport captures with controlled camera placement, use `SceneCapture2D` actor + `export_render_target()` (outputs EXR, convert with `sips -s format png`). The `take_high_res_screenshot` method may not reflect recent camera changes.
- **Port auto-detection**: Never hardcode port 13090. Read `Saved/AgentBridge.port` for the actual port: `PORT=$(cat Saved/AgentBridge.port)`

see: knowledge/recipes.md — Common recipes: query actors, spawn, get/set properties, list assets, load level, viewport camera
see: knowledge/docs_python_scripting.md — Python scripting guide: execution methods, API patterns, best practices, transactions
see: knowledge/docs_editor_utilities.md — Editor Utility Widgets, Scripted Actions, Blueprint editor scripting patterns
see: knowledge/docs_subsystems.md — Programming Subsystems: Engine, Editor, GameInstance, LocalPlayer lifecycles
see: knowledge/docs_scriptable_tools.md — Scriptable Tools: custom interactive tools via Blueprint/Python
