---
name: ue-coder
description: Write C++ code and create/edit Blueprints for Unreal Engine projects. Use when the user asks to create, modify, or debug UE C++ classes, components, actors, or Blueprints. For in-editor queries or asset inspection, use ue-editor. For building, use ue-builder. For launching the editor, use ue-runner.
---

# UE Coder

Write C++ gameplay code and create/edit Blueprints for Unreal Engine projects.

## Instructions

1. **C++ code** — create or edit `.h`/`.cpp` files directly.
2. **Blueprints** — delegate to the `ue_execute` MCP tool (Python via AgentBridge) to create/modify Blueprints programmatically.
3. **Mixed** — write C++ files, then use `ue_execute` for the Blueprint/editor part.

### Before writing code

1. Find `.uproject` to identify the project name and enabled plugins.
2. Find `Build.cs` for the target module to understand dependencies.
3. Read existing code to match established patterns.
4. Use `ue_api_search` / `ue_api_type` MCP tools to look up UE Python API types when writing Blueprint automation scripts.

## File Placement

| Type | Header | Source |
|------|--------|--------|
| Actor | `Source/<Module>/Public/<Name>.h` | `Source/<Module>/Private/<Name>.cpp` |
| Component | `Source/<Module>/Public/Components/<Name>.h` | `Source/<Module>/Private/Components/<Name>.cpp` |
| Subsystem | `Source/<Module>/Public/Subsystems/<Name>.h` | `Source/<Module>/Private/Subsystems/<Name>.cpp` |
| Interface | `Source/<Module>/Public/<Name>.h` | `Source/<Module>/Private/<Name>.cpp` |
| Function Library | `Source/<Module>/Public/<Name>.h` | `Source/<Module>/Private/<Name>.cpp` |
| Plugin Module | `Plugins/<Plugin>/Source/<Module>/Public/` | `Plugins/<Plugin>/Source/<Module>/Private/` |

If the project uses a flat structure (no subfolders), follow that instead.

## Naming Conventions

- `A` = Actor, `U` = UObject/Component, `F` = struct, `E` = enum, `I` = interface
- Files: match class name without prefix (`AMyActor` → `MyActor.h`)
- Always include `(BlueprintType)` or appropriate specifiers on UCLASS/USTRUCT/UENUM
- UPROPERTY: `EditAnywhere`, `BlueprintReadWrite`, `Category="..."`
- UFUNCTION: `BlueprintCallable`, `BlueprintPure`, `BlueprintImplementableEvent`, `BlueprintNativeEvent`

## Module Dependencies

When including types from other modules, add to `Build.cs`:
```csharp
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
```
Common: `InputCore`, `EnhancedInput`, `UMG`, `Slate`, `SlateCore`, `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `AIModule`, `NavigationSystem`, `Niagara`, `PhysicsCore`

## Generated Headers

```cpp
#include "MyActor.generated.h"  // in .h — last include before class declaration
#include "MyActor.h"            // in .cpp — first include
```

## After Writing C++ Code

Compile via the `ue_build` MCP tool (Live Coding) or use the **ue-builder** skill for full UBT builds.

## Guidelines

- **Read before writing**: match existing patterns.
- **Prefer C++ base + Blueprint child**: logic in C++, designer config in BP.
- **API macro**: `<MODULE>_API` on every cross-module class.
- **Include order**: generated header last in `.h`, own header first in `.cpp`.
- **Forward declare** in headers; include in `.cpp`.
- **Use `TObjectPtr<>`** for UPROPERTY object pointers (UE5 convention).

see: knowledge/cpp_patterns.md — Actor with component, delegates, Blueprint events, compile + create BP workflow
see: knowledge/blueprints.md — Create Blueprint, set defaults, add component, DataAsset, Widget BP
