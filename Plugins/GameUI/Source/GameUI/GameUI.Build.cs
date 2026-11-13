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

		// CoreOnline: FUniqueNetIdWrapper::ToString (the unique id handed to VerifyPlayerSession).
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "CoreOnline" });
	}
}
