// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrontendLeaderboardTypes.h"
#include "FrontendSessionTypes.h"
#include "FrontendMultiplayerSubsystem.generated.h"

class UFrontendLeaderboardEntryData;
class UFrontendSessionListEntryData;
class UFrontendSessionBackend;

/**
 * Session host/find/join entry point for the multiplayer menu flow, plus
 * leaderboard retrieval. The screens talk only to this subsystem; it forwards every
 * request to the UFrontendSessionBackend named in UFrontendMultiplayerSettings and
 * re-broadcasts the backend's results through the delegates below.
 *
 * The subsystem also owns what happens after a request succeeds: a hosted session
 * server-travels to UFrontendMultiplayerSettings::HostMap as a listen server, a
 * joined session client-travels to the connect string the backend resolved. Failures
 * surface as an Ok confirm screen on the Modal stack and as OnSessionOperationFailed.
 *
 * The default backend is a stub with simulated data.
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

	/**
	 * Leave or destroy the current session, if any. Call before travelling back to the
	 * frontend so the online service lets this player host or join again.
	 */
	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void LeaveSession();

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer")
	void FetchLeaderboard(ELeaderboardCategory Category = ELeaderboardCategory::Main);

	/** The backend currently servicing requests. Null only if construction failed outright. */
	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer")
	UFrontendSessionBackend* GetSessionBackend() const { return ActiveBackend; }

	/** The request in flight, or None. Create/Find/Join are ignored while one is running. */
	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer")
	EFrontendSessionOperation GetCurrentOperation() const { return CurrentOperation; }

	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer")
	bool IsSessionOperationInProgress() const { return CurrentOperation != EFrontendSessionOperation::None; }

	/** Name and capacity of the session this player hosts or joined. False if there is none. */
	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer")
	bool GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const;

	/** Every line logged so far, oldest first, capped by UFrontendMultiplayerSettings::APIDebugLogHistorySize. */
	UFUNCTION(BlueprintPure, Category = "Frontend Multiplayer|Debug")
	TArray<FString> GetAPIDebugLogHistory() const { return APIDebugLogHistory; }

	UFUNCTION(BlueprintCallable, Category = "Frontend Multiplayer|Debug")
	void ClearAPIDebugLog();

	/** Append a line to the API debug log. Backends call this through UFrontendSessionBackend::LogDebug. */
	void BroadcastDebugLog(const FString& LogLine);

	//~ Begin backend result sink. Called only through UFrontendSessionBackend::Report*.
	void NotifyOperationStarted(EFrontendSessionOperation Operation);
	void NotifySessionCreated(bool bSucceeded, const FText& ErrorText);
	void NotifySessionsFound(const TArray<UFrontendSessionListEntryData*>& Sessions);
	void NotifyFindSessionsFailed(const FText& ErrorText);
	void NotifySessionJoined(bool bSucceeded, const FString& ConnectString, const FText& ErrorText);
	void NotifySessionLeft();
	//~ End backend result sink.

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

	/** Fired when a request starts (bInProgress = true) and when it finishes either way (false). */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionOperationChanged, EFrontendSessionOperation, Operation, bool, bInProgress);
	UPROPERTY(BlueprintAssignable)
	FOnSessionOperationChanged OnSessionOperationChanged;

	/** Fired after a request fails. The subsystem has already shown an Ok confirm screen with the same text. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSessionOperationFailed, EFrontendSessionOperation, Operation, const FText&, Title, const FText&, Message);
	UPROPERTY(BlueprintAssignable)
	FOnSessionOperationFailed OnSessionOperationFailed;

private:
	void CreateSessionBackend();

	/** True if a request may start now; otherwise logs why it was ignored. */
	bool CanStartOperation(EFrontendSessionOperation Operation);

	void FinishOperation(EFrontendSessionOperation Operation);
	void FailOperation(EFrontendSessionOperation Operation, const FText& Title, const FText& Message);

	void TravelToHostMap();
	void TravelToConnectString(const FString& ConnectString);

	UPROPERTY(Transient)
	TObjectPtr<UFrontendSessionBackend> ActiveBackend;

	UPROPERTY(Transient)
	TArray<FString> APIDebugLogHistory;

	EFrontendSessionOperation CurrentOperation = EFrontendSessionOperation::None;
};
