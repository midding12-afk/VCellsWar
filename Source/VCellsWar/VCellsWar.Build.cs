// Copyright (c) 2026, Dmitry Tur. All rights reserved.

using UnrealBuildTool;

public class VCellsWar : ModuleRules
{
	public VCellsWar(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "OnlineSubsystem", "OnlineSubsystemSteam", "OnlineSubsystemUtils", "SteamSockets", "NetCore", "NavigationSystem","StateTreeModule","GameplayStateTreeModule","StructUtils","GameplayTags","EnhancedInput","GameplayAbilities","GameplayTasks","Niagara"});

		//PrivateDependencyModuleNames.AddRange(new string[] { "EnhancedInput" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
