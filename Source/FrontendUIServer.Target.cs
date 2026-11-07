// Diego Gallo All Rights Reserved

using UnrealBuildTool;
using System.Collections.Generic;

/**
 * Dedicated server target. Needs a source-built engine: installed (launcher) builds skip
 * Server targets when generating project files and refuse to build them. For local tests
 * on a launcher engine run the editor binary as a server instead
 * (Scripts\LaunchDedicatedServer.bat). See Docs/DedicatedServer.md.
 */
public class FrontendUIServerTarget : TargetRules
{
	public FrontendUIServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;

		// Primary game module; the plugins are pulled in through the .uproject.
		ExtraModuleNames.Add("FrontendUIGame");
	}
}
