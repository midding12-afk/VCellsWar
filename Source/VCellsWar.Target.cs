// Copyright (c) 2026, Dmitry Tur. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class VCellsWarTarget : TargetRules
{
	public VCellsWarTarget(TargetInfo Target) : base(Target)
	{
		bUsesSteam = true;
		
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		ExtraModuleNames.AddRange( new string[] { "VCellsWar" } );
	}
}
