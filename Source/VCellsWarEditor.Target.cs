// Copyright (c) 2026, Dmitry Tur. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class VCellsWarEditorTarget : TargetRules
{
	public VCellsWarEditorTarget(TargetInfo Target) : base(Target)
	{
		bUsesSteam = true;
		
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		ExtraModuleNames.AddRange( new string[] { "VCellsWar" } );
	}
}
