using UnrealBuildTool;

public class FrontendMultiplayer : ModuleRules
{
	public FrontendMultiplayer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "UMG", "CommonUI",
			"GameplayTags", "DeveloperSettings", "FrontendUI"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
