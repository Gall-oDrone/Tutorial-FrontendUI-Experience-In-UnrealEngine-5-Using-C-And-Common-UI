// Diego Gallo All Rights Reserved

using UnrealBuildTool;

/**
 * The project's primary game module. Deliberately empty: all code lives in the three
 * plugins. A monolithic (packaged) executable still needs exactly one module that
 * declares IMPLEMENT_PRIMARY_GAME_MODULE, which is what this module provides. It is
 * also the natural home for code that is truly specific to this project and does not
 * belong in a reusable plugin.
 */
public class FrontendUIGame : ModuleRules
{
	public FrontendUIGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
	}
}
