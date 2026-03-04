---
name: ue-api-editor
description: "UE Python API reference for Editor Tools (87 modules, 1437 types). TRIGGER when writing Python scripts involving editor framework, blueprints, level editor, content browser, sequencer, asset management. DO NOT TRIGGER for executing scripts (use ue-scripter skill)."
---

# UE Python API — Editor Tools

Reference for 87 Unreal Engine modules covering editor framework, blueprints, level editor, content browser, sequencer, asset management.
All types accessible via `import unreal`. To execute scripts, use the **ue-scripter** skill.

## Modules

| Module | Types | Description |
|--------|-------|-------------|
| `UnrealEd` | 260 | see: knowledge/UnrealEd.md — Unreal Editor: core editor types and utilities |
| `MovieRenderPipelineCore` | 186 | see: knowledge/MovieRenderPipelineCore.md — MovieRenderPipelineCore module API reference |
| `MovieSceneTracks` | 119 | see: knowledge/MovieSceneTracks.md — MovieSceneTracks module API reference |
| `MovieScene` | 87 | see: knowledge/MovieScene.md — Sequencer: tracks, sections, channels, evaluation |
| `ScriptableToolsFramework` | 67 | see: knowledge/ScriptableToolsFramework.md — ScriptableToolsFramework module API reference |
| `Blutility` | 57 | see: knowledge/Blutility.md — Blutility module API reference |
| `FunctionalTesting` | 38 | see: knowledge/FunctionalTesting.md — Automated testing: functional tests, screenshots, AI tests |
| `SequencerScripting` | 38 | see: knowledge/SequencerScripting.md — SequencerScripting module API reference |
| `LevelSnapshotFilters` | 33 | see: knowledge/LevelSnapshotFilters.md — LevelSnapshotFilters module API reference |
| `TakeRecorder` | 32 | see: knowledge/TakeRecorder.md — TakeRecorder module API reference |
| `PropertyAnimator` | 30 | see: knowledge/PropertyAnimator.md — PropertyAnimator module API reference |
| `MovieSceneCapture` | 27 | see: knowledge/MovieSceneCapture.md — MovieSceneCapture module API reference |
| `MovieRenderPipelineRenderPasses` | 26 | see: knowledge/MovieRenderPipelineRenderPasses.md — MovieRenderPipelineRenderPasses module API reference |
| `InteractiveToolsFramework` | 25 | see: knowledge/InteractiveToolsFramework.md — InteractiveToolsFramework module API reference |
| `PropertyAnimatorCore` | 22 | see: knowledge/PropertyAnimatorCore.md — PropertyAnimatorCore module API reference |
| `ContentBrowserData` | 19 | see: knowledge/ContentBrowserData.md — ContentBrowserData module API reference |
| `LevelSequence` | 18 | see: knowledge/LevelSequence.md — Level sequences: actors, bindings, directors |
| `LevelSnapshots` | 18 | see: knowledge/LevelSnapshots.md — LevelSnapshots module API reference |
| `TakesCore` | 16 | see: knowledge/TakesCore.md — TakesCore module API reference |
| `EditorScriptingUtilities` | 15 | see: knowledge/EditorScriptingUtilities.md — EditorScriptingUtilities module API reference |
| `MovieRenderPipelineEditor` | 14 | see: knowledge/MovieRenderPipelineEditor.md — MovieRenderPipelineEditor module API reference |
| `DataValidation` | 13 | see: knowledge/DataValidation.md — DataValidation module API reference |
| `TakeRecorderSources` | 13 | see: knowledge/TakeRecorderSources.md — TakeRecorderSources module API reference |
| `TakeTrackRecorders` | 13 | see: knowledge/TakeTrackRecorders.md — TakeTrackRecorders module API reference |
| `Sequencer` | 12 | see: knowledge/Sequencer.md — Sequencer: timeline editor, tracks, keyframes |
| `AutomatedPerfTesting` | 11 | see: knowledge/AutomatedPerfTesting.md — AutomatedPerfTesting module API reference |
| `MovieSceneTools` | 11 | see: knowledge/MovieSceneTools.md — MovieSceneTools module API reference |
| `CurveEditor` | 10 | see: knowledge/CurveEditor.md — CurveEditor module API reference |
| `DataRegistry` | 10 | see: knowledge/DataRegistry.md — DataRegistry module API reference |
| `LevelEditor` | 9 | see: knowledge/LevelEditor.md — LevelEditor module API reference |
| `AssetTools` | 8 | see: knowledge/AssetTools.md — Asset tools: creation, import, export utilities |
| `LevelSnapshotsEditor` | 8 | see: knowledge/LevelSnapshotsEditor.md — LevelSnapshotsEditor module API reference |
| `NamingTokens` | 8 | see: knowledge/NamingTokens.md — NamingTokens module API reference |
| `ObjectMixerEditor` | 8 | see: knowledge/ObjectMixerEditor.md — ObjectMixerEditor module API reference |
| `TemplateSequence` | 8 | see: knowledge/TemplateSequence.md — TemplateSequence module API reference |
| `SequencerPlaylists` | 7 | see: knowledge/SequencerPlaylists.md — SequencerPlaylists module API reference |
| `SequencerScriptingEditor` | 7 | see: knowledge/SequencerScriptingEditor.md — SequencerScriptingEditor module API reference |
| `AssetRegistry` | 6 | see: knowledge/AssetRegistry.md — Asset registry: asset discovery, metadata, dependencies |
| `CurveExpression` | 6 | see: knowledge/CurveExpression.md — CurveExpression module API reference |
| `ConsoleVariablesEditor` | 5 | see: knowledge/ConsoleVariablesEditor.md — ConsoleVariablesEditor module API reference |
| `EditorScriptableToolsFramework` | 5 | see: knowledge/EditorScriptableToolsFramework.md — EditorScriptableToolsFramework module API reference |
| `EditorTests` | 5 | see: knowledge/EditorTests.md — EditorTests module API reference |
| `LevelSequenceEditor` | 5 | see: knowledge/LevelSequenceEditor.md — LevelSequenceEditor module API reference |
| `MovieRenderPipelineMP4Encoder` | 5 | see: knowledge/MovieRenderPipelineMP4Encoder.md — MovieRenderPipelineMP4Encoder module API reference |
| `MovieSceneAnimMixer` | 5 | see: knowledge/MovieSceneAnimMixer.md — MovieSceneAnimMixer module API reference |
| `SequencerCore` | 5 | see: knowledge/SequencerCore.md — SequencerCore module API reference |
| `WorldPartitionHLODUtilities` | 5 | see: knowledge/WorldPartitionHLODUtilities.md — WorldPartitionHLODUtilities module API reference |
| `DataLayerEditor` | 4 | see: knowledge/DataLayerEditor.md — DataLayerEditor module API reference |
| `MoviePipelineMaskRenderPass` | 4 | see: knowledge/MoviePipelineMaskRenderPass.md — MoviePipelineMaskRenderPass module API reference |
| `MovieRenderPipelineSettings` | 4 | see: knowledge/MovieRenderPipelineSettings.md — MovieRenderPipelineSettings module API reference |
| `SequenceRecorder` | 4 | see: knowledge/SequenceRecorder.md — SequenceRecorder module API reference |
| `ToolPresetAsset` | 4 | see: knowledge/ToolPresetAsset.md — ToolPresetAsset module API reference |
| `BlueprintGraph` | 3 | see: knowledge/BlueprintGraph.md — Blueprint graph: nodes, pins, compilation |
| `ContentBrowser` | 3 | see: knowledge/ContentBrowser.md — ContentBrowser module API reference |
| `CurveEditorTools` | 3 | see: knowledge/CurveEditorTools.md — CurveEditorTools module API reference |
| `CurveExpressionEditor` | 3 | see: knowledge/CurveExpressionEditor.md — CurveExpressionEditor module API reference |
| `EditorInteractiveToolsFramework` | 3 | see: knowledge/EditorInteractiveToolsFramework.md — EditorInteractiveToolsFramework module API reference |
| `FunctionalTestingEditor` | 3 | see: knowledge/FunctionalTestingEditor.md — FunctionalTestingEditor module API reference |
| `GameFeatures` | 3 | see: knowledge/GameFeatures.md — GameFeatures module API reference |
| `SourceControl` | 3 | see: knowledge/SourceControl.md — SourceControl module API reference |
| `TakeMovieScene` | 3 | see: knowledge/TakeMovieScene.md — TakeMovieScene module API reference |
| `AssetReferenceRestrictions` | 2 | see: knowledge/AssetReferenceRestrictions.md — AssetReferenceRestrictions module API reference |
| `BlueprintEditorLibrary` | 2 | see: knowledge/BlueprintEditorLibrary.md — BlueprintEditorLibrary module API reference |
| `ConsoleVariablesEditorRuntime` | 2 | see: knowledge/ConsoleVariablesEditorRuntime.md — ConsoleVariablesEditorRuntime module API reference |
| `Kismet` | 2 | see: knowledge/Kismet.md — Kismet module API reference |
| `MovieScenePoseSearchTracks` | 2 | see: knowledge/MovieScenePoseSearchTracks.md — MovieScenePoseSearchTracks module API reference |
| `MovieSceneTextTrack` | 2 | see: knowledge/MovieSceneTextTrack.md — MovieSceneTextTrack module API reference |
| `ScreenShotComparisonTools` | 2 | see: knowledge/ScreenShotComparisonTools.md — ScreenShotComparisonTools module API reference |
| `TemplateSequenceEditor` | 2 | see: knowledge/TemplateSequenceEditor.md — TemplateSequenceEditor module API reference |
| `WorkspaceEditor` | 2 | see: knowledge/WorkspaceEditor.md — WorkspaceEditor module API reference |
| `AssetDefinition` | 1 | see: knowledge/AssetDefinition.md — AssetDefinition module API reference |
| `AssetTags` | 1 | see: knowledge/AssetTags.md — AssetTags module API reference |
| `AutomationUtils` | 1 | see: knowledge/AutomationUtils.md — AutomationUtils module API reference |
| `BlueprintFileUtils` | 1 | see: knowledge/BlueprintFileUtils.md — BlueprintFileUtils module API reference |
| `ComponentVisualizers` | 1 | see: knowledge/ComponentVisualizers.md — ComponentVisualizers module API reference |
| `DataRegistryEditor` | 1 | see: knowledge/DataRegistryEditor.md — DataRegistryEditor module API reference |
| `EditorConfig` | 1 | see: knowledge/EditorConfig.md — EditorConfig module API reference |
| `EditorFramework` | 1 | see: knowledge/EditorFramework.md — EditorFramework module API reference |
| `EditorSubsystem` | 1 | see: knowledge/EditorSubsystem.md — EditorSubsystem module API reference |
| `GameFeaturesEditor` | 1 | see: knowledge/GameFeaturesEditor.md — GameFeaturesEditor module API reference |
| `LogVisualizer` | 1 | see: knowledge/LogVisualizer.md — LogVisualizer module API reference |
| `MovieSceneAnimMixerEditor` | 1 | see: knowledge/MovieSceneAnimMixerEditor.md — MovieSceneAnimMixerEditor module API reference |
| `NamingTokensUncookedOnly` | 1 | see: knowledge/NamingTokensUncookedOnly.md — NamingTokensUncookedOnly module API reference |
| `PropertyAnimatorCoreEditor` | 1 | see: knowledge/PropertyAnimatorCoreEditor.md — PropertyAnimatorCoreEditor module API reference |
| `TakeRecorderNamingTokens` | 1 | see: knowledge/TakeRecorderNamingTokens.md — TakeRecorderNamingTokens module API reference |
| `WorldBookmark` | 1 | see: knowledge/WorldBookmark.md — WorldBookmark module API reference |
| `WorldPartitionEditor` | 1 | see: knowledge/WorldPartitionEditor.md — WorldPartitionEditor module API reference |
