// Diego Gallo All Rights Reserved

using UnrealBuildTool;
using System.Collections.Generic;

public class FrontendUITarget : TargetRules
{
	public FrontendUITarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;

		// Primary game module; the plugins are pulled in through the .uproject.
		ExtraModuleNames.Add("FrontendUIGame");
	}
}
