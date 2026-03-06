// Copyright Anthropic, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "HttpRouteHandle.h"

class FAgentBridgeLogCapture;

class FAgentBridgeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	// Existing routes
	FHttpRouteHandle HealthRouteHandle;
	FHttpRouteHandle ExecuteRouteHandle;

	// New routes
	FHttpRouteHandle PlayRouteHandle;
	FHttpRouteHandle LaunchRouteHandle;
	FHttpRouteHandle DevicesRouteHandle;
	FHttpRouteHandle ConfigsRouteHandle;
	FHttpRouteHandle LogsRouteHandle;
	FHttpRouteHandle BuildRouteHandle;
	FHttpRouteHandle BatchRouteHandle;

	TUniquePtr<FAgentBridgeLogCapture> LogCapture;

	uint32 Port = 13090;

	void WritePortFile();
	void RemovePortFile();
};
