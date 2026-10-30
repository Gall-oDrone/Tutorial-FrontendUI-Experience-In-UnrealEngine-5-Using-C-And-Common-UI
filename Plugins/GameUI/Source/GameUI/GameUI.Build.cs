using UnrealBuildTool;

public class GameUI : ModuleRules
{
	public GameUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG", "CommonUI", "CommonInput", "GameplayTags", "DeveloperSettings", "NetCore",
			"FrontendUI", "FrontendMultiplayer"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
