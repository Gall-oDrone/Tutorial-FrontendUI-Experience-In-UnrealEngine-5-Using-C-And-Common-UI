// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrontendLeaderboardTypes.h"
#include "FrontendMultiplayerSubsystem.generated.h"

class UFrontendLeaderboardEntryData;
class UFrontendSessionListEntryData;
class UFrontendSessionBackend;

/**
 * Session host/find/join entry point for the multiplayer menu flow, plus
 * leaderboard retrieval. The screens talk only to this subsystem; it forwards every
 * request to the UFrontendSessionBackend named in UFrontendMultiplayerSettings and
 * re-broadcasts the backend's results through the delegates below. The default
 * backend is a stub with simulated data.
 */
UCLASS()
class FRONTENDMULTIPLAYER_API UFrontendMultiplayerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UFrontendMultiplayerSubsystem* Get(const UObject* WorldContextObject);

	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void CreateSession(FName SessionName, int32 MaxPlayers);

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void FindSessions();

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void JoinSession(int32 SessionIndex);

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void FetchLeaderboard(ELeaderboardCategory Category = ELeaderboardCategory::Main);

	/** The backend currently servicing requests. Null only if construction failed outright. */
	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer")
	UFrontendSessionBackend* GetSessionBackend() const { return ActiveBackend; }

	/** Every line logged so far, oldest first, capped by UFrontendMultiplayerSettings::APIDebugLogHistorySize. */
	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer|Debug")
	TArray<FString> GetAPIDebugLogHistory() const { return APIDebugLogHistory; }

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer|Debug")
	void ClearAPIDebugLog();

	/** Append a line to the API debug log. Backends call this through UFrontendSessionBackend::LogDebug. */
	void BroadcastDebugLog(const FString& LogLine);

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
	void CreateSessionBackend();

	UPROPERTY(Transient)
	TObjectPtr<UFrontendSessionBackend> ActiveBackend;

	UPROPERTY(Transient)
	TArray<FString> APIDebugLogHistory;
};
