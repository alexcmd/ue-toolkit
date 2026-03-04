---
name: ue-runner
description: Launch Unreal Editor for a project, preventing duplicate instances. Supports --restart to kill and relaunch. TRIGGER when user asks to run, launch, open, start, or restart Unreal Editor / UE editor. DO NOT TRIGGER for building, compiling, packaging, or automating inside a running editor.
---

# UE Runner

Launch Unreal Editor for a project. Auto-detects the `.uproject` file and engine installation. Prevents launching a second editor instance for the same project.

## Instructions

1. Run the launch script:

   ```bash
   bash ~/.claude/skills/ue-runner/scripts/run-editor.sh
   ```

   With explicit project path:
   ```bash
   bash ~/.claude/skills/ue-runner/scripts/run-editor.sh --project /path/to/Game.uproject
   ```

   To restart (kill running editor and relaunch):
   ```bash
   bash ~/.claude/skills/ue-runner/scripts/run-editor.sh --restart
   ```

   On native Windows (cmd.exe / PowerShell), use the batch script instead:
   ```cmd
   %USERPROFILE%\.claude\skills\ue-runner\scripts\run-editor.bat
   ```

2. The script will:
   - Find the `.uproject` file (in cwd or parent directories)
   - Parse `EngineAssociation` to determine the engine version
   - Search common install paths for the matching engine
   - Check if the editor is already running for this project
   - With `--restart`: kill the running editor, wait for exit, then relaunch
   - Without `--restart`: skip launch if already running

3. Report the result to the user.

## Environment

| Variable | Default | Description |
|----------|---------|-------------|
| `UE_ROOT` | *(auto-detect)* | Unreal Engine root directory |
| `UE_PROJECT` | *(auto-detect)* | Path to `.uproject` file |

## Error Recovery

- **"No .uproject found"**: Provide `--project <path>` or cd into the project directory.
- **"Could not determine EngineAssociation"**: Check `.uproject` contains `"EngineAssociation": "X.Y"`.
- **"Unreal Engine root not found"**: Set `UE_ROOT`, e.g. `export UE_ROOT="/Users/Shared/EpicGames/UE_5.6"`
- **"Editor is already running"**: Use `--restart` to kill and relaunch.

## Guidelines

- The script runs the editor as a background process and returns immediately.
- Duplicate detection is per-project — different projects can open simultaneously.
- The `--restart` flag gracefully kills the running editor before relaunching.
- **After rebuilding AgentBridge or any editor plugin C++ code**, always use `--restart` to pick up changes.
