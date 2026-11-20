// Diego Gallo All Rights Reserved

using UnrealBuildTool;

public class FrontendUI : ModuleRules
{
	public FrontendUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"UMG",
			"CommonUI",
			"CommonInput",
			"DeveloperSettings",
			"PropertyPath",
			"PreLoadScreen"
		});

		// FrontendUIStartup: the startup logo (pre-load screen); asked whether the logo was already shown.
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "FrontendUIStartup" });
	}
}
