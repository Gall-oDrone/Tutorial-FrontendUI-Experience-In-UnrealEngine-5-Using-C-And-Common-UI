using UnrealBuildTool;

public class FrontendMultiplayerOSS : ModuleRules
{
	public FrontendMultiplayerOSS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// OnlineSubsystem is public because the backend header names IOnlineSession
		// delegate types. Nothing else in the plugin depends on this module, so the
		// core FrontendMultiplayer module stays free of online-service dependencies.
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "OnlineSubsystem", "FrontendMultiplayer"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystemUtils" });
	}
}
