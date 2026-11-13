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
		// The setting changes a global define, which UBT only allows with a unique build environment
		// or when forced; forcing works on a Launcher engine too (game and plugin modules log, the
		// prebuilt engine modules stay silent), while a source engine recompiles everything with it.
		bUseLoggingInShipping = true;
		bOverrideBuildEnvironment = true;
	}
}
