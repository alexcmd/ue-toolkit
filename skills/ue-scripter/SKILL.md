---
name: ue-scripter
description: Execute Python commands in a running Unreal Editor via the AgentBridge HTTP plugin. TRIGGER when user asks to execute Python in UE editor, check health, or control PIE (play/stop/simulate). DO NOT TRIGGER for deciding what to automate (use ue-editor), C++ code (use ue-coder), or building (use ue-builder).
---

# UE Scripter

Execute Python scripts in a running Unreal Editor via the AgentBridge HTTP plugin.

**IMPORTANT**: Always use `ue-exec.sh` — never raw `curl`. The script handles port auto-detection, JSON encoding, and error parsing. All AgentBridge endpoints are under the `/agent/` prefix (e.g., `/agent/health`, `/agent/execute`). There is NO `/health` or `/api/v1/...` endpoint.

## Quick Start

```bash
# Check editor connectivity
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --health

# Inline script (preferred for short scripts)
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --script 'import unreal; print("Hello UE")'

# Script from file
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --file /path/to/script.py

# Batch execution (multiple scripts, one round-trip)
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --batch /tmp/batch.json
bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh --batch /tmp/batch.json --stop-on-error
```

## Response Format

JSON with fields: `success` (bool), `output` (stdout), `log` (array of `{severity, message}`), `result` (return value, usually `None`).
If `success` is false, `result` contains the Python traceback — fix the script and retry.

## Output Filtering

```bash
--max-lines 5              # Truncate output to N lines
--output-field status       # Extract single JSON field (no jq needed)
--jq '.status'              # Pipe through jq (falls back to raw if unavailable)
```
Flags combine: `--output-field output --max-lines 20`

## PIE Control

| Script | Purpose | Command |
|--------|---------|---------|
| `run-game.py` | Play In Editor — **default for "run game"** | `--file ~/.claude/skills/ue-scripter/scripts/run-game.py` |
| `stop-game.py` | Stop active play session | `--file ~/.claude/skills/ue-scripter/scripts/stop-game.py` |
| `simulate.py` | Simulate In Editor (only when explicitly asked) | `--file ~/.claude/skills/ue-scripter/scripts/simulate.py` |

## Environment

| Variable | Default | Description |
|----------|---------|-------------|
| `AGENT_BRIDGE_HOST` | `localhost` | Editor HTTP server host |
| `AGENT_BRIDGE_PORT` | *(auto-detected)* | Auto-detected from `Saved/AgentBridge.port` file. Errors if file is missing (editor must be running). |

**Port auto-detection**: The script reads `Saved/AgentBridge.port` (written by AgentBridge on startup) to find the actual port. Never hardcode port `13090` — always let auto-detection work or read the port file explicitly: `PORT=$(cat Saved/AgentBridge.port)`

## Error Recovery

- **"Cannot reach editor"**: Editor not running or AgentBridge plugin not loaded. Verify `Saved/AgentBridge.port` exists.
- **"Connection refused"**: Port may have changed. Check `Saved/AgentBridge.port` for the actual port, or set `AGENT_BRIDGE_PORT` explicitly.
- **"Python scripting plugin is not available"**: Enable PythonScriptPlugin in `.uproject`.
- **Python traceback in result**: Fix the script. Common: `ModuleNotFoundError`, `AttributeError` (use `unreal.get_editor_subsystem()` not deprecated `unreal.EditorLevelLibrary`).

## Important Caveats

- **Never call `unreal.EditorAssetLibrary.delete_asset()`** from an AgentBridge script — it opens a modal confirmation dialog that blocks the game thread and freezes the entire editor.
- **Always save assets after creating or modifying them**: `unreal.EditorAssetLibrary.save_asset('/Game/Path/Asset')`. Unsaved assets lose references (e.g., textures show checkerboard) on editor restart.
- **HTTP handlers run on the game thread** — do not use `AsyncTask(GameThread)` or `RunOnGameThread()` inside AgentBridge handlers; this causes deadlocks.
