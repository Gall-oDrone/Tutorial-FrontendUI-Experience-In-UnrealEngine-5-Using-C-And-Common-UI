// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FrontendSessionListEntryData.generated.h"

/**
 * Read-only data for one found session row in the server browser.
 * Intentionally not a UListDataObject_Base — that type carries options-system
 * reset/dependency machinery that does not apply to session listings.
 */
UCLASS(BlueprintType)
class FRONTENDMULTIPLAYER_API UFrontendSessionListEntryData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FString SessionDisplayName;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;
};
