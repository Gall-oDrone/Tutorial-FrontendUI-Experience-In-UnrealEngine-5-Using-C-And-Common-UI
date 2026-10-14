// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "FrontendLeaderboardTypes.generated.h"

/**
 * Which leaderboard a tab shows. One entry today; Ranked / Unranked (or any other
 * split) are added here and as rows in the screen's LeaderboardTabs defaults.
 */
UENUM(BlueprintType)
enum class ELeaderboardCategory : uint8
{
	Main UMETA(DisplayName = "Leaderboard")
};

UENUM(BlueprintType)
enum class ELeaderboardSortMode : uint8
{
	ScoreDescending UMETA(DisplayName = "Score: High to Low"),
	ScoreAscending  UMETA(DisplayName = "Score: Low to High")
};

/** One tab on the leaderboard screen. */
USTRUCT(BlueprintType)
struct FLeaderboardTabDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leaderboard Tab")
	FName TabID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leaderboard Tab")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leaderboard Tab")
	ELeaderboardCategory Category = ELeaderboardCategory::Main;
};
