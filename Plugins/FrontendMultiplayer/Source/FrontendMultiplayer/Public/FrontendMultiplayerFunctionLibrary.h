// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FrontendMultiplayerSettings.h"
#include "FrontendMultiplayerFunctionLibrary.generated.h"

/**
 * Blueprint-facing helpers for Frontend Multiplayer project settings.
 */
UCLASS()
class FRONTENDMULTIPLAYER_API UFrontendMultiplayerFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer")
	static bool IsMultiplayerEnabled() { return UFrontendMultiplayerSettings::Get()->bEnableMultiplayer; }

	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer")
	static bool ShouldShowAPIDebugPanel() { return UFrontendMultiplayerSettings::Get()->bShowAPIDebugPanel; }
};
