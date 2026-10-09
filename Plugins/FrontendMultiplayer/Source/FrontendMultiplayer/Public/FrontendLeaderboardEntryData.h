// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FrontendLeaderboardEntryData.generated.h"

/**
 * Read-only data for one leaderboard row.
 * Mirrors UFrontendSessionListEntryData: a plain UObject rather than a
 * UListDataObject_Base, since the options-system reset/dependency machinery
 * does not apply to a read-only ranking row.
 */
UCLASS(BlueprintType)
class FRONTENDMULTIPLAYER_API UFrontendLeaderboardEntryData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly)
	FString PlayerDisplayName;

	UPROPERTY(BlueprintReadOnly)
	int32 Score = 0;

	/** True for the row belonging to the local player, so the row can highlight itself. */
	UPROPERTY(BlueprintReadOnly)
	bool bIsLocalPlayer = false;
};
