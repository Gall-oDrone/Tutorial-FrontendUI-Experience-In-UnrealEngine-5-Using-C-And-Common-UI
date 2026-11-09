// Diego Gallo All Rights Reserved


#include "FrontendUIGame.h"
#include "Misc/Crc.h"
#include "Misc/NetworkVersion.h"

void FFrontendUIGameModule::StartupModule()
{
	// Replace the engine-changelist-based network version with one derived from the project
	// version, so Launcher-built clients and source-built servers of the same project
	// revision can talk to each other. See the class comment in FrontendUIGame.h.
	FNetworkVersion::GetLocalNetworkVersionOverride.BindStatic(&FFrontendUIGameModule::GetProjectNetworkVersion);
}

void FFrontendUIGameModule::ShutdownModule()
{
	FNetworkVersion::GetLocalNetworkVersionOverride.Unbind();
}

uint32 FFrontendUIGameModule::GetProjectNetworkVersion()
{
	// ProjectVersion comes from [/Script/EngineSettings.GeneralProjectSettings] in
	// DefaultGame.ini. The salt keeps us from colliding with another project that happens to
	// use the same scheme and version string.
	const FString ProjectVersion = FNetworkVersion::GetProjectVersion();
	const FString Salted = TEXT("FrontendUI:") + ProjectVersion;
	return FCrc::StrCrc32(*Salted);
}

// The third argument is the game name the engine reports (GInternalProjectName); it
// matches the .uproject, not the module.
IMPLEMENT_PRIMARY_GAME_MODULE(FFrontendUIGameModule, FrontendUIGame, "FrontendUI");
