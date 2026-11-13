using UnrealBuildTool;

public class FrontendMultiplayer : ModuleRules
{
	public FrontendMultiplayer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "UMG", "CommonUI", "CommonInput",
			"GameplayTags", "DeveloperSettings", "FrontendUI"
		});

		// CoreOnline: FUniqueNetIdWrapper::ToString (the local player's id sent as GameLift PlayerId).
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate", "SlateCore", "HTTP", "Json", "WebSockets", "CoreOnline"
		});
	}
}
