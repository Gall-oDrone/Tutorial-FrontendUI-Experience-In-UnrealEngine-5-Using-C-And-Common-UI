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

	/** Development tooling only; always false in Shipping so it cannot leak into a release through config. */
	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer")
	static bool ShouldShowAPIDebugPanel()
	{
#if UE_BUILD_SHIPPING
		return false;
#else
		return UFrontendMultiplayerSettings::Get()->bShowAPIDebugPanel;
#endif
	}
};
