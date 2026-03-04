# Material Recipes

Copy-paste-ready Python recipes for common material workflows. Execute via `ue_execute` MCP tool or `ue-exec.sh --script '...'`.

All recipes use:
```python
import unreal
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
ath = unreal.AssetToolsHelpers.get_asset_tools()
aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
```

## CRITICAL — Safe Material Create/Load

**NEVER call `create_asset()` if the asset already exists** — it opens a modal override dialog that freezes the editor. ALWAYS use this pattern:

```python
def get_or_create_material(name, path):
    """Load existing material or create new. Never triggers modal dialogs."""
    full_path = "{}/{}".format(path, name)
    if eal.does_asset_exist(full_path):
        mat = eal.load_asset(full_path)
        if mat is not None:
            # Clean slate: clear existing expressions
            for i in range(3):
                mel.delete_all_material_expressions(mat)
            print("Loaded existing: {}".format(full_path))
            return mat
    # Create new
    mat = ath.create_asset(name, path, unreal.Material, unreal.MaterialFactoryNew())
    if mat is None:
        print("ERROR: Failed to create material at {}".format(full_path))
    else:
        print("Created new: {}".format(full_path))
    return mat
```

**ALWAYS validate the return value before using it:**
```python
mat = get_or_create_material('M_MyMaterial', '/Game/Materials')
if mat is None:
    raise RuntimeError("Material create/load failed — check logs")
```

---

## 0. Open Material in Material Editor

Open the Material Editor with its live preview viewport and node graph. Use for visual verification.

```python
import unreal
eal = unreal.EditorAssetLibrary
aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)

mat = eal.load_asset('/Game/Materials/M_BasicPBR')

# Open — launches Material Editor with preview + node graph
aes.open_editor_for_assets([mat])

# Optionally set preview mesh (Sphere, Cube, Cylinder, Plane)
mat.set_editor_property('preview_mesh',
    unreal.SoftObjectPath('/Engine/EditorMeshes/EditorSphere.EditorSphere'))

print('Material Editor opened for M_BasicPBR')
```

Close when done:
```python
aes.close_all_editors_for_asset(mat)
```

**Note:** Preview camera (zoom/orbit) is not exposed to Python — only the preview mesh can be set programmatically. The node graph and preview update live when you modify the material via `MaterialEditingLibrary` + `recompile_material()`.

---

## 1. Basic PBR Material

Create a simple opaque material with constant base color, metallic, and roughness.

```python
import unreal
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
ath = unreal.AssetToolsHelpers.get_asset_tools()

# Safe create-or-load (NEVER create_asset on existing path — modal dialog!)
mat_path = '/Game/Materials/M_BasicPBR'
if eal.does_asset_exist(mat_path):
    mat = eal.load_asset(mat_path)
    for i in range(3):
        mel.delete_all_material_expressions(mat)
else:
    mat = ath.create_asset('M_BasicPBR', '/Game/Materials', unreal.Material, unreal.MaterialFactoryNew())
if mat is None:
    print('ERROR: Failed to create/load material')
    raise RuntimeError('Material is None')

# Base color — dark metal
color = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -200)
color.set_editor_property('constant', unreal.LinearColor(0.02, 0.02, 0.02, 1.0))
mel.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)

# Metallic — fully metallic
metallic = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 0)
metallic.set_editor_property('r', 1.0)
mel.connect_material_property(metallic, '', unreal.MaterialProperty.MP_METALLIC)

# Roughness — very smooth
roughness = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
roughness.set_editor_property('r', 0.1)
mel.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(mat)
eal.save_asset('/Game/Materials/M_BasicPBR')
print('Created /Game/Materials/M_BasicPBR')
```

---

## 2. Parameterized Material

Material with exposed ScalarParameter and VectorParameter for instance overrides.

```python
import unreal
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
ath = unreal.AssetToolsHelpers.get_asset_tools()

mat = ath.create_asset('M_Parameterized', '/Game/Materials', unreal.Material, unreal.MaterialFactoryNew())

# Base color parameter
color_param = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -200)
color_param.set_editor_property('parameter_name', 'BaseColor')
color_param.set_editor_property('default_value', unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
mel.connect_material_property(color_param, '', unreal.MaterialProperty.MP_BASE_COLOR)

# Metallic parameter
metallic_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 0)
metallic_param.set_editor_property('parameter_name', 'Metallic')
metallic_param.set_editor_property('default_value', 0.0)
mel.connect_material_property(metallic_param, '', unreal.MaterialProperty.MP_METALLIC)

# Roughness parameter
roughness_param = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 200)
roughness_param.set_editor_property('parameter_name', 'Roughness')
roughness_param.set_editor_property('default_value', 0.5)
mel.connect_material_property(roughness_param, '', unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(mat)
eal.save_asset('/Game/Materials/M_Parameterized')

# Verify parameters
names = mel.get_scalar_parameter_names(mat)
print(f'Scalar params: {[str(n) for n in names]}')
names = mel.get_vector_parameter_names(mat)
print(f'Vector params: {[str(n) for n in names]}')
```

---

## 3. Material Instance from Parent

Create a MaterialInstanceConstant that overrides parent parameters.

```python
import unreal
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
ath = unreal.AssetToolsHelpers.get_asset_tools()

# Load parent material (must have parameters exposed)
parent = eal.load_asset('/Game/Materials/M_Parameterized')

# Create instance
mic = ath.create_asset('MI_RedVariant', '/Game/Materials', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
mel.set_material_instance_parent(mic, parent)

# Override parameters
mel.set_material_instance_vector_parameter_value(mic, 'BaseColor', unreal.LinearColor(0.8, 0.1, 0.05, 1.0))
mel.set_material_instance_scalar_parameter_value(mic, 'Metallic', 0.9)
mel.set_material_instance_scalar_parameter_value(mic, 'Roughness', 0.2)

eal.save_asset('/Game/Materials/MI_RedVariant')
print('Created MI_RedVariant with parent M_Parameterized')

# Verify
val = mel.get_material_instance_scalar_parameter_value(mic, 'Roughness')
print(f'Roughness = {val}')
```

---

## 4. Apply Material to Actor

Find an actor by label and set its material.

```python
import unreal
eal = unreal.EditorAssetLibrary
subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Find actor
target_label = 'MySphere'
actor = None
for a in subsys.get_all_level_actors():
    if a.get_actor_label() == target_label:
        actor = a
        break

if actor is None:
    print(f'ERROR: Actor "{target_label}" not found')
else:
    # Get mesh component
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if mesh is None:
        mesh = actor.get_component_by_class(unreal.SkeletalMeshComponent)

    if mesh is None:
        print('ERROR: No mesh component found on actor')
    else:
        # Load and apply material
        mat = eal.load_asset('/Game/Materials/M_BasicPBR')
        mesh.set_material(0, mat)
        print(f'Applied M_BasicPBR to {target_label} slot 0')
```

---

## 5. Dynamic Material Instance (Runtime)

Create a dynamic material instance for runtime parameter changes.

```python
import unreal
eal = unreal.EditorAssetLibrary
subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

target_label = 'MySphere'
actor = None
for a in subsys.get_all_level_actors():
    if a.get_actor_label() == target_label:
        actor = a
        break

if actor:
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    source_mat = eal.load_asset('/Game/Materials/M_Parameterized')

    # Create dynamic material instance
    dmi = mesh.create_dynamic_material_instance(0, source_mat)
    dmi.set_scalar_parameter_value('Roughness', 0.05)
    dmi.set_vector_parameter_value('BaseColor', unreal.LinearColor(0.0, 0.5, 1.0, 1.0))
    print(f'Created dynamic material instance on {target_label}')
else:
    print(f'ERROR: Actor "{target_label}" not found')
```

---

## 6. Translucent Emissive Material

Material with translucency, unlit shading, and emissive glow.

```python
import unreal
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
ath = unreal.AssetToolsHelpers.get_asset_tools()

mat = ath.create_asset('M_TranslucentGlow', '/Game/Materials', unreal.Material, unreal.MaterialFactoryNew())

# Material properties for translucency
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided', True)

# Emissive color parameter
emissive_color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, -200)
emissive_color.set_editor_property('parameter_name', 'EmissiveColor')
emissive_color.set_editor_property('default_value', unreal.LinearColor(0.0, 0.8, 1.0, 1.0))

# Emissive intensity multiplier (keep low: 2-5x to avoid bloom washout)
intensity = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 0)
intensity.set_editor_property('parameter_name', 'EmissiveIntensity')
intensity.set_editor_property('default_value', 3.0)

# Multiply color by intensity
multiply = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, -100)
mel.connect_material_expressions(emissive_color, '', multiply, 'A')
mel.connect_material_expressions(intensity, '', multiply, 'B')
mel.connect_material_property(multiply, '', unreal.MaterialProperty.MP_EMISSION_COLOR)

# Opacity
opacity = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 200)
opacity.set_editor_property('parameter_name', 'Opacity')
opacity.set_editor_property('default_value', 0.7)
mel.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)

mel.recompile_material(mat)
eal.save_asset('/Game/Materials/M_TranslucentGlow')
print('Created /Game/Materials/M_TranslucentGlow (translucent, unlit, emissive)')
```

---

## 7. Texture-Mapped Material

Material with texture sample, UV tiling, and normal map.

```python
import unreal
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
ath = unreal.AssetToolsHelpers.get_asset_tools()

mat = ath.create_asset('M_TextureMapped', '/Game/Materials', unreal.Material, unreal.MaterialFactoryNew())

# UV tiling
uv_coord = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -800, 0)
uv_coord.set_editor_property('u_tiling', 2.0)
uv_coord.set_editor_property('v_tiling', 2.0)

# Diffuse texture parameter
diffuse_tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, -200)
diffuse_tex.set_editor_property('parameter_name', 'DiffuseTexture')
mel.connect_material_expressions(uv_coord, '', diffuse_tex, 'UVs')
mel.connect_material_property(diffuse_tex, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)

# Normal map texture parameter
normal_tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 200)
normal_tex.set_editor_property('parameter_name', 'NormalTexture')
# Set sampler type to normal for correct compression handling
normal_tex.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
mel.connect_material_expressions(uv_coord, '', normal_tex, 'UVs')
mel.connect_material_property(normal_tex, 'RGB', unreal.MaterialProperty.MP_NORMAL)

# Roughness parameter
roughness = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 400)
roughness.set_editor_property('parameter_name', 'Roughness')
roughness.set_editor_property('default_value', 0.5)
mel.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(mat)
eal.save_asset('/Game/Materials/M_TextureMapped')
print('Created /Game/Materials/M_TextureMapped')
print(f'Texture params: {[str(n) for n in mel.get_texture_parameter_names(mat)]}')
```

---

## 8. Inspect Material Parameters and Statistics

Query an existing material for its parameters, textures, and shader statistics.

```python
import unreal
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

mat_path = '/Game/Materials/M_Parameterized'
mat = eal.load_asset(mat_path)

if mat is None:
    print(f'ERROR: Material not found at {mat_path}')
else:
    print(f'Material: {mat_path}')
    print(f'Expressions: {mel.get_num_material_expressions(mat)}')

    # Parameters
    scalars = mel.get_scalar_parameter_names(mat)
    vectors = mel.get_vector_parameter_names(mat)
    textures = mel.get_texture_parameter_names(mat)
    print(f'Scalar params: {[str(n) for n in scalars]}')
    print(f'Vector params: {[str(n) for n in vectors]}')
    print(f'Texture params: {[str(n) for n in textures]}')

    # Default values
    for name in scalars:
        val = mel.get_material_default_scalar_parameter_value(mat, str(name))
        print(f'  {name} = {val}')
    for name in vectors:
        val = mel.get_material_default_vector_parameter_value(mat, str(name))
        print(f'  {name} = ({val.r:.2f}, {val.g:.2f}, {val.b:.2f}, {val.a:.2f})')

    # Textures used
    used_textures = mel.get_used_textures(mat)
    print(f'Used textures: {[t.get_name() for t in used_textures]}')

    # Shader statistics
    stats = mel.get_statistics(mat)
    print(f'Pixel shader instructions: {stats.num_pixel_shader_instructions}')
    print(f'Texture samples: {stats.num_pixel_texture_samples}')
    print(f'Samplers: {stats.num_samplers}')
```

---

## 9. Animated Material (Panner + Sine)

Material with scrolling UVs and pulsing emissive driven by Time/Sine nodes.

```python
import unreal
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
ath = unreal.AssetToolsHelpers.get_asset_tools()

mat = ath.create_asset('M_Animated', '/Game/Materials', unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided', True)

# --- Scrolling UV for base pattern ---
uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -800, -200)
uv.set_editor_property('u_tiling', 4.0)
uv.set_editor_property('v_tiling', 4.0)

panner = mel.create_material_expression(mat, unreal.MaterialExpressionPanner, -600, -200)
panner.set_editor_property('speed_x', 0.1)
panner.set_editor_property('speed_y', 0.05)
mel.connect_material_expressions(uv, '', panner, 'Coordinate')

# --- Pulsing emissive via Sine(Time) ---
time_node = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -800, 100)
sine_node = mel.create_material_expression(mat, unreal.MaterialExpressionSine, -600, 100)
mel.connect_material_expressions(time_node, '', sine_node, '')

# Remap sine [-1,1] to [1,3] for emissive intensity
# (sine + 1) = [0, 2], then + 1 = [1, 3]
one_const = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, 200)
one_const.set_editor_property('r', 1.0)
add_node = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -400, 100)
mel.connect_material_expressions(sine_node, '', add_node, 'A')
mel.connect_material_expressions(one_const, '', add_node, 'B')

# Emissive color
emissive_color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, -50)
emissive_color.set_editor_property('parameter_name', 'EmissiveColor')
emissive_color.set_editor_property('default_value', unreal.LinearColor(0.0, 1.0, 0.5, 1.0))

multiply = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 0)
mel.connect_material_expressions(emissive_color, '', multiply, 'A')
mel.connect_material_expressions(add_node, '', multiply, 'B')
mel.connect_material_property(multiply, '', unreal.MaterialProperty.MP_EMISSION_COLOR)

# Opacity
opacity = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300)
opacity.set_editor_property('r', 0.6)
mel.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)

mel.recompile_material(mat)
eal.save_asset('/Game/Materials/M_Animated')
print('Created /Game/Materials/M_Animated (scrolling UVs + pulsing emissive)')
```

---

## 10. Shield Effect (Edge Glow + Transparent Center + Scanning)

Energy shield: hex pattern, bright edges, see-through center, animated scan line.

```python
import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

mat = eal.load_asset("/Game/Materials/M_HexShield")  # or create new
hex_tex = eal.load_asset("/Game/Effects/Textures/Decals/hexagon")

# Clean slate
for i in range(3):
    mel.delete_all_material_expressions(mat)

mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)

# --- HEX TEXTURE (standard UVs — NOT triplanar!) ---
uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
uv.set_editor_property("u_tiling", 5.0)
uv.set_editor_property("v_tiling", 5.0)

tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -1000, 0)
tex.set_editor_property("texture", hex_tex)
mel.connect_material_expressions(uv, "", tex, "UVs")

# --- FRESNEL (drives edge vs center visibility) ---
fresnel = mel.create_material_expression(mat, unreal.MaterialExpressionFresnel, -1000, 300)
fresnel.set_editor_property("exponent", 2.0)
fresnel.set_editor_property("base_reflect_fraction", 0.02)

# --- SCANNING BAND ---
world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1200, 700)
obj_pos = mel.create_material_expression(mat, unreal.MaterialExpressionObjectPositionWS, -1200, 600)
local_pos = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -1000, 650)
mel.connect_material_expressions(world_pos, "", local_pos, "A")
mel.connect_material_expressions(obj_pos, "", local_pos, "B")

z_mask = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -800, 650)
z_mask.set_editor_property("r", False)
z_mask.set_editor_property("g", False)
z_mask.set_editor_property("b", True)
mel.connect_material_expressions(local_pos, "", z_mask, "")

div_range = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -800, 730)
div_range.set_editor_property("r", 500.0)  # sphere diameter (scale 5)
z_norm = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -600, 650)
mel.connect_material_expressions(z_mask, "", z_norm, "A")
mel.connect_material_expressions(div_range, "", z_norm, "B")
half_c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, 730)
half_c.set_editor_property("r", 0.5)
z_01 = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -450, 650)
mel.connect_material_expressions(z_norm, "", z_01, "A")
mel.connect_material_expressions(half_c, "", z_01, "B")

# Animated scan position
time_n = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -800, 900)
scan_speed = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -800, 970)
scan_speed.set_editor_property("r", 0.8)
t_mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -600, 900)
mel.connect_material_expressions(time_n, "", t_mul, "A")
mel.connect_material_expressions(scan_speed, "", t_mul, "B")
sin_n = mel.create_material_expression(mat, unreal.MaterialExpressionSine, -450, 900)
mel.connect_material_expressions(t_mul, "", sin_n, "")
half2 = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -450, 970)
half2.set_editor_property("r", 0.5)
sin_scaled = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 900)
mel.connect_material_expressions(sin_n, "", sin_scaled, "A")
mel.connect_material_expressions(half2, "", sin_scaled, "B")
scan_pos = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -150, 900)
mel.connect_material_expressions(sin_scaled, "", scan_pos, "A")
mel.connect_material_expressions(half2, "", scan_pos, "B")

scan_diff = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -150, 700)
mel.connect_material_expressions(z_01, "", scan_diff, "A")
mel.connect_material_expressions(scan_pos, "", scan_diff, "B")
scan_abs = mel.create_material_expression(mat, unreal.MaterialExpressionAbs, 0, 700)
mel.connect_material_expressions(scan_diff, "", scan_abs, "")
band_width = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 0, 780)
band_width.set_editor_property("r", 0.08)
scan_ratio = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, 150, 700)
mel.connect_material_expressions(scan_abs, "", scan_ratio, "A")
mel.connect_material_expressions(band_width, "", scan_ratio, "B")
scan_inv = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, 300, 700)
mel.connect_material_expressions(scan_ratio, "", scan_inv, "")
zero_c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 300, 780)
zero_c.set_editor_property("r", 0.0)
scan_band = mel.create_material_expression(mat, unreal.MaterialExpressionMax, 450, 700)
mel.connect_material_expressions(scan_inv, "", scan_band, "A")
mel.connect_material_expressions(zero_c, "", scan_band, "B")

# --- COMBINE: Fresnel + scan = visibility mask ---
fresnel_plus_scan = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -600, 400)
mel.connect_material_expressions(fresnel, "", fresnel_plus_scan, "A")
mel.connect_material_expressions(scan_band, "", fresnel_plus_scan, "B")

# CRITICAL: texture * visibility (NOT texture + visibility)
hex_x_vis = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 200)
mel.connect_material_expressions(tex, "", hex_x_vis, "A")
mel.connect_material_expressions(fresnel_plus_scan, "", hex_x_vis, "B")

# Faint base hex everywhere
base_str = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 100)
base_str.set_editor_property("r", 0.05)
base_hex = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 50)
mel.connect_material_expressions(tex, "", base_hex, "A")
mel.connect_material_expressions(base_str, "", base_hex, "B")

pattern = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, 0, 100)
mel.connect_material_expressions(hex_x_vis, "", pattern, "A")
mel.connect_material_expressions(base_hex, "", pattern, "B")

# --- EMISSIVE ---
color = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, 0, -200)
color.set_editor_property("constant", unreal.LinearColor(0.3, 0.5, 1.0, 1.0))
em1 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 200, -50)
mel.connect_material_expressions(color, "", em1, "A")
mel.connect_material_expressions(pattern, "", em1, "B")
brt = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 200, -150)
brt.set_editor_property("r", 5.0)
em_out = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 400, -100)
mel.connect_material_expressions(em1, "", em_out, "A")
mel.connect_material_expressions(brt, "", em_out, "B")

# --- OPACITY ---
op_scale = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, 200, 200)
op_scale.set_editor_property("r", 0.85)
op_out = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 400, 150)
mel.connect_material_expressions(pattern, "", op_out, "A")
mel.connect_material_expressions(op_scale, "", op_out, "B")

mel.connect_material_property(em_out, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(op_out, "", unreal.MaterialProperty.MP_OPACITY)

mel.layout_material_expressions(mat)
mel.recompile_material(mat)
eal.save_asset("/Game/Materials/M_HexShield")
print("Shield material created")
```

---

## 11. Fresnel Rim-Light Material

Opaque material with a Fresnel-based rim highlight on emissive channel.

```python
import unreal
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
ath = unreal.AssetToolsHelpers.get_asset_tools()

mat = ath.create_asset('M_FresnelRim', '/Game/Materials', unreal.Material, unreal.MaterialFactoryNew())

# Base color — dark surface
base_color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -300)
base_color.set_editor_property('parameter_name', 'BaseColor')
base_color.set_editor_property('default_value', unreal.LinearColor(0.05, 0.05, 0.08, 1.0))
mel.connect_material_property(base_color, '', unreal.MaterialProperty.MP_BASE_COLOR)

# Metallic
metallic = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, -100)
metallic.set_editor_property('parameter_name', 'Metallic')
metallic.set_editor_property('default_value', 0.0)
mel.connect_material_property(metallic, '', unreal.MaterialProperty.MP_METALLIC)

# Roughness
roughness = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 0)
roughness.set_editor_property('parameter_name', 'Roughness')
roughness.set_editor_property('default_value', 0.4)
mel.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)

# Fresnel for rim lighting
fresnel = mel.create_material_expression(mat, unreal.MaterialExpressionFresnel, -500, 200)
fresnel.set_editor_property('exponent', 3.0)
fresnel.set_editor_property('base_reflect_fraction', 0.02)

# Rim color
rim_color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, 350)
rim_color.set_editor_property('parameter_name', 'RimColor')
rim_color.set_editor_property('default_value', unreal.LinearColor(0.0, 0.5, 1.0, 1.0))

# Rim intensity
rim_intensity = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 450)
rim_intensity.set_editor_property('parameter_name', 'RimIntensity')
rim_intensity.set_editor_property('default_value', 2.0)

# Fresnel * RimColor * RimIntensity → Emissive
mul1 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 250)
mel.connect_material_expressions(fresnel, '', mul1, 'A')
mel.connect_material_expressions(rim_color, '', mul1, 'B')

mul2 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -100, 300)
mel.connect_material_expressions(mul1, '', mul2, 'A')
mel.connect_material_expressions(rim_intensity, '', mul2, 'B')
mel.connect_material_property(mul2, '', unreal.MaterialProperty.MP_EMISSION_COLOR)

mel.recompile_material(mat)
eal.save_asset('/Game/Materials/M_FresnelRim')
print('Created /Game/Materials/M_FresnelRim (Fresnel rim-light, emissive)')
params = mel.get_scalar_parameter_names(mat)
print(f'Scalar params: {[str(n) for n in params]}')
params = mel.get_vector_parameter_names(mat)
print(f'Vector params: {[str(n) for n in params]}')
```
