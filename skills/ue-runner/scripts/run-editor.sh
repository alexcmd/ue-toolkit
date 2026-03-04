#!/usr/bin/env bash
# Launch Unreal Editor for a project, preventing duplicate instances.
# Usage: ./run-editor.sh [--project <path.uproject>]
#
# Auto-detects the .uproject file, engine version, and engine installation.
# Works on macOS, Linux, and Windows (Git Bash / MSYS / Cygwin).

set -euo pipefail

# ─── defaults ────────────────────────────────────────────────────────
PROJECT=""
RESTART=false

# ─── parse args ──────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
  case "$1" in
    --project)  PROJECT="$2"; shift 2 ;;
    --restart)  RESTART=true; shift ;;
    -h|--help)
      echo "Usage: $0 [--project <path.uproject>] [--restart]"
      echo ""
      echo "Options:"
      echo "  --project PATH   Path to .uproject file (auto-detected if omitted)"
      echo "  --restart        Kill running editor for this project and relaunch"
      echo ""
      echo "Environment:"
      echo "  UE_ROOT          Unreal Engine root directory (auto-detected)"
      echo "  UE_PROJECT       Alternative to --project"
      exit 0
      ;;
    *) echo "Unknown argument: $1"; exit 1 ;;
  esac
done

# ─── find .uproject ─────────────────────────────────────────────────
find_uproject() {
  local dir="$1"
  while [[ "$dir" != "/" ]]; do
    local found
    found=$(find "$dir" -maxdepth 1 -name "*.uproject" -print -quit 2>/dev/null || true)
    if [[ -n "$found" ]]; then
      echo "$found"
      return 0
    fi
    dir=$(dirname "$dir")
  done
  return 1
}

if [[ -z "$PROJECT" ]]; then
  PROJECT="${UE_PROJECT:-}"
fi

if [[ -z "$PROJECT" ]]; then
  PROJECT=$(find_uproject "$(pwd)") || true
fi

if [[ -z "$PROJECT" || ! -f "$PROJECT" ]]; then
  echo "ERROR: No .uproject file found."
  echo "  Provide --project <path>, set UE_PROJECT, or run from within a UE project directory."
  exit 1
fi

PROJECT=$(cd "$(dirname "$PROJECT")" && pwd)/$(basename "$PROJECT")
PROJECT_NAME=$(basename "$PROJECT" .uproject)

# ─── check if already running ───────────────────────────────────────
if pgrep -f "UnrealEditor.*${PROJECT_NAME}" > /dev/null 2>&1; then
  if [[ "$RESTART" == true ]]; then
    echo "Stopping running Unreal Editor for ${PROJECT_NAME}..."
    pkill -f "UnrealEditor.*${PROJECT_NAME}" 2>/dev/null || true
    # Wait for the process to fully exit
    WAIT_SECONDS=0
    while pgrep -f "UnrealEditor.*${PROJECT_NAME}" > /dev/null 2>&1; do
      sleep 1
      WAIT_SECONDS=$((WAIT_SECONDS + 1))
      if [[ $WAIT_SECONDS -ge 30 ]]; then
        echo "WARNING: Editor did not exit after 30s, sending SIGKILL..."
        pkill -9 -f "UnrealEditor.*${PROJECT_NAME}" 2>/dev/null || true
        sleep 2
        break
      fi
    done
    echo "Editor stopped."
  else
    echo "Unreal Editor is already running with ${PROJECT_NAME}."
    exit 0
  fi
fi

# ─── parse EngineAssociation from .uproject ──────────────────────────
ENGINE_VERSION=$(grep -o '"EngineAssociation"\s*:\s*"[^"]*"' "$PROJECT" | grep -o '"[^"]*"$' | tr -d '"')
if [[ -z "$ENGINE_VERSION" ]]; then
  echo "WARNING: Could not determine EngineAssociation from $PROJECT"
  echo "  Will search for any installed engine version."
fi

# ─── detect platform ────────────────────────────────────────────────
HOST_OS="$(uname -s)"

# ─── find UE_ROOT ───────────────────────────────────────────────────
find_ue_root() {
  local candidates=()
  case "$HOST_OS" in
    Darwin)
      if [[ -n "${ENGINE_VERSION:-}" ]]; then
        candidates+=("/Users/Shared/EpicGames/UE_${ENGINE_VERSION}")
      fi
      candidates+=(
        "/Users/Shared/EpicGames"/UE_5.*
        "$HOME/UnrealEngine"
        /opt/UnrealEngine
      )
      ;;
    Linux)
      if [[ -n "${ENGINE_VERSION:-}" ]]; then
        candidates+=("/opt/UnrealEngine/UE_${ENGINE_VERSION}")
        candidates+=("$HOME/UnrealEngine/UE_${ENGINE_VERSION}")
      fi
      candidates+=(
        /opt/UnrealEngine
        "$HOME/UnrealEngine"
        /home/*/UnrealEngine
        /opt/unreal-engine/UE_5.*
      )
      ;;
    MINGW*|MSYS*|CYGWIN*)
      if [[ -n "${ENGINE_VERSION:-}" ]]; then
        candidates+=("C:/Program Files/Epic Games/UE_${ENGINE_VERSION}")
        candidates+=("D:/Program Files/Epic Games/UE_${ENGINE_VERSION}")
      fi
      candidates+=(
        "C:/Program Files/Epic Games"/UE_5.*
        "D:/Program Files/Epic Games"/UE_5.*
        "C:/EpicGames"/UE_5.*
      )
      ;;
  esac

  for candidate in "${candidates[@]}"; do
    if [[ -d "$candidate" ]]; then
      echo "$candidate"
      return 0
    fi
  done
  return 1
}

if [[ -z "${UE_ROOT:-}" ]]; then
  UE_ROOT=$(find_ue_root) || true
fi

if [[ -z "${UE_ROOT:-}" || ! -d "${UE_ROOT:-}" ]]; then
  echo "ERROR: Unreal Engine root not found."
  echo "  Set UE_ROOT to your UE installation directory."
  echo "  Example: export UE_ROOT=\"/Users/Shared/EpicGames/UE_5.6\""
  exit 1
fi

# ─── locate editor binary ───────────────────────────────────────────
EDITOR=""
case "$HOST_OS" in
  Darwin)
    EDITOR="${UE_ROOT}/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
    ;;
  Linux)
    EDITOR="${UE_ROOT}/Engine/Binaries/Linux/UnrealEditor"
    ;;
  MINGW*|MSYS*|CYGWIN*)
    EDITOR="${UE_ROOT}/Engine/Binaries/Win64/UnrealEditor.exe"
    ;;
esac

if [[ -z "$EDITOR" || ! -x "$EDITOR" ]]; then
  echo "ERROR: UnrealEditor binary not found at expected location."
  echo "  UE_ROOT: ${UE_ROOT}"
  echo "  Expected: ${EDITOR:-<unknown>}"
  exit 1
fi

# ─── launch ─────────────────────────────────────────────────────────
echo "Project: ${PROJECT}"
echo "Engine:  ${UE_ROOT}"
echo "Version: ${ENGINE_VERSION:-unknown}"
echo ""
echo "Starting Unreal Editor..."
"$EDITOR" "$PROJECT" &
echo "Editor launched (PID: $!)"
