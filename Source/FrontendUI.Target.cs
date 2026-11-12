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

		// Keep UE_LOG alive in Shipping so a packaged client writes Saved/Logs/FrontendUI.log and a
		// failed HTTPS call (libcurl error 60 and friends) can be diagnosed; see Docs/GameLift.md §4.
		// A Launcher engine cannot give a game target a unique build environment, so the setting is
		// forced there (game and plugin modules log, the prebuilt engine modules stay silent); a
		// source engine simply recompiles its Shipping modules with logging on.
		bUseLoggingInShipping = true;
		if (Unreal.IsEngineInstalled())
		{
			bOverrideBuildEnvironment = true;
		}
		else
		{
			BuildEnvironment = TargetBuildEnvironment.Unique;
		}
	}
}
