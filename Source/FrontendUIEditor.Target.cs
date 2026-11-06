// Diego Gallo All Rights Reserved

using UnrealBuildTool;
using System.Collections.Generic;

public class FrontendUIEditorTarget : TargetRules
{
	public FrontendUIEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;

		// Primary game module; the plugins are pulled in through the .uproject.
		ExtraModuleNames.Add("FrontendUIGame");
	}
}
