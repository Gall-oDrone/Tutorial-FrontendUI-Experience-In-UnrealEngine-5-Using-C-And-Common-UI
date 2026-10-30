// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FrontendMultiplayerSettings.generated.h"

class UFrontendSessionBackend;
class UWorld;

/**
 * Project settings that gate the multiplayer menu flow and its debug tooling.
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Frontend Multiplayer Settings"))
class FRONTENDMULTIPLAYER_API UFrontendMultiplayerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFrontendMultiplayerSettings();

	static const UFrontendMultiplayerSettings* Get() { return GetDefault<UFrontendMultiplayerSettings>(); }

	UPROPERTY(Config, EditAnywhere, Category = "Multiplayer")
	bool bEnableMultiplayer = false;

	/**
	 * Class that hosts, finds and joins sessions on behalf of the UI. Defaults to the
	 * simulated stub. Point it at UFrontendSessionBackend_OSS (FrontendMultiplayerOSS
	 * module) or your own implementation to go live; no screen changes are needed.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Multiplayer", meta = (EditCondition = "bEnableMultiplayer"))
	TSoftClassPtr<UFrontendSessionBackend> SessionBackendClass;

	/**
	 * Map the host travels to, as a listen server, once its session has been created.
	 * Joining clients follow the host there. Must use a game mode that handles networked
	 * players (for this project, BP_MatchGameMode).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Session", meta = (EditCondition = "bEnableMultiplayer"))
	TSoftObjectPtr<UWorld> HostMap;

	/** Whether players may join a session after it has started. Mirrored into the online session settings. */
	UPROPERTY(Config, EditAnywhere, Category = "Session", meta = (EditCondition = "bEnableMultiplayer"))
	bool bAllowJoinInProgress = true;

	/** Upper bound on rows a session search may return. */
	UPROPERTY(Config, EditAnywhere, Category = "Session", meta = (EditCondition = "bEnableMultiplayer", ClampMin = "1", UIMin = "1", UIMax = "200"))
	int32 MaxSearchResults = 50;

	/**
	 * If the active backend's online service has no leaderboard support (the Null
	 * subsystem, for one), show simulated rows instead of an empty list.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Leaderboard", meta = (EditCondition = "bEnableMultiplayer"))
	bool bSimulateLeaderboardWhenUnsupported = true;

	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableMultiplayer"))
	bool bShowAPIDebugPanel = false;

	/** Oldest lines are dropped once the API debug log exceeds this many entries. */
	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableMultiplayer", ClampMin = "1", UIMin = "1"))
	int32 APIDebugLogHistorySize = 200;
};
