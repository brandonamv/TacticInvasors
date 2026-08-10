// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TacticInvasors : ModuleRules
{
	public TacticInvasors(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG" }); // Se añadió "UMG"

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystemSteam");

		// To include OnlineSubsystemSteam, add it to the plugins section in uproject file with a dependency on OnlineSubsystem,
		// and add PublicDependencyModuleNames.Add("OnlineSubsystemSteam");
	}
}
