// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Backends/FrontendSessionBackend.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "FrontendSessionBackend_OSS.generated.h"

class FOnlineSessionSearch;

/** A request parked until a stale NAME_GameSession has been destroyed. Not reflected. */
enum class EFrontendOSSDeferredRequest : uint8
{
	None,
	Create,
	Join
};

/**
 * Session backend on the legacy OnlineSubsystem (IOnlineSession). Works unchanged on
 * top of the Null subsystem (LAN), Steam or EOS; which one is chosen by
 * [OnlineSubsystem] DefaultPlatformService in DefaultEngine.ini, not by this class.
 *
 * The engine-level session is always NAME_GameSession so AGameSession's player
 * registration lines up. The name typed on the Host screen travels as a custom
 * session setting and is what the server browser displays.
 *
 * Select it in Project Settings > Game > Frontend Multiplayer Settings >
 * Session Backend Class.
 */
UCLASS()
class FRONTENDMULTIPLAYEROSS_API UFrontendSessionBackend_OSS : public UFrontendSessionBackend
{
	GENERATED_BODY()

public:
	//~ Begin UFrontendSessionBackend Interface
	virtual void Shutdown() override;
	virtual void CreateSession(FName SessionName, int32 MaxPlayers) override;
	virtual void FindSessions() override;
	virtual void JoinSession(int32 SessionIndex) override;
	virtual void LeaveSession() override;
	virtual void FetchLeaderboard(ELeaderboardCategory Category) override;
	virtual FString GetBackendDisplayName() const override;
	//~ End UFrontendSessionBackend Interface

private:
	/**
	 * Resolved per call rather than cached: in PIE each instance has its own online
	 * subsystem keyed by world context, and the world does not exist yet when the
	 * owning subsystem initialises.
	 */
	IOnlineSessionPtr GetSessionInterface() const;
	UWorld* GetWorldForOnline() const;
	FName GetSubsystemName() const;
	bool IsLAN() const;

	void CreateSessionNow();
	void JoinSessionNow();
	void DestroyStaleSessionThen(EFrontendOSSDeferredRequest Request);
	void ClearDelegateHandles();

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle StartSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;

	/** Results of the last FindSessions; JoinSession indexes into this. */
	TSharedPtr<FOnlineSessionSearch> LastSearch;

	EFrontendOSSDeferredRequest DeferredRequest = EFrontendOSSDeferredRequest::None;
	FString PendingDisplayName;
	int32 PendingMaxPlayers = 0;
	int32 PendingJoinIndex = INDEX_NONE;
};
