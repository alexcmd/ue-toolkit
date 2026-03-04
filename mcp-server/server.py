"""MCP server wrapping Unreal Engine AgentBridge HTTP endpoints.

Provides native tool calls for Claude Code, replacing shell script indirection.
Tools: 9 AgentBridge endpoints + 3 API search + 2 convenience composites.
"""

import json as _json
import os
from pathlib import Path
from typing import Annotated, List, Literal, Optional

import httpx
from mcp.server.fastmcp import FastMCP
from pydantic import Field

from api_index import ApiIndex

# ── Port discovery (mirrors ue-exec.sh logic) ──────────────────────────────

def _discover_port() -> int:
    """Find AgentBridge port: env var → AgentBridge.port file. No fallback."""
    env_port = os.environ.get("AGENT_BRIDGE_PORT")
    if env_port and env_port.isdigit():
        return int(env_port)

    search = Path.cwd()
    while search != search.parent:
        port_file = search / "Saved" / "AgentBridge.port"
        if port_file.is_file():
            text = port_file.read_text().strip()
            if text.isdigit():
                return int(text)
        if list(search.glob("*.uproject")):
            break
        search = search.parent

    raise RuntimeError(
        "Cannot detect AgentBridge port. Saved/AgentBridge.port not found. "
        "Make sure the Unreal Editor is running with AgentBridge plugin enabled."
    )


def _base_url() -> str:
    host = os.environ.get("AGENT_BRIDGE_HOST", "localhost")
    port = _discover_port()
    return f"http://{host}:{port}"


# ── MCP server setup ───────────────────────────────────────────────────────

mcp = FastMCP(
    "ue-bridge",
    description="Unreal Engine AgentBridge — execute Python in UE editor, query API reference",
)

# Shared HTTP client
_client: Optional[httpx.AsyncClient] = None


async def _http() -> httpx.AsyncClient:
    global _client
    if _client is None:
        _client = httpx.AsyncClient(base_url=_base_url(), timeout=60.0)
    return _client


# ── Output capping ────────────────────────────────────────────────────────

_MAX_RESPONSE_CHARS = 50_000  # Safety cap for any tool response (~12K tokens)
_DEFAULT_MAX_OUTPUT = 10_000  # Default cap for script output field


def _cap_string(s: str, limit: int) -> str:
    """Truncate a string keeping head and tail, with omission notice in the middle."""
    if len(s) <= limit:
        return s
    keep = limit - 60  # room for the notice
    head = keep // 2
    tail = keep - head
    omitted = len(s) - keep
    return f"{s[:head]}\n[... {omitted} chars omitted ...]\n{s[-tail:]}"


def _cap_json_response(text: str, limit: int) -> str:
    """Cap a JSON response by truncating its longest string fields."""
    if len(text) <= limit:
        return text
    original_len = len(text)
    try:
        data = _json.loads(text)
        if not isinstance(data, dict):
            return _cap_string(text, limit)
        data["_truncated"] = True
        data["_original_chars"] = original_len
        # Iteratively shrink the longest string field until under budget
        for _ in range(5):
            serialized = _json.dumps(data)
            if len(serialized) <= limit:
                return serialized
            longest_key = max(
                (k for k, v in data.items() if isinstance(v, str) and k[0] != "_"),
                key=lambda k: len(data[k]),
                default=None,
            )
            if longest_key is None or len(data[longest_key]) < 200:
                break
            excess = len(serialized) - limit
            data[longest_key] = _cap_string(
                data[longest_key], max(len(data[longest_key]) - excess - 100, 200)
            )
        return _json.dumps(data)
    except (ValueError, TypeError):
        return _cap_string(text, limit)


def _cap_execute_output(response: str, max_output: int) -> str:
    """Cap the 'output' field in an execute response to stay within budget."""
    try:
        data = _json.loads(response)
        if not isinstance(data, dict) or "output" not in data:
            return response
        output = data["output"]
        if not isinstance(output, str) or len(output) <= max_output:
            return response
        data["_output_truncated"] = True
        data["_original_output_chars"] = len(output)
        data["output"] = _cap_string(output, max_output)
        return _json.dumps(data)
    except (ValueError, TypeError, KeyError):
        return response


def _cap_batch_results(response: str, max_output: int) -> str:
    """Cap the 'output' field in each result of a batch response."""
    try:
        data = _json.loads(response)
        if not isinstance(data, dict) or "results" not in data:
            return response
        changed = False
        for item in data["results"]:
            if not isinstance(item, dict) or "output" not in item:
                continue
            output = item["output"]
            if isinstance(output, str) and len(output) > max_output:
                item["_output_truncated"] = True
                item["_original_output_chars"] = len(output)
                item["output"] = _cap_string(output, max_output)
                changed = True
        return _json.dumps(data) if changed else response
    except (ValueError, TypeError, KeyError):
        return response


# ── HTTP helper ───────────────────────────────────────────────────────────

async def _call(
    method: str, path: str, *, timeout: float = 30.0, **kwargs
) -> str:
    """Make HTTP request to AgentBridge and return validated JSON string.

    Centralizes error handling, response validation, and output capping so
    individual tools never pass through malformed or oversized JSON.
    """
    client = await _http()
    try:
        if method == "GET":
            r = await client.get(path, timeout=timeout, **kwargs)
        else:
            r = await client.post(path, timeout=timeout, **kwargs)
        r.raise_for_status()
        r.json()  # validate JSON; raises on malformed response
        text = r.text
        if len(text) > _MAX_RESPONSE_CHARS:
            text = _cap_json_response(text, _MAX_RESPONSE_CHARS)
        return text
    except httpx.ConnectError:
        return _json.dumps({"success": False, "error": "Cannot reach editor — is it running with AgentBridge enabled?"})
    except httpx.HTTPStatusError as e:
        body = e.response.text[:300] if e.response.text else ""
        return _json.dumps({"success": False, "error": f"HTTP {e.response.status_code}", "detail": body})
    except _json.JSONDecodeError:
        return _json.dumps({"success": False, "error": "Invalid JSON from editor", "raw": r.text[:500]})
    except httpx.TimeoutException:
        return _json.dumps({"success": False, "error": f"Request timed out after {timeout}s"})
    except Exception as e:
        return _json.dumps({"success": False, "error": str(e)})


# ── AgentBridge tools ──────────────────────────────────────────────────────

@mcp.tool()
async def ue_health() -> str:
    """Check if the Unreal Editor is running and reachable via the AgentBridge HTTP plugin.
    Use this as the first step before any editor interaction to confirm connectivity.
    Returns a JSON object with editor status, project name, and engine version.
    If the editor is not running or the plugin is not enabled, returns an error message."""
    return await _call("GET", "/agent/health", timeout=5.0)


@mcp.tool()
async def ue_execute(
    script: Annotated[str, Field(description="Python code to execute in the editor")],
    timeout: Annotated[int, Field(default=30, ge=1, le=300, description="Max seconds to wait")] = 30,
    max_output_chars: Annotated[int, Field(
        default=_DEFAULT_MAX_OUTPUT, ge=100, le=100_000,
        description="Max characters for the output field before truncation (default 10000)",
    )] = _DEFAULT_MAX_OUTPUT,
) -> str:
    """Execute a Python script in the running Unreal Editor via the AgentBridge plugin.
    The script runs on the editor's game thread with full access to the `unreal` module.
    Use `print()` to return data — it is the only way output reaches the caller.
    Scripts should start with `import unreal`; access subsystems via `unreal.get_editor_subsystem(SubsystemClass)`.
    Timeout defaults to 30s; increase for heavy operations like asset processing or level streaming.
    Large script output is truncated to max_output_chars (default 10000) to keep context manageable.
    """
    result = await _call("POST", "/agent/execute", json={"script": script}, timeout=float(timeout) + 5)
    return _cap_execute_output(result, max_output_chars)


@mcp.tool()
async def ue_batch(
    scripts: Annotated[List[dict], Field(description="List of {id, script} objects to execute sequentially")],
    stop_on_error: Annotated[bool, Field(default=False, description="Stop on first failure")] = False,
    start_from: Annotated[int, Field(default=0, ge=0, description="Resume from this script index (0-based), skipping earlier scripts")] = 0,
    max_output_chars: Annotated[int, Field(
        default=_DEFAULT_MAX_OUTPUT, ge=100, le=100_000,
        description="Max characters per script output before truncation (default 10000)",
    )] = _DEFAULT_MAX_OUTPUT,
) -> str:
    """Execute multiple Python scripts sequentially in a single call to the editor.
    Use for multi-step operations that need to run in order (e.g., create asset, then configure it).
    Each script gets its own execution context. Falls back to sequential single-script calls
    if the batch endpoint is unavailable. Returns combined results with per-script success status.

    On failure with stop_on_error=True, the response includes `last_successful_index` — pass this
    value + 1 as `start_from` in a subsequent call to resume from the checkpoint.
    Large per-script output is truncated to max_output_chars to keep context manageable.
    """
    result = await _call(
        "POST", "/agent/batch",
        json={"scripts": scripts, "stop_on_error": stop_on_error, "start_from": start_from},
        timeout=120.0,
    )
    # If batch endpoint returned a valid response, cap per-result output
    try:
        parsed = _json.loads(result)
        if "error" not in parsed or parsed.get("success") is not None:
            return _cap_batch_results(result, max_output_chars)
    except _json.JSONDecodeError:
        pass

    # Fallback: sequential execution via single-script endpoint
    results = []
    all_ok = True
    last_successful_index = -1
    for i, item in enumerate(scripts):
        sid = item.get("id", str(i))
        code = item.get("script", "")
        # Skip scripts before the checkpoint
        if i < start_from:
            results.append({"id": sid, "success": True, "skipped": True})
            last_successful_index = i
            continue
        single = await _call("POST", "/agent/execute", json={"script": code}, timeout=35.0)
        single = _cap_execute_output(single, max_output_chars)
        try:
            parsed = _json.loads(single)
            results.append({"id": sid, **parsed})
            if parsed.get("success"):
                last_successful_index = i
            else:
                all_ok = False
                if stop_on_error:
                    break
        except _json.JSONDecodeError:
            results.append({"id": sid, "success": False, "error": "Malformed response"})
            all_ok = False
            if stop_on_error:
                break
    return _json.dumps({
        "success": all_ok,
        "last_successful_index": last_successful_index,
        "total": len(scripts),
        "results": results,
    })


@mcp.tool()
async def ue_play(
    mode: Annotated[Literal["pie", "simulate", "stop"], Field(description="Play mode")] = "pie",
) -> str:
    """Control Play In Editor sessions: start playing, simulate, or stop.
    Use 'pie' to play the game in the viewport, 'simulate' for Simulate In Editor,
    or 'stop' to end the current session. Only one PIE/SIE session can be active at a time.
    """
    return await _call("POST", "/agent/play", json={"mode": mode}, timeout=15.0)


@mcp.tool()
async def ue_build(
    wait: Annotated[bool, Field(default=True, description="Wait for build to complete")] = True,
) -> str:
    """Trigger a Live Coding (hot reload) compile in the running editor.
    This recompiles C++ changes without restarting the editor. Use after editing C++ source files.
    Set wait=True (default) to block until compilation finishes and get the result.
    For full project builds or packaging, use the ue-builder skill instead.
    """
    timeout = 120.0 if wait else 10.0
    return await _call("POST", "/agent/build", json={"wait": wait}, timeout=timeout)


@mcp.tool()
async def ue_logs(
    lines: Annotated[int, Field(default=50, ge=1, le=2000, description="Number of lines to return")] = 50,
    filter: Annotated[str, Field(default="", description="Text filter for log lines")] = "",
    severity: Annotated[
        Literal["", "error", "warning", "display", "log", "verbose"],
        Field(default="", description="Filter by severity level"),
    ] = "",
) -> str:
    """Read recent log lines from the Unreal Editor output log.
    Useful for diagnosing errors after script execution, checking warnings, or monitoring editor state.
    Supports text filtering and severity filtering. Returns newest lines first.
    """
    params: dict = {"lines": lines}
    if filter:
        params["filter"] = filter
    if severity:
        params["severity"] = severity
    return await _call("GET", "/agent/logs", params=params, timeout=10.0)


@mcp.tool()
async def ue_devices(
    platform: Annotated[str, Field(default="", description="Filter by platform (e.g., 'Windows', 'Android')")] = "",
) -> str:
    """List available target devices for deployment and testing.
    Returns device names, IDs, and connection status. Filter by platform to narrow results.
    Use before ue_launch to identify the correct device_id for deployment.
    """
    params = {}
    if platform:
        params["platform"] = platform
    return await _call("GET", "/agent/devices", params=params, timeout=10.0)


@mcp.tool()
async def ue_configs() -> str:
    """List available build configurations for the current project.
    Returns configurations like Development, Shipping, Debug, etc. with their settings.
    Use to discover valid configuration names before launching or packaging."""
    return await _call("GET", "/agent/configs", timeout=10.0)


@mcp.tool()
async def ue_launch(
    platform: Annotated[str, Field(description="Target platform (e.g., 'Windows', 'Android')")],
    device_id: Annotated[str, Field(default="", description="Specific device ID from ue_devices")] = "",
) -> str:
    """Launch the project on a target device for testing.
    Initiates a cook-and-launch cycle for the specified platform. This may take several minutes.
    Use ue_devices first to discover available device IDs. The timeout is 120s for the launch request.
    """
    payload: dict = {"platform": platform}
    if device_id:
        payload["device_id"] = device_id
    return await _call("POST", "/agent/launch", json=payload, timeout=120.0)


# ── Convenience composites ─────────────────────────────────────────────────

_SCREENSHOT_SCRIPT = """\
import unreal, os
width = {width}
height = {height}
out_path = os.path.join(unreal.Paths.project_saved_dir(), "Screenshots", "viewport.png")
os.makedirs(os.path.dirname(out_path), exist_ok=True)
unreal.AutomationLibrary.take_high_res_screenshot(width, height, out_path)
print(out_path)
"""

_SCENE_TREE_SCRIPT = """\
import unreal
actor_subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_subsys.get_all_level_actors()
children_map = {}
root_actors = []
for a in actors:
    parent = a.get_attach_parent_actor()
    if parent:
        parent_name = parent.get_actor_label()
        children_map.setdefault(parent_name, []).append(a)
    else:
        root_actors.append(a)

def print_actor(actor, indent=0):
    prefix = "  " * indent
    label = actor.get_actor_label()
    cls = actor.get_class().get_name()
    loc = actor.get_actor_location()
    print(f"{prefix}{label} [{cls}] ({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f})")
    for child in children_map.get(label, []):
        print_actor(child, indent + 1)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
print(f"Level: {world.get_name()}")
print(f"Actors: {len(actors)}")
print("---")
for a in sorted(root_actors, key=lambda x: x.get_actor_label()):
    print_actor(a)
"""


@mcp.tool()
async def ue_screenshot(
    width: Annotated[int, Field(default=800, ge=64, le=3840, description="Image width in pixels")] = 800,
    height: Annotated[int, Field(default=600, ge=64, le=2160, description="Image height in pixels")] = 600,
) -> str:
    """Capture a screenshot of the active editor viewport and return the saved file path.
    The image is saved to the project's Saved/Screenshots/ directory as viewport.png.
    Use to verify visual changes after modifying actors, materials, or lighting.
    """
    script = _SCREENSHOT_SCRIPT.format(width=width, height=height)
    return await ue_execute(script, timeout=15)


@mcp.tool()
async def ue_scene_tree() -> str:
    """Get the full scene hierarchy of the current level: actor labels, class types, and world locations.
    Returns a tree view with parent-child relationships, useful for understanding level layout.
    Use to discover actor names before writing scripts that modify specific actors."""
    return await ue_execute(_SCENE_TREE_SCRIPT, timeout=15)


# ── API search tools (loaded from knowledge files) ────────────────────────

_api_index: Optional[ApiIndex] = None


def _get_api_index() -> ApiIndex:
    global _api_index
    if _api_index is None:
        # Knowledge files live in ue-toolkit/skills/ue-api-*/knowledge/
        server_dir = Path(__file__).resolve().parent
        toolkit_dir = server_dir.parent
        skills_dir = toolkit_dir / "skills"
        _api_index = ApiIndex(skills_dir)
    return _api_index


@mcp.tool()
async def ue_api_search(
    query: Annotated[str, Field(description="Search term (type name, module name, or keyword)")],
    limit: Annotated[int, Field(default=10, ge=1, le=100, description="Max results to return")] = 10,
) -> str:
    """Search the UE Python API knowledge base (15K+ types across 896 modules) by type name, module, or keyword.
    Returns ranked summaries with type name, module, kind, and member counts.
    Use this to discover available types before writing Python scripts. Supports partial and
    case-insensitive matching. Follow up with ue_api_type for full details on a specific type.
    """
    index = _get_api_index()
    results = index.search(query, limit=limit)
    return _json.dumps(results, indent=2)


@mcp.tool()
async def ue_api_type(
    name: Annotated[str, Field(description="Exact or partial type name (e.g., 'EditorActorSubsystem')")],
) -> str:
    """Get full API reference for a specific UE Python type including properties, methods, and inheritance.
    Returns the complete type definition from the knowledge base. Use after ue_api_search to drill
    into a specific type. Supports case-insensitive lookup and falls back to fuzzy suggestions
    if the exact name is not found.
    """
    index = _get_api_index()
    result = index.get_type(name)
    if result is None:
        matches = index.search(name, limit=5)
        return _json.dumps({"error": f"Type '{name}' not found", "suggestions": matches}, indent=2)
    return _json.dumps(result, indent=2)


@mcp.tool()
async def ue_api_module(
    module: Annotated[str, Field(description="Module name (e.g., 'Engine', 'EnhancedInput')")],
) -> str:
    """List all types in a specific UE module with summaries.
    Returns every type defined in the module with name, kind, inheritance, and member counts.
    Use to explore what a module provides. If the module is not found, returns a list of
    available modules for discovery. Module names are case-insensitive.
    """
    index = _get_api_index()
    result = index.get_module(module)
    if result is None:
        modules = index.list_modules()
        return _json.dumps({"error": f"Module '{module}' not found", "available_modules": modules[:50]}, indent=2)
    return _json.dumps(result, indent=2)


# ── Entry point ────────────────────────────────────────────────────────────
# This server uses HTTP+SSE transport. Clients connect via GET /sse for the
# event stream and POST /messages/ to send requests.

MCP_PORT = int(os.environ.get("UE_MCP_PORT", "13091"))

if __name__ == "__main__":
    mcp.run(transport="sse", host="127.0.0.1", port=MCP_PORT)
