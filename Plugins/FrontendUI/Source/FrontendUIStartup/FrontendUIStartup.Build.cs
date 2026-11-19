// Diego Gallo All Rights Reserved

using UnrealBuildTool;

// The part of the frontend framework that has to exist before the engine has finished
// starting: the studio logo shown while it does. No UObject types on purpose - it is loaded
// in the PreEarlyLoadingScreen phase, long before the rest of the plugin. That phase is only
// loaded by game clients and the editor; on a dedicated server and in commandlets the module
// is linked but never started, which is all the rest of the plugin needs from it there.
public class FrontendUIStartup : ModuleRules
{
	public FrontendUIStartup(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core" });

		PrivateDependencyModuleNames.AddRange(new string[] {
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"PreLoadScreen"
		});

		// The startup movies are handed on to the movie player after the logo; a dedicated
		// server shows neither (the code is behind !UE_SERVER).
		if (Target.Type != TargetType.Server)
		{
			PrivateDependencyModuleNames.Add("MoviePlayer");
		}
	}
}
