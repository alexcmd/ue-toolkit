#!/usr/bin/env bash
# Execute Python commands in Unreal Editor via AgentBridge HTTP plugin.
# Usage: ./ue-exec.sh --health
#        ./ue-exec.sh --script 'import unreal; print("hello")'
#        ./ue-exec.sh --file /path/to/script.py

set -euo pipefail

# ─── defaults ────
HOST="${AGENT_BRIDGE_HOST:-localhost}"
PORT="${AGENT_BRIDGE_PORT:-}"
TIMEOUT=30
MODE=""
SCRIPT=""
SCRIPT_FILE=""
BATCH_FILE=""
STOP_ON_ERROR=false
ISOLATED=false
MAX_LINES=""
JQ_FILTER=""
OUTPUT_FIELD=""

# ─── auto-detect port ──
# Searches for AgentBridge.port file in Saved/ directories of .uproject locations
auto_detect_port() {
  # 1. Check AGENT_BRIDGE_PORT env var (already handled above)
  if [[ -n "$PORT" ]]; then
    return 0
  fi

  # 2. Search for AgentBridge.port file near .uproject files
  local search_dir="${PWD}"
  while [[ "$search_dir" != "/" ]]; do
    local port_file="${search_dir}/Saved/AgentBridge.port"
    if [[ -f "$port_file" ]]; then
      PORT=$(cat "$port_file" 2>/dev/null | tr -d '[:space:]')
      if [[ -n "$PORT" && "$PORT" =~ ^[0-9]+$ ]]; then
        return 0
      fi
    fi
    # Check if this dir has a .uproject
    if compgen -G "${search_dir}/*.uproject" > /dev/null 2>&1; then
      break
    fi
    search_dir=$(dirname "$search_dir")
  done

  # 3. No port file found — error out
  echo "ERROR: Cannot detect AgentBridge port. Saved/AgentBridge.port not found." >&2
  echo "  Make sure the Unreal Editor is running with AgentBridge plugin enabled." >&2
  exit 1
}

auto_detect_port
BASE_URL="http://${HOST}:${PORT}"

# ─── parse args ──
while [[ $# -gt 0 ]]; do
  case "$1" in
    --health)    MODE="health";  shift ;;
    --script)    MODE="exec";    SCRIPT="$2";      shift 2 ;;
    --file)      MODE="file";    SCRIPT_FILE="$2";  shift 2 ;;
    --host)      HOST="$2";      BASE_URL="http://${HOST}:${PORT}"; shift 2 ;;
    --port)      PORT="$2";      BASE_URL="http://${HOST}:${PORT}"; shift 2 ;;
    --timeout)       TIMEOUT="$2";      shift 2 ;;
    --batch)         MODE="batch";      BATCH_FILE="$2";   shift 2 ;;
    --stop-on-error) STOP_ON_ERROR=true; shift ;;
    --isolated)      ISOLATED=true;     shift ;;
    --max-lines)     MAX_LINES="$2";    shift 2 ;;
    --jq)            JQ_FILTER="$2";    shift 2 ;;
    --output-field)  OUTPUT_FIELD="$2"; shift 2 ;;
    -h|--help)
      echo "Usage: $0 [options]"
      echo ""
      echo "Modes:"
      echo "  --health              Check if editor is reachable"
      echo "  --script 'code'       Execute inline Python script"
      echo "  --file /path/to.py    Execute Python script from file"
      echo "  --batch FILE          Execute batch of scripts from JSON file"
      echo ""
      echo "Options:"
      echo "  --host HOST           Editor host (default: localhost)"
      echo "  --port PORT           Editor port (default: auto-detected from Saved/AgentBridge.port)"
      echo "  --timeout SECONDS     Request timeout (default: 30)"
      echo "  --stop-on-error       Stop batch on first error"
      echo "  --isolated            Run in private scope (fresh dict, no __main__ pollution)."
      echo "                        REQUIRED for scripts that call new_level() or load maps."
      echo ""
      echo "Output filtering:"
      echo "  --max-lines N         Truncate output to first N lines"
      echo "  --jq FILTER           Pipe JSON response through jq (falls back to raw)"
      echo "  --output-field FIELD  Extract a top-level JSON field (no jq needed)"
      echo ""
      echo "Environment:"
      echo "  AGENT_BRIDGE_HOST    default localhost"
      echo "  AGENT_BRIDGE_PORT    auto-detected from Saved/AgentBridge.port (no fallback)"
      exit 0
      ;;
    *) echo "Unknown argument: $1"; exit 1 ;;
  esac
done

if [[ -z "$MODE" ]]; then
  echo "ERROR: Specify --health, --script, --file, or --batch"
  echo "Run $0 --help for usage"
  exit 1
fi

# ─── output filtering helper ──
apply_output_filters() {
  local raw="$1"

  # Apply --output-field (extract a top-level JSON field via python3)
  if [[ -n "$OUTPUT_FIELD" ]]; then
    local extracted
    extracted=$(echo "$raw" | python3 -c "
import json, sys
try:
    data = json.load(sys.stdin)
    val = data.get('$OUTPUT_FIELD')
    if isinstance(val, (dict, list)):
        print(json.dumps(val, indent=2))
    elif val is not None:
        print(val)
except Exception:
    sys.stdout.write(sys.stdin.read())
" 2>/dev/null) && raw="$extracted" || true
  fi

  # Apply --jq (try jq, fall back to python3, fall back to raw)
  if [[ -n "$JQ_FILTER" ]]; then
    local filtered
    if command -v jq &>/dev/null; then
      filtered=$(echo "$raw" | jq -r "$JQ_FILTER" 2>/dev/null) && raw="$filtered" || true
    else
      filtered=$(echo "$raw" | python3 -c "
import json, sys, subprocess
data = json.load(sys.stdin)
# Simple field extraction for common jq patterns like '.field'
filt = '''$JQ_FILTER'''.strip()
if filt.startswith('.') and filt[1:].isidentifier():
    val = data.get(filt[1:])
    if isinstance(val, (dict, list)):
        print(json.dumps(val, indent=2))
    elif val is not None:
        print(val)
else:
    print(json.dumps(data, indent=2))
" 2>/dev/null) && raw="$filtered" || true
    fi
  fi

  # Apply --max-lines (truncate output)
  if [[ -n "$MAX_LINES" ]]; then
    local total
    total=$(echo "$raw" | wc -l)
    if (( total > MAX_LINES )); then
      local truncated
      truncated=$(echo "$raw" | head -n "$MAX_LINES")
      raw="${truncated}
[...truncated $((total - MAX_LINES)) more lines]"
    fi
  fi

  echo "$raw"
}

# ─── health check ──
if [[ "$MODE" == "health" ]]; then
  RESPONSE=$(curl -s --max-time 5 "${BASE_URL}/agent/health" 2>/dev/null) || true
  if [[ -z "$RESPONSE" ]]; then
    echo "ERROR: Cannot reach editor at ${BASE_URL}"
    echo "  Make sure the Unreal Editor is running with AgentBridge plugin enabled."
    # Show port detection info for debugging
    if [[ -n "${AGENT_BRIDGE_PORT:-}" ]]; then
      echo "  Port source: AGENT_BRIDGE_PORT env var (${AGENT_BRIDGE_PORT})"
    else
      # Check if port file exists anywhere nearby
      local_search="${PWD}"
      while [[ "$local_search" != "/" ]]; do
        if [[ -f "${local_search}/Saved/AgentBridge.port" ]]; then
          echo "  Port source: ${local_search}/Saved/AgentBridge.port"
          break
        fi
        if compgen -G "${local_search}/*.uproject" > /dev/null 2>&1; then
          echo "  Port file not found: ${local_search}/Saved/AgentBridge.port"
          break
        fi
        local_search=$(dirname "$local_search")
      done
    fi
    exit 1
  fi
  apply_output_filters "$RESPONSE"
  exit 0
fi

# ─── batch mode ──
if [[ "$MODE" == "batch" ]]; then
  if [[ ! -f "$BATCH_FILE" ]]; then
    echo "ERROR: Batch file not found: ${BATCH_FILE}"
    exit 1
  fi

  # Build batch payload
  BATCH_PAYLOAD=$(python3 -c "
import json, sys
batch = json.load(open('$BATCH_FILE'))
scripts = batch if isinstance(batch, list) else batch.get('scripts', [])
payload = {
    'scripts': scripts,
    'stop_on_error': $([[ "$STOP_ON_ERROR" == "true" ]] && echo "True" || echo "False"),
    'isolated': $([[ "$ISOLATED" == "true" ]] && echo "True" || echo "False"),
}
print(json.dumps(payload))
" 2>/dev/null)

  if [[ -z "$BATCH_PAYLOAD" ]]; then
    echo "ERROR: Failed to parse batch file"
    exit 1
  fi

  # Try /agent/batch endpoint
  HTTP_CODE=$(curl -s -o /tmp/ue-exec-batch-resp.json -w '%{http_code}' \
    --max-time "$TIMEOUT" \
    -X POST "${BASE_URL}/agent/batch" \
    -H "Content-Type: application/json" \
    -d "$BATCH_PAYLOAD" 2>/dev/null) || HTTP_CODE="000"

  if [[ "$HTTP_CODE" == "200" ]]; then
    RESPONSE=$(cat /tmp/ue-exec-batch-resp.json)
    apply_output_filters "$RESPONSE"
    SUCCESS=$(echo "$RESPONSE" | python3 -c "import json,sys; print(json.load(sys.stdin).get('success', False))" 2>/dev/null || echo "False")
    [[ "$SUCCESS" != "True" ]] && exit 1
    exit 0
  fi

  # Fallback: sequential /agent/execute calls
  RESPONSE=$(python3 -c "
import json, sys, subprocess

batch = json.load(open('$BATCH_FILE'))
scripts = batch if isinstance(batch, list) else batch.get('scripts', [])
stop_on_error = $([[ "$STOP_ON_ERROR" == "true" ]] && echo "True" || echo "False")

results = []
all_success = True
skip_rest = False

for item in scripts:
    sid = item.get('id', str(len(results)))
    script = item.get('script', '')

    if skip_rest:
        results.append({'id': sid, 'success': False, 'output': '', 'result': '', 'skipped': True})
        continue

    payload = json.dumps({'script': script})
    try:
        proc = subprocess.run(
            ['curl', '-s', '--max-time', '$TIMEOUT',
             '-X', 'POST', '${BASE_URL}/agent/execute',
             '-H', 'Content-Type: application/json',
             '-d', payload],
            capture_output=True, text=True, timeout=int('$TIMEOUT') + 5
        )
        resp = json.loads(proc.stdout) if proc.stdout else {}
    except Exception as e:
        resp = {'success': False, 'output': '', 'result': str(e)}

    success = resp.get('success', False)
    results.append({
        'id': sid,
        'success': success,
        'output': resp.get('output', ''),
        'result': resp.get('result', '')
    })

    if not success:
        all_success = False
        if stop_on_error:
            skip_rest = True

print(json.dumps({'success': all_success, 'results': results}, indent=2))
" 2>/dev/null)

  if [[ -z "$RESPONSE" ]]; then
    echo "ERROR: Batch fallback failed"
    exit 1
  fi

  apply_output_filters "$RESPONSE"
  SUCCESS=$(echo "$RESPONSE" | python3 -c "import json,sys; print(json.load(sys.stdin).get('success', False))" 2>/dev/null || echo "False")
  [[ "$SUCCESS" != "True" ]] && exit 1
  exit 0
fi

# ─── load script from file ──
if [[ "$MODE" == "file" ]]; then
  if [[ ! -f "$SCRIPT_FILE" ]]; then
    echo "ERROR: File not found: ${SCRIPT_FILE}"
    exit 1
  fi
  SCRIPT=$(cat "$SCRIPT_FILE")
fi

if [[ -z "$SCRIPT" ]]; then
  echo "ERROR: Empty script"
  exit 1
fi

# ─── build JSON payload ──
# Use python3 for safe JSON encoding of arbitrary script content
PAYLOAD=$(python3 -c "
import json, sys
payload = {'script': sys.stdin.read()}
if '$ISOLATED' == 'true':
    payload['isolated'] = True
print(json.dumps(payload))
" <<< "$SCRIPT")

# ─── execute ──
RESPONSE=$(curl -s --max-time "$TIMEOUT" \
  -X POST "${BASE_URL}/agent/execute" \
  -H "Content-Type: application/json" \
  -d "$PAYLOAD" 2>/dev/null) || true

if [[ -z "$RESPONSE" ]]; then
  echo "ERROR: No response from editor at ${BASE_URL}"
  echo "  Editor may have frozen or the request timed out (${TIMEOUT}s)."
  exit 1
fi

apply_output_filters "$RESPONSE"

# ─── check success ──
SUCCESS=$(echo "$RESPONSE" | python3 -c "import json,sys; print(json.load(sys.stdin).get('success', False))" 2>/dev/null || echo "False")
if [[ "$SUCCESS" != "True" ]]; then
  exit 1
fi
