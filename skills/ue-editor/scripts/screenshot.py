"""Take a viewport screenshot at a given resolution.
Default: 800x600 (small for token efficiency).
Output: Saved/Screenshots/viewport.png (or custom path).
"""
import unreal
import os

width = globals().get("__screenshot_width__", 800)
height = globals().get("__screenshot_height__", 600)
default_path = os.path.join(unreal.Paths.project_saved_dir(), "Screenshots", "viewport.png")
out_path = globals().get("__screenshot_path__", default_path)

os.makedirs(os.path.dirname(out_path), exist_ok=True)

unreal.AutomationLibrary.take_high_res_screenshot(width, height, out_path)
print(out_path)
