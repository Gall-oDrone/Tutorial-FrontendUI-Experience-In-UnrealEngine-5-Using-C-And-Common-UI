// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FrontendMultiplayerSettings.generated.h"

/**
 * Project settings that gate the multiplayer menu flow and its debug tooling.
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Frontend Multiplayer Settings"))
class FRONTENDMULTIPLAYER_API UFrontendMultiplayerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UFrontendMultiplayerSettings* Get() { return GetDefault<UFrontendMultiplayerSettings>(); }

	UPROPERTY(Config, EditAnywhere, Category = "Multiplayer")
	bool bEnableMultiplayer = false;

	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableMultiplayer"))
	bool bShowAPIDebugPanel = false;
};
