---
name: ue-agent
description: Delegate complex multi-step Unreal Editor tasks to a subagent with its own context window. Use when a task needs 3+ editor operations, API exploration combined with scripting, or iterative trial-and-error that would clutter the main conversation.
---

# UE Agent — Subagent Delegation

Spawn a focused subagent for complex UE editor tasks that involve multiple steps, API exploration, or iterative experimentation.

## When to Delegate

- **3+ editor operations** in sequence (e.g., create asset → configure properties → place in level → verify)
- **API exploration + scripting** — searching for types, reading their methods, then writing a script that uses them
- **Bulk operations** — processing many actors/assets with retry logic
- **Iterative trial-and-error** — when the first script attempt may fail and need debugging

## When NOT to Delegate

- Single `ue_execute` call — just run it directly
- C++ code writing — use **ue-coder** skill
- Building or packaging — use **ue-builder** skill
- Quick health check — use `ue_health` directly
- Launching the editor — use **ue-runner** skill

## How to Spawn

Use the **Agent** tool with `subagent_type: "general-purpose"`. Include a focused prompt with the task objective, available tools, and expected output.

### Prompt Template

```
You are a UE editor automation agent. Complete the following task in the running Unreal Editor.

**Task:** [describe what needs to be done]

**Available MCP tools:**
- ue_health — check editor connectivity
- ue_execute(script, timeout) — run Python in editor (use `import unreal`, `print()` for output)
- ue_batch(scripts) — run multiple scripts sequentially
- ue_api_search(query) — search 15K+ UE Python API types
- ue_api_type(name) — get full type reference (properties, methods)
- ue_api_module(module) — list all types in a module
- ue_screenshot(width, height) — capture viewport
- ue_scene_tree() — get level actor hierarchy
- ue_play(mode) — control PIE (pie/simulate/stop)
- ue_build(wait) — trigger Live Coding compile
- ue_logs(lines, filter, severity) — read editor logs

**Shell fallback (if MCP tools unavailable):**
Use `bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh`:
- `--health` — health check
- `--script 'import unreal; ...'` — execute inline Python
- `--file /tmp/script.py` — execute from file
DO NOT use raw `curl`. All AgentBridge endpoints are under `/agent/` prefix
(e.g., `/agent/health`, `/agent/execute`). There is NO `/health` or `/api/v1/...`.

**Workflow:**
1. Run ue_health (or `ue-exec.sh --health`) to confirm the editor is reachable
2. Use ue_api_search / ue_api_type to find the right API types and methods
3. Write and execute Python scripts via ue_execute (or `ue-exec.sh --script`)
4. Verify results (ue_scene_tree, ue_screenshot, or another ue_execute)
5. Report a structured summary of what was done and the results

**Key patterns:**
- Always `import unreal` as the first line
- Access subsystems: `unreal.get_editor_subsystem(unreal.EditorActorSubsystem)`
- Properties: `obj.set_editor_property('name', value)` / `obj.get_editor_property('name')`
- Output: `print()` is the ONLY way to return data from scripts
- Combine related operations in one script to reduce round-trips
- **Always save assets** after creating/modifying: `unreal.EditorAssetLibrary.save_asset('/Game/Path/Asset')`
- **Never call `delete_asset()`** — it opens a modal dialog that freezes the editor

**CRITICAL — Level switching / map loading:**
Any script that calls `new_level()`, `load_level()`, `new_level_from_template()`, or any other API that changes the active map **MUST** use `--isolated` flag (shell) or `"isolated": true` in the JSON payload. Without it, Python's `FPyReferenceCollector` retains stale `UWorld` references in `__main__` global scope, preventing UE's GC from collecting the old map → **fatal crash** (`EditorServer.cpp` assert: "World Memory Leaks").

```bash
# Correct — level-switching script uses --isolated
ue-exec.sh --isolated --script '
import unreal
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_subsystem.new_level("/Game/Maps/MyNewLevel")
# All local vars freed when isolated scope exits; gc.collect() runs automatically
'
```

**Other API limitations:**
- `load_blueprint_class(path)` does **not exist** — use `unreal.EditorAssetLibrary.load_asset(path)` for BP assets or `unreal.load_class(None, "/Script/Module.ClassName")` for C++ classes
- Properties with `EditDefaultsOnly` on actor instances cannot be set from Python — set them manually in the editor's Details/World Settings panel

**Reusable static scripts** (use instead of writing inline Python):
```bash
SCRIPTS="$HOME/.claude/skills/ue-editor/scripts"
# Camera look-at (NEVER guess rotator angles — always use this):
ue-exec.sh --script '__target_actor__="MyActor"; exec(open("'$SCRIPTS'/camera-look-at.py").read())'
# Find assets by type:
ue-exec.sh --script '__asset_type__="NiagaraSystem"; exec(open("'$SCRIPTS'/find-assets.py").read())'
# Spawn mesh actor:
ue-exec.sh --script '__label__="MySphere"; __scale__=4; exec(open("'$SCRIPTS'/spawn-mesh-actor.py").read())'
# Spawn Niagara ring:
ue-exec.sh --script '__system_path__="/Game/Effects/NS_Sparks"; __radius__=200; __count__=8; exec(open("'$SCRIPTS'/spawn-niagara-ring.py").read())'
# Cleanup actors:
ue-exec.sh --script '__label_contains__="ShieldSpark"; exec(open("'$SCRIPTS'/cleanup-actors.py").read())'
# Screenshot & scene tree:
ue-exec.sh --file $SCRIPTS/screenshot.py
ue-exec.sh --file $SCRIPTS/scene-tree.py
```

**Output format:**
Return a structured summary:
- What was done (steps taken)
- Results (data returned, actors created, etc.)
- Any errors encountered and how they were resolved
```

### Example Invocation

```python
# Spawning a subagent to populate a level with properly configured lights
Agent(
    subagent_type="general-purpose",
    description="Set up level lighting",
    prompt="""You are a UE editor automation agent. Complete the following task...

    **Task:** Add 3 point lights to the current level at positions (0,0,300),
    (500,0,300), (-500,0,300) with intensity 5000 and warm white color (1.0, 0.85, 0.7).
    Verify they appear in the scene tree.

    [include available tools and workflow from template above]
    """
)
```

## CRITICAL — Niagara Particle Systems

**Do NOT try to author/configure Niagara emitter internals from Python.** The API cannot set spawn rate, velocity, lifetime, gravity, forces, or renderer properties. Attempting this wastes iterations and can crash the editor (SIGSEGV from `NiagaraPythonEmitter`).

**Correct approach for particle effects:**
1. **Reuse existing project effects** — search `/Game/Effects/`, `/Game/FX/`, `/Game/VFX/`, duplicate the closest match, reposition via Python
2. **Set only user parameter overrides** on `NiagaraComponent`: `set_float_parameter()`, `set_vector_parameter()`, `set_color_parameter()`
3. **Tell the user** to customize behavior in the Niagara Editor UI — Python cannot do it
4. **NEVER**: instantiate `NiagaraPythonEmitter`, call `help()`/`dir()` on Niagara classes, or iterate >3 times trying property names

See `ue-editor/knowledge/niagara.md` for full API limitations and crash triggers.

## Tips

- Keep subagent prompts focused on a single objective
- Include the full tool list — the subagent does not inherit skill context
- For very large tasks, break into multiple subagent calls rather than one giant prompt
- The subagent's output is returned to you — summarize it for the user
