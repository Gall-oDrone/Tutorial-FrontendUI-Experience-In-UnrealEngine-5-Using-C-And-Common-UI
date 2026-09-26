// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
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
	void FetchLeaderboard();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAPIDebugLogUpdated, const FString&, LogLine);
	UPROPERTY(BlueprintAssignable)
	FOnAPIDebugLogUpdated OnAPIDebugLogUpdated;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionListUpdated, const TArray<UFrontendSessionListEntryData*>&, Sessions);
	UPROPERTY(BlueprintAssignable)
	FOnSessionListUpdated OnSessionListUpdated;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeaderboardUpdated, const TArray<UFrontendLeaderboardEntryData*>&, Entries);
	UPROPERTY(BlueprintAssignable)
	FOnLeaderboardUpdated OnLeaderboardUpdated;

private:
	void BroadcastDebugLog(const FString& LogLine);
};
