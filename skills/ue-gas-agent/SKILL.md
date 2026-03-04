---
name: ue-gas-agent
description: Delegate Gameplay Ability System (GAS) tasks to a specialized subagent. TRIGGER when the user asks to set up an ability system, create AttributeSets, implement GameplayAbilities, configure GameplayEffects, build Execution Calculations, set up GameplayCues, or architect GAS-based combat/RPG systems. DO NOT TRIGGER for single attribute queries, general C++ questions unrelated to GAS, Blueprint-only visual scripting, material/rendering tasks, or API reference lookups.
---

# UE GAS Agent — Specialized Subagent

Spawn a focused subagent for complex Unreal Engine Gameplay Ability System tasks that require setting up ability system components, creating attribute sets, implementing abilities, configuring effects, writing execution calculations, or architecting multiplayer ability frameworks.

## CRITICAL — Mistakes That Waste Hours

These rules were learned from official documentation and community experience. Violating them causes silent failures, replication bugs, or wasted debugging cycles.

### 1. ALWAYS Call EndAbility()
- Every code path in `ActivateAbility()` MUST call `EndAbility()` when done
- Forgetting it leaves the ability perpetually "active" — blocks future activations, breaks cooldowns
- Wire it to ALL completion/cancellation delegates on AbilityTasks

### 2. ASC Placement: Character vs PlayerState
- **Character**: Simpler but attributes/effects LOST on respawn (fine for single-player)
- **PlayerState**: Attributes persist across respawns (required for multiplayer)
- Decide this FIRST — changing later requires major refactoring

### 3. InitAbilityActorInfo() on BOTH Server and Client
- Server: call in `PossessedBy()`
- Client: call in `OnRep_PlayerState()` or `AcknowledgePossession()`
- Missing either side causes abilities to silently fail on that side

### 4. Mixed Replication Mode Requires Owner
- `EGameplayEffectReplicationMode::Mixed` requires `OwnerActor->GetOwner()` to be the Controller
- PlayerState has this by default; Character needs explicit `SetOwner(Controller)` in `PossessedBy()`
- Symptom: GameplayEffects don't replicate to owning client

### 5. NEVER Replicate Meta Attributes
- Meta attributes (IncomingDamage, IncomingHealing) are temporary calculation helpers
- They should NOT have `ReplicatedUsing` or appear in `GetLifetimeReplicatedProps`
- Replicating them causes race conditions and stale values

### 6. PreAttributeChange is for Clamping ONLY
- **WRONG**: Triggering death, spawning effects, sending events in `PreAttributeChange()`
- **CORRECT**: Only clamp values here (e.g., `NewValue = FMath::Clamp(NewValue, 0.f, Max)`)
- Use `PostGameplayEffectExecute()` for game reactions (death, XP gain, etc.)

### 7. SetByCaller Must Be Set Before Applying
- Missing `SetSetByCallerMagnitude()` returns 0.0 with a runtime error log
- Always set ALL SetByCaller values on the spec before `ApplyGameplayEffectSpec`
- Use `FGameplayTag::RequestGameplayTag()` — NOT string literals — for the tag parameter

### 8. Use ABILITYLIST_SCOPE_LOCK() When Iterating
- Iterating `GetActivatableAbilities()` without the lock can crash if abilities are added/removed during iteration
- Always wrap with `ABILITYLIST_SCOPE_LOCK()` macro

### 9. Build.cs Must Include All Three Modules
- `"GameplayAbilities"`, `"GameplayTags"`, `"GameplayTasks"` — missing any one causes cryptic linker errors
- Add to `PrivateDependencyModuleNames`, not `PublicDependencyModuleNames` (unless other modules need them)

### 10. GameplayCues Are Cosmetic ONLY
- **NEVER** put gameplay logic in GameplayCues — they use unreliable replication
- Clients may miss cue events entirely; use them only for particles, sounds, camera shakes
- All gameplay state changes must go through GameplayEffects or ability logic

## When to Delegate

- **ASC setup** — implementing IAbilitySystemInterface, initialization on server+client, replication mode selection
- **AttributeSet creation** — declaring attributes with replication, accessor macros, clamping, meta attributes
- **GameplayAbility implementation** — activation, cost/cooldown, ability tasks, tag gating, net execution policy
- **GameplayEffect configuration** — modifiers, duration types, stacking, SetByCaller, immunity
- **Execution Calculations** — damage formulas capturing source/target attributes
- **Custom MMC** — modifier magnitude calculations with attribute capture
- **GAS architecture** — combat system design, ability hierarchy, tag taxonomy, replication strategy
- **GameplayCue setup** — actor cue classes with spawn/destroy lifecycle
- **Multiplayer ability systems** — prediction, replication modes, PlayerState-based ASC

## When NOT to Delegate

- **Single attribute query** — just look it up in the reference
- **General C++ unrelated to GAS** — use **ue-coder** skill
- **Blueprint-only ability graphs** — GAS Blueprint work is straightforward, doesn't need a subagent
- **Material/rendering tasks** — use **ue-material-agent** skill
- **API reference lookup** — use `ue_api_search` / `ue_api_type` directly
- **General editor automation** — use **ue-agent** skill

## How to Spawn

Use the **Agent** tool with `subagent_type: "general-purpose"`. Include the prompt template below with the specific task filled in.

### Prompt Template

```
You are a UE Gameplay Ability System (GAS) automation agent. Complete the following GAS task for an Unreal Engine project.

**Task:** [describe what to implement — ASC setup, AttributeSet, abilities, effects, etc.]

**Available MCP tools:**
- ue_health — check editor connectivity
- ue_execute(script, timeout) — run Python in editor (use `import unreal`, `print()` for output)
- ue_batch(scripts) — run multiple scripts sequentially
- ue_api_search(query) — search 15K+ UE Python API types
- ue_api_type(name) — get full type reference (properties, methods)
- ue_screenshot(width, height) — capture viewport
- ue_scene_tree() — get level actor hierarchy
- ue_logs(lines, filter, severity) — read editor logs

**Shell fallback (if MCP tools unavailable):**
Use `bash ~/.claude/skills/ue-scripter/scripts/ue-exec.sh`:
- `--health` — health check
- `--script 'import unreal; ...'` — execute inline Python
- `--file /tmp/script.py` — execute from file
DO NOT use raw `curl`. All AgentBridge endpoints are under `/agent/` prefix
(e.g., `/agent/health`, `/agent/execute`). There is NO `/health` or `/api/v1/...`.

**C++ File Workflow:**
GAS code is primarily C++. Use Read/Write/Edit tools to create and modify .h/.cpp files directly in the project Source directory. After writing files:
1. Check existing files with Glob/Grep to understand project structure
2. Write .h and .cpp files using Write tool
3. Use ue_execute to trigger hot-reload or inform user to recompile
4. Verify compilation via ue_logs(filter="CompilerResultsLog")

## GAS Workflow Paths

### Path 1: ASC Setup (AbilitySystemComponent on Character or PlayerState)
1. **Decide placement**: Character (simple) vs PlayerState (multiplayer-persistent)
2. Create/modify Character class: add ASC UPROPERTY, implement IAbilitySystemInterface
3. Implement GetAbilitySystemComponent() override
4. Initialize in PossessedBy() (server) and OnRep_PlayerState() (client)
5. Set replication mode: Full (SP), Mixed (MP players), Minimal (MP AI)
6. If PlayerState-based: add ASC to PlayerState, forward from Character

### Path 2: AttributeSet Creation
1. Create UAttributeSet subclass with FGameplayAttributeData members
2. Add GAMEPLAYATTRIBUTE_PROPERTY_GETTER/VALUE_GETTER/SETTER/INITTER macros for each attribute
3. Implement replication: OnRep_ functions, GetLifetimeReplicatedProps with DOREPLIFETIME_CONDITION_NOTIFY
4. Override PreAttributeChange() for clamping
5. Override PostGameplayEffectExecute() for game reactions (death, etc.)
6. Register with ASC (constructor subobject or programmatic)

### Path 3: GameplayAbility Creation
1. Create UGameplayAbility subclass
2. Set instancing policy (InstancedPerActor recommended)
3. Set net execution policy (LocalPredicted for responsive MP)
4. Configure tag requirements (ActivationRequired/Blocked, Cancel/Block tags)
5. Implement ActivateAbility() with game logic
6. Call CommitAbility() for cost/cooldown
7. Use AbilityTasks for async operations (montages, waits)
8. ALWAYS call EndAbility() on ALL completion paths
9. Create Cost GE (Instant, subtract resource) and Cooldown GE (Duration with cooldown tag)

### Path 4: GameplayEffect Configuration
1. Create UGameplayEffect subclass or Blueprint
2. Set DurationPolicy: Instant/HasDuration/Infinite
3. Add Modifiers: attribute, operation (Add/Multiply/Override), magnitude type
4. For SetByCaller: set DataTag on modifier, set value at runtime via spec
5. For complex math: add Execution Calculation class
6. Configure stacking if needed (count, duration behavior, overflow)
7. Add GE Components (tag requirements, immunity, chained effects)

### Path 5: Execution Calculation (Damage Formula)
1. Create UGameplayEffectExecutionCalculation subclass
2. Declare captured attributes in constructor using FGameplayEffectAttributeCaptureDefinition
3. Override Execute_Implementation()
4. Use ExecutionParams.AttemptCalculateCapturedAttributeMagnitude() to read values
5. Compute result and add to OutExecutionOutput via AddOutputModifier()
6. Reference from a GameplayEffect's Executions array

### Path 6: GameplayCue Setup
1. Create AGameplayCueNotify_Actor subclass for persistent effects
2. Override OnActive/WhileActive/OnExecute/OnRemove as needed
3. Tag MUST start with "GameplayCue." prefix
4. Trigger via ASC: ExecuteGameplayCue (one-shot) or AddGameplayCue (persistent)
5. NEVER put gameplay logic in cues — cosmetic only

## Critical Rules

1. **ALWAYS call EndAbility()** — every code path must end the ability or it stays "active" forever
2. **ALWAYS include all three modules in Build.cs** — GameplayAbilities, GameplayTags, GameplayTasks
3. **ALWAYS InitAbilityActorInfo on both server AND client** — server in PossessedBy, client in OnRep_PlayerState
4. **ALWAYS use ABILITYLIST_SCOPE_LOCK()** when iterating GetActivatableAbilities()
5. **ALWAYS set SetByCaller values before applying** — missing values return 0 with runtime error
6. **NEVER replicate meta attributes** — they are temporary calculation helpers
7. **NEVER put gameplay logic in GameplayCues** — unreliable replication, cosmetic only
8. **NEVER use PreAttributeChange for game reactions** — clamping only; use PostGameplayEffectExecute
9. **NEVER use Non-Instanced policy with Blueprints** — C++ only, no member vars or delegates
10. **NEVER place multiple ASCs on one actor** — causes ambiguity in all queries
11. **Use RequestGameplayTag() for tag lookups** — not string constructors
12. **Instant GEs modify BaseValue; Duration/Infinite modify CurrentValue** — know the difference
13. **GameplayEffect removal cannot be predicted** — workaround: predict adding an inverse effect
14. **DataTable MinValue/MaxValue columns have NO effect** — implement clamping yourself
15. **Increase PlayerState NetUpdateFrequency** when ASC lives there — default is very low

## Verification Steps

After completing GAS implementation, the subagent MUST:
1. Verify all .h/.cpp files compile: check ue_logs(filter="CompilerResultsLog") or inform user to build
2. Confirm Build.cs includes GameplayAbilities, GameplayTags, GameplayTasks
3. Verify all abilities call EndAbility() on every code path
4. Check that InitAbilityActorInfo is called on both server and client paths
5. Confirm replication setup: GetLifetimeReplicatedProps, OnRep_ functions, GAMEPLAYATTRIBUTE_REPNOTIFY macro
6. Report structured summary of what was created

**Output format:**
Return a structured summary:
- What was done (steps taken)
- Files created/modified (full paths)
- Classes created (names, parent classes, key methods)
- Attributes defined (names, types, replication status)
- Abilities implemented (names, net execution policy, instancing)
- Effects configured (names, duration type, modifiers)
- Any compilation warnings or issues
```

### Example Invocations

**Basic GAS setup for a character:**
```python
Agent(
    subagent_type="general-purpose",
    description="Set up GAS on character",
    prompt="""You are a UE Gameplay Ability System (GAS) automation agent...

    **Task:** Set up a complete GAS foundation for AMyCharacter:
    1. Add AbilitySystemComponent to the character (Character-based, single-player)
    2. Implement IAbilitySystemInterface
    3. Create a basic AttributeSet with Health, MaxHealth, Mana, MaxMana
    4. Full replication boilerplate (OnRep_, GetLifetimeReplicatedProps)
    5. PreAttributeChange clamping (Health 0..MaxHealth, Mana 0..MaxMana)
    6. PostGameplayEffectExecute with death check when Health reaches 0
    7. Add GameplayAbilities, GameplayTags, GameplayTasks to Build.cs

    Project source directory: [path to Source/]

    [include full tool list and workflow paths from template above]
    """
)
```

**Damage execution calculation:**
```python
Agent(
    subagent_type="general-purpose",
    description="Create damage execution calc",
    prompt="""You are a UE Gameplay Ability System (GAS) automation agent...

    **Task:** Create a damage execution calculation class:
    1. UMyDamageExecCalc : UGameplayEffectExecutionCalculation
    2. Capture source AttackPower and target Armor from their AttributeSets
    3. Formula: FinalDamage = max(SourceAttack - TargetArmor * 0.5, 0) * DamageMultiplier
    4. DamageMultiplier comes from SetByCaller tag "Data.DamageMultiplier"
    5. Output: subtract from target Health attribute
    6. Create a GameplayEffect Blueprint/class that uses this execution

    [include full tool list and workflow paths from template above]
    """
)
```

**Multiplayer ability with prediction:**
```python
Agent(
    subagent_type="general-purpose",
    description="Create predicted melee ability",
    prompt="""You are a UE Gameplay Ability System (GAS) automation agent...

    **Task:** Create a predicted melee attack ability:
    1. UGA_MeleeAttack : UGameplayAbility
    2. InstancedPerActor, LocalPredicted net execution
    3. PlayMontageAndWait for attack animation
    4. WaitGameplayEvent for hit notification (from anim notify)
    5. On hit: apply damage GE to target via trace
    6. Cost: 10 Stamina (Instant GE)
    7. Cooldown: 1.5 seconds (Duration GE with tag Cooldown.Ability.MeleeAttack)
    8. Block activation while stunned (ActivationBlockedTags: Status.Debuff.Stun)

    [include full tool list and workflow paths from template above]
    """
)
```

## Tips

- Keep subagent prompts focused on ONE GAS subsystem (don't mix "set up ASC" with "create 5 abilities")
- Include the full tool list — the subagent does not inherit skill context
- For full combat systems, break into sequential subagent calls: ASC setup → AttributeSet → Abilities → Effects
- The subagent's output is returned to you — summarize it for the user
- Use `ue_api_search("AbilitySystem")` to discover GAS types not listed in knowledge files

see: knowledge/gas-reference.md — Complete GAS reference: core components, ASC setup, attributes, effects, abilities, tasks, tags, cues, prediction, targeting, API tables
see: knowledge/gas-patterns.md — Copy-paste C++ recipes: ASC setup, AttributeSet, abilities, execution calculations, MMC, cues, native tags, common gameplay patterns
see: knowledge/gas-pitfalls.md — Hard-won debugging knowledge: 15+ pitfalls with symptoms, causes, and fixes
