// Copyright Anthropic, Inc. All Rights Reserved.

#include "AgentBridgeModule.h"
#include "AgentBridgeHelpers.h"
#include "AgentBridgeLogCapture.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "IPythonScriptPlugin.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Editor.h"
#include "PlayInEditorDataTypes.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "ITargetDeviceServicesModule.h"
#include "ITargetDeviceProxy.h"
#include "ITargetDeviceProxyManager.h"
#include "Misc/HotReloadInterface.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

#if PLATFORM_WINDOWS
#include "ILiveCodingModule.h"
#endif

#define LOCTEXT_NAMESPACE "FAgentBridgeModule"

DEFINE_LOG_CATEGORY_STATIC(LogAgentBridge, Log, All);

using namespace AgentBridgeHelpers;

// ────────────────────────────────────────────────────────────────────────────
// StartupModule
// ────────────────────────────────────────────────────────────────────────────

// Check if a TCP port is available by attempting to bind a socket
static bool IsPortAvailable(uint32 TestPort)
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return true; // Can't check, assume available
	}

	FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("AgentBridgePortCheck"), false);
	if (!Socket)
	{
		return true;
	}

	Socket->SetReuseAddr(false);

	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	Addr->SetAnyAddress();
	Addr->SetPort(TestPort);

	bool bBound = Socket->Bind(*Addr);
	Socket->Close();
	SocketSubsystem->DestroySocket(Socket);

	return bBound;
}

void FAgentBridgeModule::StartupModule()
{
	bool bPortFromCommandLine = FParse::Value(FCommandLine::Get(), TEXT("-AgentBridgePort="), Port);

	// If port wasn't explicitly set, check if default is available and try alternatives
	if (!bPortFromCommandLine && !IsPortAvailable(Port))
	{
		UE_LOG(LogAgentBridge, Warning, TEXT("Default port %d is already in use, searching for available port..."), Port);

		// Try a range of ports starting from default+1
		bool bFound = false;
		for (uint32 Offset = 1; Offset <= 100; ++Offset)
		{
			uint32 CandidatePort = Port + Offset;
			if (IsPortAvailable(CandidatePort))
			{
				UE_LOG(LogAgentBridge, Log, TEXT("Using available port %d"), CandidatePort);
				Port = CandidatePort;
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			UE_LOG(LogAgentBridge, Error, TEXT("No available port found in range %d-%d"), Port + 1, Port + 100);
			return;
		}
	}

	FHttpServerModule& HttpModule = FHttpServerModule::Get();
	TSharedPtr<IHttpRouter> Router = HttpModule.GetHttpRouter(Port);

	if (!Router)
	{
		UE_LOG(LogAgentBridge, Error, TEXT("Failed to get HTTP router on port %d"), Port);
		return;
	}

	// Start log capture
	LogCapture = MakeUnique<FAgentBridgeLogCapture>(2000);
	GLog->AddOutputDevice(LogCapture.Get());

	// ── GET /agent/health ────────────────────────────────────────────────

	HealthRouteHandle = Router->BindRoute(
		FHttpPath(TEXT("/agent/health")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			bool bPythonAvailable = IPythonScriptPlugin::Get() != nullptr;

			FString Json = MakeSuccessResponse([bPythonAvailable](TSharedRef<TJsonWriter<>>& W)
			{
				W->WriteValue(TEXT("status"), TEXT("ok"));
				W->WriteValue(TEXT("python"), bPythonAvailable);
			});

			SendJsonResponse(OnComplete, Json);
			return true;
		})
	);

	// ── POST /agent/execute ──────────────────────────────────────────────

	ExecuteRouteHandle = Router->BindRoute(
		FHttpPath(TEXT("/agent/execute")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			TSharedPtr<FJsonObject> Body = ParseJsonBody(Request);
			if (!Body)
			{
				// Keep backward-compatible error format with output/result fields
				FString ErrStr;
				TSharedRef<TJsonWriter<>> EW = TJsonWriterFactory<>::Create(&ErrStr);
				EW->WriteObjectStart();
				EW->WriteValue(TEXT("success"), false);
				EW->WriteValue(TEXT("output"), TEXT(""));
				EW->WriteValue(TEXT("result"), TEXT(""));
				EW->WriteValue(TEXT("error"), TEXT("Invalid JSON in request body"));
				EW->WriteObjectEnd();
				EW->Close();
				SendJsonResponse(OnComplete, ErrStr);
				return true;
			}

			FString Script = Body->GetStringField(TEXT("script"));
			if (Script.IsEmpty())
			{
				FString ErrStr;
				TSharedRef<TJsonWriter<>> EW = TJsonWriterFactory<>::Create(&ErrStr);
				EW->WriteObjectStart();
				EW->WriteValue(TEXT("success"), false);
				EW->WriteValue(TEXT("output"), TEXT(""));
				EW->WriteValue(TEXT("result"), TEXT(""));
				EW->WriteValue(TEXT("error"), TEXT("Missing 'script' field in request body"));
				EW->WriteObjectEnd();
				EW->Close();
				SendJsonResponse(OnComplete, ErrStr);
				return true;
			}

			IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
			if (!PythonPlugin)
			{
				FString ErrStr;
				TSharedRef<TJsonWriter<>> EW = TJsonWriterFactory<>::Create(&ErrStr);
				EW->WriteObjectStart();
				EW->WriteValue(TEXT("success"), false);
				EW->WriteValue(TEXT("output"), TEXT(""));
				EW->WriteValue(TEXT("result"), TEXT(""));
				EW->WriteValue(TEXT("error"), TEXT("Python scripting plugin is not available"));
				EW->WriteObjectEnd();
				EW->Close();
				SendJsonResponse(OnComplete, ErrStr);
				return true;
			}

			// Handler runs on the game thread (FHttpServerModule ticks via
			// FTSTicker on the game thread), so we can call Python directly.
			//
			// "isolated": true → Private scope (fresh dict per call, no __main__ pollution).
			// Use for any script that switches levels or loads maps to prevent
			// FPyReferenceCollector retaining stale UWorld refs → fatal GC assert.
			// Default false preserves existing cross-call variable sharing behaviour.
			bool bIsolated = false;
			Body->TryGetBoolField(TEXT("isolated"), bIsolated);

			FPythonCommandEx PythonCommand;
			PythonCommand.Command = Script;
			PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
			PythonCommand.FileExecutionScope = bIsolated
				? EPythonFileExecutionScope::Private
				: EPythonFileExecutionScope::Public;

			bool bSuccess = PythonPlugin->ExecPythonCommandEx(PythonCommand);

			// Always run gc.collect() after execution so Python releases any
			// UObject refs (worlds, actors) that left scope before UE's next GC pass.
			{
				FPythonCommandEx GcCommand;
				GcCommand.Command = TEXT("import gc; gc.collect()");
				GcCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteStatement;
				PythonPlugin->ExecPythonCommandEx(GcCommand);
			}

			FString CombinedOutput;
			for (const FPythonLogOutputEntry& LogEntry : PythonCommand.LogOutput)
			{
				if (!CombinedOutput.IsEmpty())
				{
					CombinedOutput += TEXT("\n");
				}
				CombinedOutput += LogEntry.Output;
			}

			FString ResponseStr;
			TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&ResponseStr);
			JsonWriter->WriteObjectStart();
			JsonWriter->WriteValue(TEXT("success"), bSuccess);
			JsonWriter->WriteValue(TEXT("output"), CombinedOutput);
			JsonWriter->WriteValue(TEXT("result"), PythonCommand.CommandResult);

			JsonWriter->WriteArrayStart(TEXT("log"));
			for (const FPythonLogOutputEntry& LogEntry : PythonCommand.LogOutput)
			{
				JsonWriter->WriteObjectStart();
				JsonWriter->WriteValue(TEXT("severity"), FString(LexToString(LogEntry.Type)));
				JsonWriter->WriteValue(TEXT("message"), LogEntry.Output);
				JsonWriter->WriteObjectEnd();
			}
			JsonWriter->WriteArrayEnd();

			JsonWriter->WriteObjectEnd();
			JsonWriter->Close();

			SendJsonResponse(OnComplete, ResponseStr);
			return true;
		})
	);

	// ── POST /agent/play ─────────────────────────────────────────────────

	PlayRouteHandle = Router->BindRoute(
		FHttpPath(TEXT("/agent/play")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			TSharedPtr<FJsonObject> Body = ParseJsonBody(Request);
			if (!Body)
			{
				SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Invalid JSON body")));
				return true;
			}

			FString Mode = Body->GetStringField(TEXT("mode"));
			if (Mode.IsEmpty())
			{
				SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Missing 'mode' field (pie|simulate|stop)")));
				return true;
			}

			// Handler already runs on the game thread (FHttpServerModule ticker).
			if (!GEditor)
			{
				SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Editor not available")));
				return true;
			}

			if (Mode == TEXT("stop"))
			{
				if (GEditor->IsPlaySessionInProgress())
				{
					GEditor->RequestEndPlayMap();
				}
				SendJsonResponse(OnComplete, MakeSuccessResponse([](TSharedRef<TJsonWriter<>>& W)
				{
					W->WriteValue(TEXT("state"), TEXT("stopped"));
				}));
				return true;
			}

			if (Mode == TEXT("pie") || Mode == TEXT("simulate"))
			{
				if (GEditor->IsPlaySessionInProgress())
				{
					FString CurrentState = GEditor->IsSimulateInEditorInProgress() ? TEXT("simulating") : TEXT("playing");
					SendJsonResponse(OnComplete, MakeErrorResponse(FString::Printf(TEXT("Session already active (state: %s). Stop first."), *CurrentState)));
					return true;
				}

				FRequestPlaySessionParams Params;
				Params.WorldType = (Mode == TEXT("simulate"))
					? EPlaySessionWorldType::SimulateInEditor
					: EPlaySessionWorldType::PlayInEditor;

				GEditor->RequestPlaySession(Params);

				FString NewState = (Mode == TEXT("simulate")) ? TEXT("simulating") : TEXT("playing");
				SendJsonResponse(OnComplete, MakeSuccessResponse([&NewState](TSharedRef<TJsonWriter<>>& W)
				{
					W->WriteValue(TEXT("state"), NewState);
				}));
				return true;
			}

			SendJsonResponse(OnComplete, MakeErrorResponse(FString::Printf(TEXT("Unknown mode '%s'. Use pie, simulate, or stop."), *Mode)));
			return true;
		})
	);

	// ── POST /agent/launch ───────────────────────────────────────────────

	LaunchRouteHandle = Router->BindRoute(
		FHttpPath(TEXT("/agent/launch")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			TSharedPtr<FJsonObject> Body = ParseJsonBody(Request);
			if (!Body)
			{
				SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Invalid JSON body")));
				return true;
			}

			FString Platform = Body->GetStringField(TEXT("platform"));
			FString DeviceId = Body->GetStringField(TEXT("device_id"));

			if (Platform.IsEmpty())
			{
				SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Missing 'platform' field")));
				return true;
			}

			if (!GEditor)
			{
				SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Editor not available")));
				return true;
			}

			ITargetDeviceServicesModule& DeviceServices =
				FModuleManager::LoadModuleChecked<ITargetDeviceServicesModule>("TargetDeviceServices");
			TSharedRef<ITargetDeviceProxyManager> ProxyManager = DeviceServices.GetDeviceProxyManager();

			TArray<TSharedPtr<ITargetDeviceProxy>> Proxies;
			ProxyManager->GetProxies(FName(*Platform), true, Proxies);

			TSharedPtr<ITargetDeviceProxy> TargetProxy;

			if (!DeviceId.IsEmpty())
			{
				for (const auto& Proxy : Proxies)
				{
					if (Proxy.IsValid() && Proxy->GetTargetDeviceId(NAME_None) == DeviceId)
					{
						TargetProxy = Proxy;
						break;
					}
				}
				if (!TargetProxy)
				{
					SendJsonResponse(OnComplete, MakeErrorResponse(FString::Printf(TEXT("Device '%s' not found for platform '%s'"), *DeviceId, *Platform)));
					return true;
				}
			}
			else if (Proxies.Num() > 0)
			{
				TargetProxy = Proxies[0];
			}
			else
			{
				SendJsonResponse(OnComplete, MakeErrorResponse(FString::Printf(TEXT("No devices found for platform '%s'"), *Platform)));
				return true;
			}

			FRequestPlaySessionParams Params;
			Params.SessionDestination = EPlaySessionDestinationType::Launcher;

			FRequestPlaySessionParams::FLauncherDeviceInfo DeviceInfo;
			DeviceInfo.DeviceId = TargetProxy->GetTargetDeviceId(NAME_None);
			DeviceInfo.DeviceName = TargetProxy->GetName();
			Params.LauncherTargetDevice = DeviceInfo;

			GEditor->RequestPlaySession(Params);

			FString DeviceName = TargetProxy->GetName();
			SendJsonResponse(OnComplete, MakeSuccessResponse([&DeviceName](TSharedRef<TJsonWriter<>>& W)
			{
				W->WriteValue(TEXT("message"), FString::Printf(TEXT("Launch initiated on %s"), *DeviceName));
			}));
			return true;
		})
	);

	// ── GET /agent/devices ───────────────────────────────────────────────

	DevicesRouteHandle = Router->BindRoute(
		FHttpPath(TEXT("/agent/devices")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			FString PlatformFilter = GetQueryParam(Request, TEXT("platform"));

			ITargetDeviceServicesModule& DeviceServices =
				FModuleManager::LoadModuleChecked<ITargetDeviceServicesModule>("TargetDeviceServices");
			TSharedRef<ITargetDeviceProxyManager> ProxyManager = DeviceServices.GetDeviceProxyManager();

			FName PlatformName = PlatformFilter.IsEmpty() ? NAME_None : FName(*PlatformFilter);
			TArray<TSharedPtr<ITargetDeviceProxy>> Proxies;
			ProxyManager->GetProxies(PlatformName, true, Proxies);

			FString Result = MakeSuccessResponse([&Proxies](TSharedRef<TJsonWriter<>>& W)
			{
				W->WriteArrayStart(TEXT("devices"));
				for (const auto& Proxy : Proxies)
				{
					if (!Proxy.IsValid() || Proxy->IsAggregated())
					{
						continue;
					}
					W->WriteObjectStart();
					W->WriteValue(TEXT("id"), Proxy->GetTargetDeviceId(NAME_None));
					W->WriteValue(TEXT("name"), Proxy->GetName());
					W->WriteValue(TEXT("platform"), Proxy->GetTargetPlatformName(NAME_None));
					W->WriteValue(TEXT("type"), Proxy->GetType());
					W->WriteValue(TEXT("connected"), Proxy->IsConnected());
					W->WriteValue(TEXT("model"), Proxy->GetModel());
					W->WriteValue(TEXT("host"), Proxy->GetHostName());
					W->WriteObjectEnd();
				}
				W->WriteArrayEnd();
			});

			SendJsonResponse(OnComplete, Result);
			return true;
		})
	);

	// ── GET /agent/configs ───────────────────────────────────────────────

	ConfigsRouteHandle = Router->BindRoute(
		FHttpPath(TEXT("/agent/configs")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			FString Result = MakeSuccessResponse([](TSharedRef<TJsonWriter<>>& W)
			{
				// Targets: scan Source/*.Target.cs
				TArray<FString> TargetFiles;
				IFileManager::Get().FindFiles(TargetFiles, *(FPaths::ProjectDir() / TEXT("Source") / TEXT("*.Target.cs")), true, false);

				W->WriteArrayStart(TEXT("targets"));
				for (const FString& File : TargetFiles)
				{
					FString TargetName = File;
					TargetName.RemoveFromEnd(TEXT(".Target.cs"));
					W->WriteValue(TargetName);
				}
				W->WriteArrayEnd();

				// Platforms
				ITargetPlatformManagerModule& TPM =
					FModuleManager::LoadModuleChecked<ITargetPlatformManagerModule>("TargetPlatform");
				const TArray<ITargetPlatform*>& Platforms = TPM.GetTargetPlatforms();

				W->WriteArrayStart(TEXT("platforms"));
				for (ITargetPlatform* Platform : Platforms)
				{
					W->WriteObjectStart();
					W->WriteValue(TEXT("name"), Platform->PlatformName());
					W->WriteValue(TEXT("display_name"), Platform->DisplayName().ToString());
					W->WriteObjectEnd();
				}
				W->WriteArrayEnd();

				// Configurations
				static const TCHAR* Configs[] = {
					TEXT("Debug"), TEXT("DebugGame"), TEXT("Development"), TEXT("Shipping"), TEXT("Test")
				};
				W->WriteArrayStart(TEXT("configurations"));
				for (const TCHAR* Config : Configs)
				{
					W->WriteValue(FString(Config));
				}
				W->WriteArrayEnd();
			});

			SendJsonResponse(OnComplete, Result);
			return true;
		})
	);

	// ── GET /agent/logs ──────────────────────────────────────────────────

	FAgentBridgeLogCapture* LogCapturePtr = LogCapture.Get();

	LogsRouteHandle = Router->BindRoute(
		FHttpPath(TEXT("/agent/logs")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([LogCapturePtr](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			int32 Lines = 100;
			FString LinesStr = GetQueryParam(Request, TEXT("lines"));
			if (!LinesStr.IsEmpty())
			{
				Lines = FCString::Atoi(*LinesStr);
				if (Lines <= 0) Lines = 100;
			}

			FString Filter = GetQueryParam(Request, TEXT("filter"));
			FString Severity = GetQueryParam(Request, TEXT("severity"));

			TArray<FAgentBridgeLogEntry> Entries = LogCapturePtr->GetRecentEntries(Lines, Filter, Severity);

			FString Json = MakeSuccessResponse([&Entries](TSharedRef<TJsonWriter<>>& W)
			{
				W->WriteValue(TEXT("count"), Entries.Num());
				W->WriteArrayStart(TEXT("entries"));
				for (const FAgentBridgeLogEntry& Entry : Entries)
				{
					W->WriteObjectStart();
					W->WriteValue(TEXT("timestamp"), Entry.Timestamp.ToIso8601());
					W->WriteValue(TEXT("category"), Entry.Category);
					W->WriteValue(TEXT("severity"), FAgentBridgeLogCapture::VerbosityToString(Entry.Verbosity));
					W->WriteValue(TEXT("message"), Entry.Message);
					W->WriteObjectEnd();
				}
				W->WriteArrayEnd();
			});

			SendJsonResponse(OnComplete, Json);
			return true;
		})
	);

	// ── POST /agent/build ────────────────────────────────────────────────

	BuildRouteHandle = Router->BindRoute(
		FHttpPath(TEXT("/agent/build")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			bool bWait = false;
			TSharedPtr<FJsonObject> Body = ParseJsonBody(Request);
			if (Body.IsValid())
			{
				bWait = Body->GetBoolField(TEXT("wait"));
			}

#if PLATFORM_WINDOWS
			// Try Live Coding first (Windows only)
			ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>("LiveCoding");
			if (LiveCoding && LiveCoding->IsEnabledForSession())
			{
				if (bWait)
				{
					ELiveCodingCompileResult LCResult;
					bool bStarted = LiveCoding->Compile(ELiveCodingCompileFlags::WaitForCompletion, &LCResult);

					if (!bStarted)
					{
						SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Live Coding compile could not start")));
						return true;
					}

					FString Status;
					bool bLCSuccess = false;
					switch (LCResult)
					{
					case ELiveCodingCompileResult::Success:
						Status = TEXT("success"); bLCSuccess = true; break;
					case ELiveCodingCompileResult::NoChanges:
						Status = TEXT("no_changes"); bLCSuccess = true; break;
					case ELiveCodingCompileResult::Failure:
						Status = TEXT("failure"); break;
					case ELiveCodingCompileResult::Cancelled:
						Status = TEXT("cancelled"); break;
					case ELiveCodingCompileResult::CompileStillActive:
						Status = TEXT("compile_already_active"); break;
					default:
						Status = TEXT("failure"); break;
					}

					SendJsonResponse(OnComplete, MakeSuccessResponse([&Status, bLCSuccess](TSharedRef<TJsonWriter<>>& W)
					{
						W->WriteValue(TEXT("method"), TEXT("live_coding"));
						W->WriteValue(TEXT("status"), Status);
						W->WriteValue(TEXT("compile_success"), bLCSuccess);
					}));
					return true;
				}
				else
				{
					LiveCoding->Compile();
					SendJsonResponse(OnComplete, MakeSuccessResponse([](TSharedRef<TJsonWriter<>>& W)
					{
						W->WriteValue(TEXT("method"), TEXT("live_coding"));
						W->WriteValue(TEXT("status"), TEXT("compile_started"));
					}));
					return true;
				}
			}
#endif // PLATFORM_WINDOWS

			// Fallback to Hot Reload
			if (IHotReloadInterface* HotReload = IHotReloadInterface::GetPtr())
			{
				EHotReloadFlags Flags = bWait ? EHotReloadFlags::WaitForCompletion : EHotReloadFlags::None;
				ECompilationResult::Type CompResult = HotReload->DoHotReloadFromEditor(Flags);

				FString Status;
				bool bCompileSuccess = false;
				if (!bWait)
				{
					Status = TEXT("compile_started");
				}
				else
				{
					switch (CompResult)
					{
					case ECompilationResult::Succeeded:
						Status = TEXT("success"); bCompileSuccess = true; break;
					case ECompilationResult::UpToDate:
						Status = TEXT("up_to_date"); bCompileSuccess = true; break;
					case ECompilationResult::Canceled:
						Status = TEXT("cancelled"); break;
					default:
						Status = TEXT("failure"); break;
					}
				}

				SendJsonResponse(OnComplete, MakeSuccessResponse([&Status, bCompileSuccess](TSharedRef<TJsonWriter<>>& W)
				{
					W->WriteValue(TEXT("method"), TEXT("hot_reload"));
					W->WriteValue(TEXT("status"), Status);
					W->WriteValue(TEXT("compile_success"), bCompileSuccess);
				}));
				return true;
			}

			SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Neither Live Coding nor Hot Reload is available")));
			return true;
		})
	);

	// ── POST /agent/batch ───────────────────────────────────────────────

	BatchRouteHandle = Router->BindRoute(
		FHttpPath(TEXT("/agent/batch")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			TSharedPtr<FJsonObject> Body = ParseJsonBody(Request);
			if (!Body)
			{
				SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Invalid JSON body")));
				return true;
			}

			const TArray<TSharedPtr<FJsonValue>>* ScriptsArray = nullptr;
			if (!Body->TryGetArrayField(TEXT("scripts"), ScriptsArray) || !ScriptsArray)
			{
				SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Missing 'scripts' array")));
				return true;
			}

			bool bStopOnError = false;
			Body->TryGetBoolField(TEXT("stop_on_error"), bStopOnError);

			// Resume support: skip scripts before start_from index
			double StartFromDouble = 0.0;
			Body->TryGetNumberField(TEXT("start_from"), StartFromDouble);
			int32 StartFrom = FMath::Clamp(static_cast<int32>(StartFromDouble), 0, ScriptsArray->Num());

			IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
			if (!PythonPlugin)
			{
				SendJsonResponse(OnComplete, MakeErrorResponse(TEXT("Python scripting plugin is not available")));
				return true;
			}

			// Collect script entries on HTTP thread (lightweight parsing)
			struct FScriptEntry
			{
				FString Id;
				FString Script;
				bool bIsolated = false; // Private scope when true — prevents stale UWorld GC crash
			};
			TArray<FScriptEntry> Scripts;

			// Batch-level isolated flag (applies to all scripts unless overridden per-entry)
			bool bBatchIsolated = false;
			Body->TryGetBoolField(TEXT("isolated"), bBatchIsolated);

			for (int32 i = 0; i < ScriptsArray->Num(); ++i)
			{
				const TSharedPtr<FJsonObject>* ItemObj = nullptr;
				if ((*ScriptsArray)[i]->TryGetObject(ItemObj) && ItemObj && (*ItemObj).IsValid())
				{
					FScriptEntry Entry;
					Entry.Id = (*ItemObj)->GetStringField(TEXT("id"));
					if (Entry.Id.IsEmpty())
					{
						Entry.Id = FString::Printf(TEXT("%d"), i);
					}
					Entry.Script = (*ItemObj)->GetStringField(TEXT("script"));
					// Per-entry "isolated" overrides batch-level flag
					bool bEntryIsolated = bBatchIsolated;
					(*ItemObj)->TryGetBoolField(TEXT("isolated"), bEntryIsolated);
					Entry.bIsolated = bEntryIsolated;
					Scripts.Add(MoveTemp(Entry));
				}
			}

			// Already on game thread — execute Python directly
			bool bAllSuccess = true;
			bool bSkipRest = false;
			int32 LastSuccessfulIndex = -1;

			struct FScriptResult
			{
				FString Id;
				bool bSuccess;
				FString Output;
				FString Result;
				bool bSkipped;
			};
			TArray<FScriptResult> Results;

			for (int32 ScriptIdx = 0; ScriptIdx < Scripts.Num(); ++ScriptIdx)
			{
				const FScriptEntry& Entry = Scripts[ScriptIdx];
				FScriptResult Res;
				Res.Id = Entry.Id;
				Res.bSkipped = false;

				// Skip scripts before start_from checkpoint
				if (ScriptIdx < StartFrom)
				{
					Res.bSuccess = true;
					Res.bSkipped = true;
					Results.Add(MoveTemp(Res));
					LastSuccessfulIndex = ScriptIdx;
					continue;
				}

				if (bSkipRest)
				{
					Res.bSuccess = false;
					Res.bSkipped = true;
					Results.Add(MoveTemp(Res));
					continue;
				}

				FPythonCommandEx PythonCommand;
				PythonCommand.Command = Entry.Script;
				PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
				PythonCommand.FileExecutionScope = Entry.bIsolated
					? EPythonFileExecutionScope::Private
					: EPythonFileExecutionScope::Public;

				Res.bSuccess = PythonPlugin->ExecPythonCommandEx(PythonCommand);

				// Always gc.collect() after each script to release stale UObject refs.
				{
					FPythonCommandEx GcCommand;
					GcCommand.Command = TEXT("import gc; gc.collect()");
					GcCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteStatement;
					PythonPlugin->ExecPythonCommandEx(GcCommand);
				}

				FString CombinedOutput;
				for (const FPythonLogOutputEntry& LogEntry : PythonCommand.LogOutput)
				{
					if (!CombinedOutput.IsEmpty())
					{
						CombinedOutput += TEXT("\n");
					}
					CombinedOutput += LogEntry.Output;
				}

				Res.Output = CombinedOutput;
				Res.Result = PythonCommand.CommandResult;

				if (Res.bSuccess)
				{
					LastSuccessfulIndex = ScriptIdx;
				}
				else
				{
					bAllSuccess = false;
					if (bStopOnError)
					{
						bSkipRest = true;
					}
				}

				Results.Add(MoveTemp(Res));
			}

			// Build response
			FString ResponseStr;
			TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&ResponseStr);
			JsonWriter->WriteObjectStart();
			JsonWriter->WriteValue(TEXT("success"), bAllSuccess);
			JsonWriter->WriteValue(TEXT("last_successful_index"), static_cast<double>(LastSuccessfulIndex));
			JsonWriter->WriteValue(TEXT("total"), static_cast<double>(Scripts.Num()));
			JsonWriter->WriteArrayStart(TEXT("results"));
			for (const FScriptResult& Res : Results)
			{
				JsonWriter->WriteObjectStart();
				JsonWriter->WriteValue(TEXT("id"), Res.Id);
				JsonWriter->WriteValue(TEXT("success"), Res.bSuccess);
				JsonWriter->WriteValue(TEXT("output"), Res.Output);
				JsonWriter->WriteValue(TEXT("result"), Res.Result);
				if (Res.bSkipped)
				{
					JsonWriter->WriteValue(TEXT("skipped"), true);
				}
				JsonWriter->WriteObjectEnd();
			}
			JsonWriter->WriteArrayEnd();
			JsonWriter->WriteObjectEnd();
			JsonWriter->Close();

			SendJsonResponse(OnComplete, ResponseStr);
			return true;
		})
	);

	// ── Start listeners ──────────────────────────────────────────────────

	HttpModule.StartAllListeners();

	UE_LOG(LogAgentBridge, Log, TEXT("HTTP server listening on port %d"), Port);

	// Write port file so external tools can discover the port
	WritePortFile();
}

// ────────────────────────────────────────────────────────────────────────────
// ShutdownModule
// ────────────────────────────────────────────────────────────────────────────

void FAgentBridgeModule::ShutdownModule()
{
	// Remove port file
	RemovePortFile();

	// Remove log capture before unbinding routes
	if (LogCapture.IsValid())
	{
		GLog->RemoveOutputDevice(LogCapture.Get());
		LogCapture.Reset();
	}

	FHttpServerModule& HttpModule = FHttpServerModule::Get();
	TSharedPtr<IHttpRouter> Router = HttpModule.GetHttpRouter(Port);

	if (Router)
	{
		Router->UnbindRoute(HealthRouteHandle);
		Router->UnbindRoute(ExecuteRouteHandle);
		Router->UnbindRoute(PlayRouteHandle);
		Router->UnbindRoute(LaunchRouteHandle);
		Router->UnbindRoute(DevicesRouteHandle);
		Router->UnbindRoute(ConfigsRouteHandle);
		Router->UnbindRoute(LogsRouteHandle);
		Router->UnbindRoute(BuildRouteHandle);
		Router->UnbindRoute(BatchRouteHandle);
	}

	UE_LOG(LogAgentBridge, Log, TEXT("HTTP server stopped"));
}

// ────────────────────────────────────────────────────────────────────────────
// Port file: write/remove a small file so external scripts can discover our port
// File location: <ProjectDir>/Saved/AgentBridge.port
// ────────────────────────────────────────────────────────────────────────────

void FAgentBridgeModule::WritePortFile()
{
	FString PortFilePath = FPaths::ProjectSavedDir() / TEXT("AgentBridge.port");
	FString PortString = FString::Printf(TEXT("%d"), Port);

	if (FFileHelper::SaveStringToFile(PortString, *PortFilePath))
	{
		UE_LOG(LogAgentBridge, Log, TEXT("Wrote port file: %s"), *PortFilePath);
	}
	else
	{
		UE_LOG(LogAgentBridge, Warning, TEXT("Failed to write port file: %s"), *PortFilePath);
	}
}

void FAgentBridgeModule::RemovePortFile()
{
	FString PortFilePath = FPaths::ProjectSavedDir() / TEXT("AgentBridge.port");

	if (FPaths::FileExists(PortFilePath))
	{
		IFileManager::Get().Delete(*PortFilePath);
		UE_LOG(LogAgentBridge, Log, TEXT("Removed port file: %s"), *PortFilePath);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAgentBridgeModule, AgentBridge)
