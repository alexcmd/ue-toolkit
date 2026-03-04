---
name: ue-api-virtualproduction
description: "UE Python API reference for Virtual Production (95 modules, 915 types). TRIGGER when writing Python scripts involving Live Link, nDisplay, virtual cameras, composure, lens calibration, performance capture. DO NOT TRIGGER for executing scripts (use ue-scripter skill)."
---

# UE Python API — Virtual Production

Reference for 95 Unreal Engine modules covering Live Link, nDisplay, virtual cameras, composure, lens calibration, performance capture.
All types accessible via `import unreal`. To execute scripts, use the **ue-scripter** skill.

## Modules

| Module | Types | Description |
|--------|-------|-------------|
| `DisplayClusterConfiguration` | 103 | see: knowledge/DisplayClusterConfiguration.md — DisplayClusterConfiguration module API reference |
| `Composure` | 58 | see: knowledge/Composure.md — Composure module API reference |
| `LiveLinkInterface` | 55 | see: knowledge/LiveLinkInterface.md — LiveLinkInterface module API reference |
| `VCamCore` | 55 | see: knowledge/VCamCore.md — VCamCore module API reference |
| `PerformanceCaptureWorkflow` | 46 | see: knowledge/PerformanceCaptureWorkflow.md — PerformanceCaptureWorkflow module API reference |
| `DisplayCluster` | 43 | see: knowledge/DisplayCluster.md — nDisplay: multi-display rendering, ICVFX |
| `MultiUserClientLibrary` | 43 | see: knowledge/MultiUserClientLibrary.md — MultiUserClientLibrary module API reference |
| `CameraCalibrationCore` | 39 | see: knowledge/CameraCalibrationCore.md — CameraCalibrationCore module API reference |
| `LiveLinkOpenTrackIO` | 33 | see: knowledge/LiveLinkOpenTrackIO.md — LiveLinkOpenTrackIO module API reference |
| `RemoteControl` | 27 | see: knowledge/RemoteControl.md — RemoteControl module API reference |
| `VPUtilities` | 23 | see: knowledge/VPUtilities.md — VPUtilities module API reference |
| `VREditor` | 19 | see: knowledge/VREditor.md — VREditor module API reference |
| `ViewportInteraction` | 19 | see: knowledge/ViewportInteraction.md — ViewportInteraction module API reference |
| `VirtualCamera` | 18 | see: knowledge/VirtualCamera.md — VirtualCamera module API reference |
| `VCamExtensions` | 17 | see: knowledge/VCamExtensions.md — VCamExtensions module API reference |
| `LiveLink` | 16 | see: knowledge/LiveLink.md — Live Link: real-time data streaming framework |
| `RemoteControlLogic` | 16 | see: knowledge/RemoteControlLogic.md — RemoteControlLogic module API reference |
| `CinematicCamera` | 15 | see: knowledge/CinematicCamera.md — CinematicCamera module API reference |
| `LiveLinkCapabilities` | 14 | see: knowledge/LiveLinkCapabilities.md — LiveLinkCapabilities module API reference |
| `LiveLinkDevice` | 14 | see: knowledge/LiveLinkDevice.md — LiveLinkDevice module API reference |
| `CaptureDataCore` | 11 | see: knowledge/CaptureDataCore.md — CaptureDataCore module API reference |
| `CineCameraRigs` | 10 | see: knowledge/CineCameraRigs.md — CineCameraRigs module API reference |
| `CameraCalibrationEditor` | 9 | see: knowledge/CameraCalibrationEditor.md — CameraCalibrationEditor module API reference |
| `DisplayClusterMoviePipeline` | 8 | see: knowledge/DisplayClusterMoviePipeline.md — DisplayClusterMoviePipeline module API reference |
| `VPUtilitiesEditor` | 8 | see: knowledge/VPUtilitiesEditor.md — VPUtilitiesEditor module API reference |
| `CinematicPrestreamingEditor` | 7 | see: knowledge/CinematicPrestreamingEditor.md — CinematicPrestreamingEditor module API reference |
| `LiveLinkControlRig` | 7 | see: knowledge/LiveLinkControlRig.md — LiveLinkControlRig module API reference |
| `VCamCoreEditor` | 7 | see: knowledge/VCamCoreEditor.md — VCamCoreEditor module API reference |
| `CineAssemblyToolsEditor` | 6 | see: knowledge/CineAssemblyToolsEditor.md — CineAssemblyToolsEditor module API reference |
| `LiveLinkComponents` | 6 | see: knowledge/LiveLinkComponents.md — LiveLinkComponents module API reference |
| `LiveLinkSequencer` | 6 | see: knowledge/LiveLinkSequencer.md — LiveLinkSequencer module API reference |
| `OpenCVHelper` | 6 | see: knowledge/OpenCVHelper.md — OpenCVHelper module API reference |
| `CaptureManagerEditorSettings` | 5 | see: knowledge/CaptureManagerEditorSettings.md — CaptureManagerEditorSettings module API reference |
| `CineAssemblyTools` | 5 | see: knowledge/CineAssemblyTools.md — CineAssemblyTools module API reference |
| `CinematicPrestreaming` | 5 | see: knowledge/CinematicPrestreaming.md — CinematicPrestreaming module API reference |
| `DisplayClusterWarp` | 5 | see: knowledge/DisplayClusterWarp.md — DisplayClusterWarp module API reference |
| `LensComponent` | 5 | see: knowledge/LensComponent.md — LensComponent module API reference |
| `PerformanceCaptureCore` | 5 | see: knowledge/PerformanceCaptureCore.md — PerformanceCaptureCore module API reference |
| `VPBookmark` | 5 | see: knowledge/VPBookmark.md — VPBookmark module API reference |
| `CaptureManagerEditor` | 4 | see: knowledge/CaptureManagerEditor.md — CaptureManagerEditor module API reference |
| `ConcertReplicationScripting` | 4 | see: knowledge/ConcertReplicationScripting.md — ConcertReplicationScripting module API reference |
| `DisplayClusterMedia` | 4 | see: knowledge/DisplayClusterMedia.md — DisplayClusterMedia module API reference |
| `DisplayClusterMediaEditor` | 4 | see: knowledge/DisplayClusterMediaEditor.md — DisplayClusterMediaEditor module API reference |
| `LiveLinkAnimationCore` | 4 | see: knowledge/LiveLinkAnimationCore.md — LiveLinkAnimationCore module API reference |
| `LiveLinkLens` | 4 | see: knowledge/LiveLinkLens.md — LiveLinkLens module API reference |
| `LiveLinkPrestonMDR` | 4 | see: knowledge/LiveLinkPrestonMDR.md — LiveLinkPrestonMDR module API reference |
| `CaptureDataEditor` | 3 | see: knowledge/CaptureDataEditor.md — CaptureDataEditor module API reference |
| `CaptureDataUtils` | 3 | see: knowledge/CaptureDataUtils.md — CaptureDataUtils module API reference |
| `CaptureManagerSettings` | 3 | see: knowledge/CaptureManagerSettings.md — CaptureManagerSettings module API reference |
| `ConcertSyncClient` | 3 | see: knowledge/ConcertSyncClient.md — ConcertSyncClient module API reference |
| `DisplayClusterConfigurator` | 3 | see: knowledge/DisplayClusterConfigurator.md — DisplayClusterConfigurator module API reference |
| `DisplayClusterProjection` | 3 | see: knowledge/DisplayClusterProjection.md — DisplayClusterProjection module API reference |
| `LensComponentEditor` | 3 | see: knowledge/LensComponentEditor.md — LensComponentEditor module API reference |
| `LiveLinkCameraRecording` | 3 | see: knowledge/LiveLinkCameraRecording.md — LiveLinkCameraRecording module API reference |
| `LiveLinkEditor` | 3 | see: knowledge/LiveLinkEditor.md — LiveLinkEditor module API reference |
| `OpenCVLensDistortion` | 3 | see: knowledge/OpenCVLensDistortion.md — OpenCVLensDistortion module API reference |
| `PFMExporter` | 3 | see: knowledge/PFMExporter.md — PFMExporter module API reference |
| `PanoramicCapture` | 3 | see: knowledge/PanoramicCapture.md — PanoramicCapture module API reference |
| `RemoteControlCommon` | 3 | see: knowledge/RemoteControlCommon.md — RemoteControlCommon module API reference |
| `CompositeCore` | 2 | see: knowledge/CompositeCore.md — CompositeCore module API reference |
| `ConcertSyncTest` | 2 | see: knowledge/ConcertSyncTest.md — ConcertSyncTest module API reference |
| `ConcertTakeRecorder` | 2 | see: knowledge/ConcertTakeRecorder.md — ConcertTakeRecorder module API reference |
| `DisplayClusterShaders` | 2 | see: knowledge/DisplayClusterShaders.md — DisplayClusterShaders module API reference |
| `LensDistortion` | 2 | see: knowledge/LensDistortion.md — LensDistortion module API reference |
| `LiveLinkCurveDebugUI` | 2 | see: knowledge/LiveLinkCurveDebugUI.md — LiveLinkCurveDebugUI module API reference |
| `LiveLinkMovieScene` | 2 | see: knowledge/LiveLinkMovieScene.md — LiveLinkMovieScene module API reference |
| `RemoteSession` | 2 | see: knowledge/RemoteSession.md — RemoteSession module API reference |
| `ReplayTracks` | 2 | see: knowledge/ReplayTracks.md — ReplayTracks module API reference |
| `Shotgrid` | 2 | see: knowledge/Shotgrid.md — Shotgrid module API reference |
| `VCamExtensionsEditor` | 2 | see: knowledge/VCamExtensionsEditor.md — VCamExtensionsEditor module API reference |
| `VPRoles` | 2 | see: knowledge/VPRoles.md — VPRoles module API reference |
| `VirtualScouting` | 2 | see: knowledge/VirtualScouting.md — VirtualScouting module API reference |
| `CaptureManagerUnrealEndpoint` | 1 | see: knowledge/CaptureManagerUnrealEndpoint.md — CaptureManagerUnrealEndpoint module API reference |
| `CineCameraSceneCapture` | 1 | see: knowledge/CineCameraSceneCapture.md — CineCameraSceneCapture module API reference |
| `DisplayClusterLightCardExtender` | 1 | see: knowledge/DisplayClusterLightCardExtender.md — DisplayClusterLightCardExtender module API reference |
| `ICVFXTesting` | 1 | see: knowledge/ICVFXTesting.md — ICVFXTesting module API reference |
| `LedWallCalibration` | 1 | see: knowledge/LedWallCalibration.md — LedWallCalibration module API reference |
| `LiveLinkExampleDevice` | 1 | see: knowledge/LiveLinkExampleDevice.md — LiveLinkExampleDevice module API reference |
| `LiveLinkFaceImporter` | 1 | see: knowledge/LiveLinkFaceImporter.md — LiveLinkFaceImporter module API reference |
| `LiveLinkFreeD` | 1 | see: knowledge/LiveLinkFreeD.md — LiveLinkFreeD module API reference |
| `LiveLinkGraphNode` | 1 | see: knowledge/LiveLinkGraphNode.md — LiveLinkGraphNode module API reference |
| `LiveLinkHub` | 1 | see: knowledge/LiveLinkHub.md — LiveLinkHub module API reference |
| `LiveLinkHubMessaging` | 1 | see: knowledge/LiveLinkHubMessaging.md — LiveLinkHubMessaging module API reference |
| `LiveLinkVRPN` | 1 | see: knowledge/LiveLinkVRPN.md — LiveLinkVRPN module API reference |
| `MultiUserReplicationEditor` | 1 | see: knowledge/MultiUserReplicationEditor.md — MultiUserReplicationEditor module API reference |
| `OpenCVLensCalibration` | 1 | see: knowledge/OpenCVLensCalibration.md — OpenCVLensCalibration module API reference |
| `RemoteControlComponents` | 1 | see: knowledge/RemoteControlComponents.md — RemoteControlComponents module API reference |
| `RemoteControlUI` | 1 | see: knowledge/RemoteControlUI.md — RemoteControlUI module API reference |
| `RemoteControlWebInterface` | 1 | see: knowledge/RemoteControlWebInterface.md — RemoteControlWebInterface module API reference |
| `ReplayTracksEditor` | 1 | see: knowledge/ReplayTracksEditor.md — ReplayTracksEditor module API reference |
| `StormSyncEditor` | 1 | see: knowledge/StormSyncEditor.md — StormSyncEditor module API reference |
| `VPBookmarkEditor` | 1 | see: knowledge/VPBookmarkEditor.md — VPBookmarkEditor module API reference |
| `VPSettings` | 1 | see: knowledge/VPSettings.md — VPSettings module API reference |
| `VirtualCameraEditor` | 1 | see: knowledge/VirtualCameraEditor.md — VirtualCameraEditor module API reference |
| `WarpUtils` | 1 | see: knowledge/WarpUtils.md — WarpUtils module API reference |
