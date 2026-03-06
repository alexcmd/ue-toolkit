// Copyright Anthropic, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AgentBridge : ModuleRules
{
	public AgentBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"HTTPServer",
				"Sockets",
				"Networking",
				"Json",
				"JsonUtilities",
				"PythonScriptPlugin",
				"UnrealEd",
				"TargetPlatform",
				"TargetDeviceServices",
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateDependencyModuleNames.Add("LiveCoding");
		}
	}
}
