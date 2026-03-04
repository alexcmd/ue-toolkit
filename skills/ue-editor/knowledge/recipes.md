# UE Editor Recipes

Common Python recipes for editor automation. Execute via ue-scripter:
```bash
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --script '...'
```

## Coordinate system and units

**Axes** — left-handed, Z-up:
| Axis | Color | Direction | Positive |
|------|-------|-----------|----------|
| X | Red | Forward/back | Forward |
| Y | Green | Left/right | Right |
| Z | Blue | Up/down | Up |

**Units** — 1 Unreal Unit = 1 centimeter. Angles in degrees.

**Default mesh sizes** (before scaling):
| Shape | Size |
|-------|------|
| Sphere (`/Engine/BasicShapes/Sphere`) | 100 cm diameter (50 cm radius) |
| Cube (`/Engine/BasicShapes/Cube`) | 100 × 100 × 100 cm |
| Cylinder | 100 cm diameter, 100 cm tall |
| Plane | 100 × 100 cm |

To get a 5-meter diameter sphere: `set_actor_scale3d(Vector(50, 50, 50))` — scale × 100 cm = size.

**FRotator** — `(Pitch, Yaw, Roll)` as properties, but the **Python constructor order is `(roll, pitch, yaw)`**:
| Property | Axis | Positive direction |
|----------|------|--------------------|
| Pitch | Around Y | Looking up |
| Yaw | Around Z | Turning right (clockwise from above) |
| Roll | Around X | Tilting right |

```python
# WRONG — constructor is NOT (pitch, yaw, roll):
rot = unreal.Rotator(pitch, yaw, 0)

# RIGHT — constructor is (roll, pitch, yaw):
rot = unreal.Rotator(0, -20, 180)  # roll=0, pitch=-20 (look down 20°), yaw=180 (face -X)
print(rot.pitch, rot.yaw, rot.roll)  # -20.0  180.0  0.0
```

**Yaw reference**: 0° = +X, 90° = +Y, 180°/-180° = −X, −90° = −Y.

## Camera look-at

NEVER compute pitch/yaw with `math.atan2` — UE's left-handed conventions make manual math error-prone.
ALWAYS use `MathLibrary.find_look_at_rotation`:
```python
import unreal
cam_pos = unreal.Vector(500, 300, 400)
target  = unreal.Vector(0, 0, 200)
look_rot = unreal.MathLibrary.find_look_at_rotation(cam_pos, target)

subsys = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
subsys.set_level_viewport_camera_info(cam_pos, look_rot)
```

**Camera distance**: ensure the camera is far enough from the target to see it. For a sphere
of radius R, place the camera at least `R * 2.5` away. Example: sphere at scale 5 (radius
250 cm) → camera at least 625 cm from center.

## Screenshots

`take_high_res_screenshot` is **async** and captures a **stale frame**. Follow this protocol:

1. **Script 1** — make all changes (camera, material, actors)
2. **Wait** — the editor must render at least 1-2 frames with the new state
3. **Script 2** (separate execution) — take the screenshot
4. **Wait ~5-10 seconds** before reading the file from disk (it writes asynchronously)

Always use absolute paths for the output file:
```python
import unreal, os
saved = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())
out = os.path.join(saved, "Screenshots", "my_shot.png")
os.makedirs(os.path.dirname(out), exist_ok=True)
unreal.AutomationLibrary.take_high_res_screenshot(1280, 720, out)
```

## Custom material expressions (HLSL)

Custom HLSL nodes compile across **all shader permutations** (base pass, depth pass,
debug view, etc.). Inputs may not be available in every permutation, causing errors like
`undeclared identifier 'UV'`.

**Rules for Custom nodes:**
- NEVER reference input names directly in the code body without testing all permutations
- Input variables become available as function-scope parameters, but some permutations
  (like `FDebugViewModePS`) may not pass them
- Prefer **standard material expression nodes** over Custom HLSL when possible — they
  compile correctly in all permutations
- If Custom HLSL is required, **test compilation** by checking editor logs after
  `recompile_material()` — look for `Failed to compile Material` warnings
- Use `ue_logs(filter="ShaderCompiler", severity="warning")` to detect shader compile failures

**Bad — Custom HLSL with raw input reference:**
```python
custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, 0, 0)
custom.set_editor_property("code", "float2 uv = UV; ...")  # UV may be undeclared!
```

**Good — standard nodes that compile everywhere:**
```python
# Use built-in expressions: Fresnel, OneMinus, Add, Multiply, Sine, Time, etc.
fresnel = mel.create_material_expression(mat, unreal.MaterialExpressionFresnel, 0, 0)
fresnel.set_editor_property("exponent", 3.0)
```

## Materials — translucent/emissive

- Set `blend_mode = BLEND_TRANSLUCENT`, `shading_model = MSM_UNLIT`, `two_sided = True`
- **Emissive brightness**: keep multiplier at **2-5×** max. Higher values (10×+) cause
  post-process bloom to wash out the material to solid white in screenshots
- **Opacity**: use `Clamp` node to keep values in [0, 1]. For a visible-but-transparent
  shield, set hex lines to 0.6-0.8 opacity and cell interiors to 0.0
- **Verify compilation**: after `recompile_material()`, check logs for shader errors before
  proceeding — a silently failed material renders as default gray

## Spawning actors

Before spawning, **check the scene** for existing actors at the target location:
```python
import unreal
subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = subsys.get_all_level_actors()
target_loc = unreal.Vector(0, 0, 200)
for a in actors:
    loc = a.get_actor_location()
    dist = ((loc.x - target_loc.x)**2 + (loc.y - target_loc.y)**2 + (loc.z - target_loc.z)**2)**0.5
    if dist < 100:
        print(f"WARNING: {a.get_actor_label()} already at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")
```

## Query editor world and actors
```python
import unreal
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
for a in actors:
    print(f"{a.get_name()} ({a.get_class().get_name()})")
```

## Get/set actor properties
Use `set_editor_property` / `get_editor_property` — direct attribute access does NOT work:
```python
# WRONG: obj.speed_x = 0.1
# RIGHT:
obj.set_editor_property("speed_x", 0.1)
val = obj.get_editor_property("speed_x")
```

## Get viewport camera info
Returns a tuple `(location, rotation)` — no arguments:
```python
import unreal
subsys = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
loc, rot = subsys.get_level_viewport_camera_info()
```

## Batch execution
Execute multiple scripts in a single round-trip:
```bash
cat > /tmp/batch.json << 'EOF'
[
  {"id": "step1", "script": "import unreal\nprint(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_name())"},
  {"id": "step2", "script": "import unreal\nactors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()\nprint(len(actors))"}
]
EOF
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --batch /tmp/batch.json --stop-on-error
```

## Screenshot and resize
```bash
# Take screenshot
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --file ~/.claude/skills/ue-editor/scripts/screenshot.py

# Scale down to reduce tokens
bash ~/.claude/skills/ue-editor/scripts/resize-image.sh /path/to/image.png --max-width 400

# Resize to exact dimensions
bash ~/.claude/skills/ue-editor/scripts/resize-image.sh /path/to/image.png --resize 640x480

# Crop a region (left, top, right, bottom)
bash ~/.claude/skills/ue-editor/scripts/resize-image.sh /path/to/image.png --crop 100,50,700,500
```

## Scene tree
```bash
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --file ~/.claude/skills/ue-editor/scripts/scene-tree.py
```
Output format:
```
Level: LevelName
Actors: 20
---
ActorLabel [ClassName] (x, y, z)
  ChildActor [ClassName] (x, y, z)
```
