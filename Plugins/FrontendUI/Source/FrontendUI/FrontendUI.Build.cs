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

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
