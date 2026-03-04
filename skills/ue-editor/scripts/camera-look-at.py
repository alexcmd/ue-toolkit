"""Position viewport camera to look at a target point.

Params (set as globals before exec, or uses defaults):
  __cam_x__, __cam_y__, __cam_z__  — camera position (default: 500, -500, 300)
  __target_x__, __target_y__, __target_z__ — look-at target (default: 0, 0, 0)
  __target_actor__ — actor label to look at (overrides target xyz if set)

Usage:
  # With defaults:
  ue-exec.sh --file camera-look-at.py

  # With params:
  ue-exec.sh --script '__cam_x__=1500; __cam_y__=-450; __cam_z__=200; __target_actor__="ShieldSphere"; exec(open("...camera-look-at.py").read())'
"""
import unreal

g = globals()
target_label = g.get("__target_actor__", "")

# Resolve target from actor label if provided
if target_label:
    subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in subsys.get_all_level_actors():
        if a.get_actor_label() == target_label:
            loc = a.get_actor_location()
            tx, ty, tz = loc.x, loc.y, loc.z
            break
    else:
        print("ERROR: Actor '{}' not found".format(target_label))
        tx, ty, tz = 0, 0, 0
else:
    tx = g.get("__target_x__", 0)
    ty = g.get("__target_y__", 0)
    tz = g.get("__target_z__", 0)

cx = g.get("__cam_x__", tx + 500)
cy = g.get("__cam_y__", ty - 500)
cz = g.get("__cam_z__", tz + 300)

cam_pos = unreal.Vector(cx, cy, cz)
target = unreal.Vector(tx, ty, tz)

rot = unreal.MathLibrary.find_look_at_rotation(cam_pos, target)

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
ues.set_level_viewport_camera_info(cam_pos, rot)

print("Camera: ({:.0f}, {:.0f}, {:.0f}) -> ({:.0f}, {:.0f}, {:.0f}) pitch={:.1f} yaw={:.1f}".format(
    cx, cy, cz, tx, ty, tz, rot.pitch, rot.yaw))
