// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FrontendMultiplayerSettings.generated.h"

class UFrontendSessionBackend;

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
	 * simulated stub. Point it at an OnlineSubsystem / EOS / GameLift implementation
	 * to go live; no screen changes are needed.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Multiplayer", meta = (EditCondition = "bEnableMultiplayer"))
	TSoftClassPtr<UFrontendSessionBackend> SessionBackendClass;

	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableMultiplayer"))
	bool bShowAPIDebugPanel = false;

	/** Oldest lines are dropped once the API debug log exceeds this many entries. */
	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableMultiplayer", ClampMin = "1", UIMin = "1"))
	int32 APIDebugLogHistorySize = 200;
};
