"""Spawn a static mesh actor in the level.

Params:
  __mesh_path__  — static mesh asset path (default: /Engine/BasicShapes/Sphere)
  __label__      — actor label (default: "MeshActor")
  __x/y/z__      — position (default: 0, 0, 0)
  __scale__      — uniform scale (default: 1.0)
  __material__   — material asset path to apply (optional)

Usage:
  ue-exec.sh --script '__mesh_path__="/Engine/BasicShapes/Sphere"; __label__="ShieldSphere"; __x__=1500; __z__=100; __scale__=4; __material__="/Game/Materials/M_HexShield"; exec(open("...spawn-mesh-actor.py").read())'
"""
import unreal

g = globals()
mesh_path = g.get("__mesh_path__", "/Engine/BasicShapes/Sphere")
label = g.get("__label__", "MeshActor")
x = float(g.get("__x__", 0))
y = float(g.get("__y__", 0))
z = float(g.get("__z__", 0))
scale = float(g.get("__scale__", 1.0))
mat_path = g.get("__material__", "")

eal = unreal.EditorAssetLibrary
subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

loc = unreal.Vector(x, y, z)
actor = subsys.spawn_actor_from_class(unreal.StaticMeshActor, loc)
actor.set_actor_label(label)
actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))

mesh_comp = actor.get_component_by_class(unreal.StaticMeshComponent)
mesh = unreal.load_asset(mesh_path)
mesh_comp.set_static_mesh(mesh)

if mat_path:
    mat = eal.load_asset(mat_path)
    if mat:
        mesh_comp.set_material(0, mat)
        print("Applied material: {}".format(mat_path))

print("Spawned '{}' at ({:.0f}, {:.0f}, {:.0f}) scale={} mesh={}".format(
    label, x, y, z, scale, mesh_path))
