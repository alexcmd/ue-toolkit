#!/usr/bin/env bash
# Build (compile) an Unreal Engine project using UnrealBuildTool.
# Usage: ./ue-build.sh --project <path.uproject> [--platform Win64] [--config Development] [--target Editor] [--extra-args "..."]

set -euo pipefail

# ─── defaults ────────────────────────────────────────────────────────
PLATFORM=""
CONFIG="Development"
TARGET="Editor"
PROJECT=""
EXTRA_ARGS=""
TIMEOUT=1800  # 30 minutes

# ─── parse args ──────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
  case "$1" in
    --project)   PROJECT="$2";     shift 2 ;;
    --platform)  PLATFORM="$2";    shift 2 ;;
    --config)    CONFIG="$2";      shift 2 ;;
    --target)    TARGET="$2";      shift 2 ;;
    --extra)     EXTRA_ARGS="$2";  shift 2 ;;
    --timeout)   TIMEOUT="$2";     shift 2 ;;
    -h|--help)
      echo "Usage: $0 --project <path.uproject> [options]"
      echo ""
      echo "Options:"
      echo "  --project PATH   Path to .uproject file (auto-detected if omitted)"
      echo "  --platform NAME  Win64 | Linux | LinuxArm64 | Mac (auto-detected)"
      echo "  --config NAME    Debug | DebugGame | Development | Test | Shipping"
      echo "  --target NAME    Game | Editor | Server | Client (default: Editor)"
      echo "  --extra ARGS     Additional UBT arguments"
      echo "  --timeout SECS   Build timeout in seconds (default: 1800)"
      echo ""
      echo "Environment:"
      echo "  UE_ROOT          Unreal Engine root directory (auto-detected)"
      echo "  UE_PROJECT       Alternative to --project"
      exit 0
      ;;
    *) echo "Unknown argument: $1"; exit 1 ;;
  esac
done

# ─── auto-detect platform ───────────────────────────────────────────
detect_platform() {
  case "$(uname -s)" in
    Darwin)  echo "Mac" ;;
    Linux)
      case "$(uname -m)" in
        aarch64|arm64) echo "LinuxArm64" ;;
        *)             echo "Linux" ;;
      esac
      ;;
    MINGW*|MSYS*|CYGWIN*) echo "Win64" ;;
    *) echo "Win64" ;;
  esac
}

if [[ -z "$PLATFORM" ]]; then
  PLATFORM=$(detect_platform)
  echo "Auto-detected platform: ${PLATFORM}"
fi

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
  echo "  Provide --project <path> or set UE_PROJECT, or run from within a UE project directory."
  exit 1
fi

PROJECT=$(cd "$(dirname "$PROJECT")" && pwd)/$(basename "$PROJECT")
PROJECT_NAME=$(basename "$PROJECT" .uproject)
echo "Project: ${PROJECT}"
echo "Project name: ${PROJECT_NAME}"

# ─── find UE_ROOT ───────────────────────────────────────────────────
find_ue_root() {
  # Common install locations by platform
  local candidates=()
  case "$(uname -s)" in
    Darwin)
      candidates=(
        "/Users/Shared/Epic Games"/UE_5.*
        "$HOME/UnrealEngine"
        "/opt/UnrealEngine"
      )
      ;;
    Linux)
      candidates=(
        /opt/UnrealEngine
        "$HOME/UnrealEngine"
        /home/*/UnrealEngine
        /opt/unreal-engine/UE_5.*
      )
      ;;
    MINGW*|MSYS*|CYGWIN*)
      candidates=(
        "C:/Program Files/Epic Games"/UE_5.*
        "D:/Program Files/Epic Games"/UE_5.*
        "C:/EpicGames"/UE_5.*
      )
      ;;
  esac

  for candidate in "${candidates[@]}"; do
    if [[ -d "$candidate" && -f "$candidate/Engine/Build/BatchFiles/RunUAT.sh" ]]; then
      echo "$candidate"
      return 0
    fi
    if [[ -d "$candidate" && -f "$candidate/Engine/Build/BatchFiles/RunUAT.bat" ]]; then
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
  echo "  Example: export UE_ROOT=\"/Users/Shared/Epic Games/UE_5.5\""
  exit 1
fi

echo "UE_ROOT: ${UE_ROOT}"

# ─── locate UBT binary ──────────────────────────────────────────────
UBT=""
case "$(uname -s)" in
  Darwin|Linux)
    # UE5 .NET-based UBT
    if [[ -f "${UE_ROOT}/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool" ]]; then
      UBT="${UE_ROOT}/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool"
    elif [[ -f "${UE_ROOT}/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll" ]]; then
      UBT="dotnet ${UE_ROOT}/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll"
    # Legacy UE4 path
    elif [[ -f "${UE_ROOT}/Engine/Binaries/DotNET/UnrealBuildTool.exe" ]]; then
      UBT="mono ${UE_ROOT}/Engine/Binaries/DotNET/UnrealBuildTool.exe"
    fi
    ;;
  MINGW*|MSYS*|CYGWIN*)
    if [[ -f "${UE_ROOT}/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe" ]]; then
      UBT="${UE_ROOT}/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe"
    elif [[ -f "${UE_ROOT}/Engine/Binaries/DotNET/UnrealBuildTool.exe" ]]; then
      UBT="${UE_ROOT}/Engine/Binaries/DotNET/UnrealBuildTool.exe"
    fi
    ;;
esac

if [[ -z "$UBT" ]]; then
  echo "ERROR: UnrealBuildTool not found in ${UE_ROOT}"
  echo "  Expected at: Engine/Binaries/DotNET/UnrealBuildTool/"
  exit 1
fi

echo "UBT: ${UBT}"

# ─── build target name ──────────────────────────────────────────────
# UBT target format: <ProjectName><TargetSuffix>
# Editor target = <ProjectName>Editor, Game target = <ProjectName>, etc.
BUILD_TARGET="${PROJECT_NAME}"
case "$TARGET" in
  Editor)  BUILD_TARGET="${PROJECT_NAME}Editor" ;;
  Game)    BUILD_TARGET="${PROJECT_NAME}" ;;
  Server)  BUILD_TARGET="${PROJECT_NAME}Server" ;;
  Client)  BUILD_TARGET="${PROJECT_NAME}Client" ;;
  *)       BUILD_TARGET="$TARGET" ;;  # allow custom target names
esac

# ─── run UBT ─────────────────────────────────────────────────────────
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  Building: ${BUILD_TARGET} | ${PLATFORM} | ${CONFIG}"
echo "═══════════════════════════════════════════════════════════════"
echo ""

START_TIME=$(date +%s)

# shellcheck disable=SC2086
timeout "$TIMEOUT" $UBT \
  "$BUILD_TARGET" \
  "$PLATFORM" \
  "$CONFIG" \
  -Project="$PROJECT" \
  -WaitMutex \
  -FromMsBuild \
  $EXTRA_ARGS \
  2>&1

BUILD_EXIT=$?
END_TIME=$(date +%s)
ELAPSED=$(( END_TIME - START_TIME ))
MINUTES=$(( ELAPSED / 60 ))
SECONDS_REM=$(( ELAPSED % 60 ))

echo ""
if [[ $BUILD_EXIT -eq 0 ]]; then
  echo "BUILD SUCCEEDED in ${MINUTES}m ${SECONDS_REM}s"
else
  echo "BUILD FAILED (exit code: ${BUILD_EXIT}) after ${MINUTES}m ${SECONDS_REM}s"
  exit $BUILD_EXIT
fi
