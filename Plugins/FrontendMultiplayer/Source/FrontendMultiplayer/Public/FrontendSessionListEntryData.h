// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FrontendSessionListEntryData.generated.h"

class UTexture2D;

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

	UPROPERTY(BlueprintReadOnly)
	FString HostName;

	UPROPERTY(BlueprintReadOnly)
	FString MapName;

	UPROPERTY(BlueprintReadOnly)
	int32 PingInMs = 0;

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> PreviewImage;

	bool IsFull() const { return CurrentPlayers >= MaxPlayers; }
};
