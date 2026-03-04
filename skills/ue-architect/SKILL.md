---
name: ue-architect
description: Delegate complex Unreal Engine architecture and system design questions to a specialized subagent. TRIGGER when the user asks about game architecture, system design, code organization, module/plugin structure, class hierarchy decisions, scalability strategy, networking architecture, or when to use subsystems vs components vs actors. DO NOT TRIGGER for writing specific C++ code (use ue-coder), in-editor automation (use ue-editor/ue-agent), building/compiling (use ue-builder), or API reference lookups without architectural context.
---

# UE Architect Agent — Specialized Subagent

Spawn a focused subagent for Unreal Engine architecture consultation — system design, module organization, class hierarchies, communication patterns, scalability strategy, and pattern selection (GAS vs custom, subsystems vs components, etc.).

## When to Delegate

- **System design** — "how should I structure my inventory system?", "what's the best architecture for a lobby?"
- **Pattern selection** — "should I use GAS or a custom ability system?", "subsystem vs component vs manager actor?"
- **Module/plugin organization** — game features, plugin boundaries, dependency management
- **Class hierarchy decisions** — actor vs component vs subsystem, interface vs inheritance
- **Networking strategy** — replication topology, authority model, relevancy
- **Scalability planning** — World Partition, streaming, LOD strategy, open-world architecture
- **Architecture review** — critique an existing design, identify anti-patterns, suggest refactoring
- **Data-driven design** — DataAssets vs DataTables, gameplay tags taxonomy, config-driven behavior
- **UI architecture** — CommonUI, widget hierarchy, MVC/MVVM in UE
- **AI architecture** — behavior tree vs state machine vs utility AI, EQS, perception

## When NOT to Delegate

- **Writing specific C++ code** — use **ue-coder** skill
- **In-editor automation** — use **ue-agent** or **ue-editor** skill
- **Building or packaging** — use **ue-builder** skill
- **Material creation** — use **ue-material-agent** skill
- **GAS implementation** — use **ue-gas-agent** skill (for writing code; ue-architect is for design decisions about GAS)
- **API reference lookup** — use `ue_api_search` / `ue_api_type` directly

## How to Spawn

Use the **Agent** tool with `subagent_type: "general-purpose"`. Include the prompt template below with the specific question filled in.

### Prompt Template

```
You are a senior Unreal Engine architect with deep expertise in Epic's production patterns (Lyra, Fortnite, Valley of the Ancient). Provide actionable architectural guidance for the following question.

**Question:** [describe the architecture question or design challenge]

**Context:** [game type, team size, multiplayer?, target platforms, current project structure if relevant]

**Knowledge base — read these files based on the topic:**

| Topic | File |
|-------|------|
| Core principles, SOLID in UE | ue-toolkit/skills/ue-architect/knowledge/architecture-principles.md |
| Module and plugin design | ue-toolkit/skills/ue-architect/knowledge/module-design.md |
| Experience system & GameFeatures | ue-toolkit/skills/ue-architect/knowledge/experience-system.md |
| Gameplay Ability System architecture | ue-toolkit/skills/ue-architect/knowledge/gas-architecture.md |
| Networking & replication | ue-toolkit/skills/ue-architect/knowledge/networking.md |
| Data-driven design & tags | ue-toolkit/skills/ue-architect/knowledge/data-driven-design.md |
| Component architecture & init state | ue-toolkit/skills/ue-architect/knowledge/component-architecture.md |
| UI architecture (CommonUI) | ue-toolkit/skills/ue-architect/knowledge/ui-architecture.md |
| Performance optimization | ue-toolkit/skills/ue-architect/knowledge/performance.md |
| Common anti-patterns | ue-toolkit/skills/ue-architect/knowledge/anti-patterns.md |
| Subsystem patterns | ue-toolkit/skills/ue-architect/knowledge/subsystems.md |
| AI architecture | ue-toolkit/skills/ue-architect/knowledge/ai-architecture.md |
| Testing strategies | ue-toolkit/skills/ue-architect/knowledge/testing.md |
| Asset management & loading | ue-toolkit/skills/ue-architect/knowledge/asset-management.md |
| Scalability & world partition | ue-toolkit/skills/ue-architect/knowledge/scalability.md |
| Decision frameworks (when to use what) | ue-toolkit/skills/ue-architect/knowledge/decision-frameworks.md |
| Equipment, inventory & items | ue-toolkit/skills/ue-architect/knowledge/equipment-inventory.md |
| Team & player systems | ue-toolkit/skills/ue-architect/knowledge/team-player-systems.md |
| Messaging & events | ue-toolkit/skills/ue-architect/knowledge/messaging-events.md |
| Camera & input | ue-toolkit/skills/ue-architect/knowledge/camera-input.md |

**Instructions:**
1. ALWAYS read `knowledge/architecture-principles.md` first for foundational context
2. Read topic-specific knowledge files relevant to the question
3. For broad architectural questions, also read `knowledge/decision-frameworks.md`
4. Reference Lyra as the gold-standard implementation where applicable
5. Proactively warn about common anti-patterns related to the topic

**Response format:**
1. **Recommended Approach** — The pattern/architecture you recommend and why
2. **Implementation Outline** — Key classes, modules, data flow
3. **Lyra Reference** — How Lyra solves this (if applicable)
4. **Trade-offs** — What you gain and what you sacrifice
5. **Anti-patterns** — What to avoid and why
6. **Scale Considerations** — How this changes at different project scales
```

### Example Invocations

**System design question:**
```python
Agent(
    subagent_type="general-purpose",
    description="Architect inventory system",
    prompt="""You are a senior Unreal Engine architect...

    **Question:** How should I architect an equipment/inventory system for a multiplayer
    action RPG? Need to support: item stacking, equipment slots (weapon, armor, accessories),
    item rarity tiers, and networked inventory sync.

    **Context:** Small team (3 devs), UE 5.4, dedicated server, 4-player co-op PvE.
    Currently have GAS set up for combat. No inventory code yet.

    [include knowledge base table and instructions from template above]
    """
)
```

**Pattern selection question:**
```python
Agent(
    subagent_type="general-purpose",
    description="Compare AI architecture approaches",
    prompt="""You are a senior Unreal Engine architect...

    **Question:** Should I use Behavior Trees, State Machines, or Utility AI for enemy NPCs
    in a stealth game? Enemies need patrol, investigate, chase, search, and alert states
    with smooth transitions. ~20 concurrent AI agents.

    **Context:** Single-player, UE 5.5, medium-complexity AI (not AAA but needs to feel
    responsive). Using GAS for combat abilities.

    [include knowledge base table and instructions from template above]
    """
)
```

**Architecture review:**
```python
Agent(
    subagent_type="general-purpose",
    description="Review game mode architecture",
    prompt="""You are a senior Unreal Engine architect...

    **Question:** Review this game mode architecture for a round-based multiplayer shooter:
    - AMyGameMode manages round state (waiting, playing, post-round)
    - AMyPlayerController handles input + HUD
    - AMyCharacter has all gameplay logic (health, weapons, abilities) in the actor class
    - Items are actors spawned in the world with overlap detection
    - Score tracked in GameState with TMap<APlayerState*, int32>
    Is this sound? What should be refactored?

    **Context:** 8-player FFA, dedicated server, UE 5.4. Team of 2.

    [include knowledge base table and instructions from template above]
    """
)
```

## Tips

- Include project context (game type, team size, multiplayer, scale) — architecture advice depends heavily on context
- For broad "how do I structure my whole game" questions, break into focused sub-questions
- The subagent reads knowledge files via the Read tool — ensure file paths are correct relative to the project root
- The subagent's output is returned to you — summarize the key recommendations for the user
- For questions that span multiple domains (e.g., "networked inventory with GAS integration"), the subagent will read multiple knowledge files

---

see: knowledge/architecture-principles.md — Core UE architecture principles, SOLID adaptation for Unreal, Lyra patterns
see: knowledge/decision-frameworks.md — When to use what: subsystems vs components, GAS vs custom, actors vs objects
see: knowledge/anti-patterns.md — Common UE architectural mistakes and how to avoid them
see: knowledge/module-design.md — Module/plugin organization, GameFeature plugins, dependency management
see: knowledge/networking.md — Replication architecture, authority models, relevancy, prediction
