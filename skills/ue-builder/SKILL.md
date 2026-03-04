---
name: ue-builder
description: Build, cook, and package Unreal Engine games using UBT and UAT. TRIGGER when user asks to build, compile, cook, package, or clean an Unreal Engine project. DO NOT TRIGGER for general C++ builds or non-UE projects.
---

# UE Builder

Build, cook, package, and clean Unreal Engine projects using UnrealBuildTool (UBT) and AutomationTool (UAT / RunUAT).

## Instructions

1. Determine the operation: **build** (compile), **package** (BuildCookRun), or **clean**.
2. Determine parameters:
   - **project** — path to `.uproject` (auto-detected if in project directory)
   - **platform** — `Win64`, `Linux`, `LinuxArm64`, `Mac` (default: current host)
   - **config** — `Development`, `Shipping`, `DebugGame`, `Debug`, `Test` (default: `Development`)
   - **target** — `Game`, `Editor`, `Server`, `Client` (default: `Editor` for build, `Game` for package)

3. Run the appropriate script:

   **Build (compile):**
   ```bash
   bash ~/.claude/skills/ue-builder/scripts/ue-build.sh \
     --project "/path/to/Game.uproject" \
     --platform Win64 --config Development --target Editor
   ```

   **Package (BuildCookRun):**
   ```bash
   bash ~/.claude/skills/ue-builder/scripts/ue-package.sh \
     --project "/path/to/Game.uproject" \
     --platform Win64 --config Shipping --archive "/path/to/output"
   ```

   **Clean:**
   ```bash
   bash ~/.claude/skills/ue-builder/scripts/ue-clean.sh \
     --project "/path/to/Game.uproject"
   ```

4. Report the result (success/failure, build time, output location).

## Environment

| Variable | Default | Description |
|----------|---------|-------------|
| `UE_ROOT` | *(auto-detect)* | Unreal Engine root directory |
| `UE_PROJECT` | *(auto-detect)* | Path to `.uproject` file |

## Supported Platforms

| Platform | UBT Name | Notes |
|----------|----------|-------|
| Windows 64-bit | `Win64` | Most common target |
| Linux x86_64 | `Linux` | Dedicated servers |
| Linux ARM64 | `LinuxArm64` | DGX Spark / ARM servers |
| macOS | `Mac` | Apple Silicon & Intel |

## Build Configurations

| Config | Use Case |
|--------|----------|
| `Debug` | Full debugging, no optimizations |
| `DebugGame` | Game code debug, engine optimized |
| `Development` | Default — balanced debug/perf |
| `Test` | Test builds without editor |
| `Shipping` | Final release builds |

## Error Recovery

- **"UE_ROOT not found"**: Set `UE_ROOT`, e.g. `export UE_ROOT="/Users/Shared/Epic Games/UE_5.5"`
- **"No .uproject found"**: Provide `--project` or cd into the project directory.
- **Build errors**: Check include paths, `Build.cs` dependencies, run `Setup.sh` if missing deps.

## Guidelines

- Default timeout: 30 min for build, 60 min for package.
- Use **build** for iterative development (fast compile). Use **package** only for distributable.
- **clean** is safe — only removes generated artifacts, not source code.
- Always pass `--config Shipping` for release builds.
