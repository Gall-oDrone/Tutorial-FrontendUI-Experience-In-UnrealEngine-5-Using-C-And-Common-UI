// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrontendLeaderboardTypes.h"
#include "FrontendMultiplayerSubsystem.generated.h"

class UFrontendLeaderboardEntryData;
class UFrontendSessionListEntryData;

/**
 * Session host/find/join entry point for the multiplayer menu flow, plus
 * leaderboard retrieval. Every call is stubbed with simulated logs and fake
 * data so the UI can be exercised before real networking / GameLift lands.
 */
UCLASS()
class FRONTENDMULTIPLAYER_API UFrontendMultiplayerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UFrontendMultiplayerSubsystem* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void CreateSession(FName SessionName, int32 MaxPlayers);

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void FindSessions();

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void JoinSession(int32 SessionIndex);

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void FetchLeaderboard(ELeaderboardCategory Category = ELeaderboardCategory::Main);

	/** Every line logged so far, oldest first, capped by UFrontendMultiplayerSettings::APIDebugLogHistorySize. */
	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer|Debug")
	TArray<FString> GetAPIDebugLogHistory() const { return APIDebugLogHistory; }

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer|Debug")
	void ClearAPIDebugLog();

	/** Fired per line as it is logged. Read GetAPIDebugLogHistory() for lines logged before subscribing. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAPIDebugLogUpdated, const FString&, LogLine);
	UPROPERTY(BlueprintAssignable)
	FOnAPIDebugLogUpdated OnAPIDebugLogUpdated;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAPIDebugLogCleared);
	UPROPERTY(BlueprintAssignable)
	FOnAPIDebugLogCleared OnAPIDebugLogCleared;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionListUpdated, const TArray<UFrontendSessionListEntryData*>&, Sessions);
	UPROPERTY(BlueprintAssignable)
	FOnSessionListUpdated OnSessionListUpdated;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLeaderboardUpdated, ELeaderboardCategory, Category, const TArray<UFrontendLeaderboardEntryData*>&, Entries);
	UPROPERTY(BlueprintAssignable)
	FOnLeaderboardUpdated OnLeaderboardUpdated;

private:
	void BroadcastDebugLog(const FString& LogLine);

	UPROPERTY(Transient)
	TArray<FString> APIDebugLogHistory;
};
