// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Backends/FrontendSessionBackend.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "FrontendSessionBackend_OSS.generated.h"

class FOnlineSessionSearch;
class FUniqueNetId;

/**
 * A request parked until a prerequisite finishes: a sign-in, or destroying a stale
 * NAME_GameSession. Not reflected.
 */
enum class EFrontendOSSDeferredRequest : uint8
{
	None,
	Create,
	Find,
	Join
};

/**
 * Session backend on the legacy OnlineSubsystem (IOnlineSession + IOnlineIdentity).
 * Works unchanged on top of the Null subsystem (LAN), Steam or EOS; which one is
 * chosen by [OnlineSubsystem] DefaultPlatformService in DefaultEngine.ini, not by
 * this class.
 *
 * The engine-level session is always NAME_GameSession so AGameSession's player
 * registration lines up. The name typed on the Host screen travels as a custom
 * session setting and is what the server browser displays.
 *
 * Sign-in is lazy: Host / Find / Join first make sure local user 0 is logged in on
 * subsystems that need it (anything but Null), using the credential chain in
 * UFrontendMultiplayerOSSSettings. Login() does the same explicitly.
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
	virtual bool IsLoggedIn() const override;
	virtual void Login() override;
	virtual FString GetLocalPlayerDisplayName() const override;
	virtual bool GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const override;
	virtual FString GetHostTravelOptions() const override;
	//~ End UFrontendSessionBackend Interface

private:
	/**
	 * Resolved per call rather than cached: in PIE each instance has its own online
	 * subsystem keyed by world context, and the world does not exist yet when the
	 * owning subsystem initialises.
	 */
	IOnlineSessionPtr GetSessionInterface() const;
	IOnlineIdentityPtr GetIdentityInterface() const;
	UWorld* GetWorldForOnline() const;
	FName GetSubsystemName() const;
	bool IsLAN() const;

	/** Null advertises by LAN broadcast and has no account; every other subsystem needs a signed-in user. */
	bool RequiresLogin() const { return !IsLAN(); }

	//~ Begin sign-in
	/**
	 * Run Request now if user 0 is signed in, otherwise sign in first and run it from
	 * HandleLoginComplete. A sign-in already in flight simply adopts the new Request.
	 */
	void EnsureLoggedInThen(EFrontendOSSDeferredRequest Request);
	void StartLogin(const FOnlineAccountCredentials& Credentials);
	void FinishLogin(bool bWasSuccessful, const FString& Error);
	void ContinueAfterLogin(EFrontendOSSDeferredRequest Request);
	void FailDeferredRequest(EFrontendOSSDeferredRequest Request, const FText& ErrorText);

	/**
	 * Build the credential chain from UFrontendMultiplayerOSSSettings and the command
	 * line. Returns the first credentials to try; OutFallback is set when a second
	 * attempt should follow a failure (persistentauth -> configured type).
	 */
	FOnlineAccountCredentials BuildCredentials(bool& bOutHasFallback, FOnlineAccountCredentials& OutFallback) const;

	void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void HandleLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId);
	//~ End sign-in

	void CreateSessionAfterLogin();
	void CreateSessionNow();
	void FindSessionsNow();
	void JoinSessionAfterLogin();
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
	FDelegateHandle LoginCompleteHandle;
	FDelegateHandle LoginStatusChangedHandle;

	/** Results of the last FindSessions; JoinSession indexes into this. */
	TSharedPtr<FOnlineSessionSearch> LastSearch;

	EFrontendOSSDeferredRequest DeferredRequest = EFrontendOSSDeferredRequest::None;
	FString PendingDisplayName;
	int32 PendingMaxPlayers = 0;
	int32 PendingJoinIndex = INDEX_NONE;

	/** Sign-in state. A second attempt with FallbackCredentials follows a failed first one. */
	bool bLoginInFlight = false;
	bool bHasFallbackCredentials = false;
	FOnlineAccountCredentials FallbackCredentials;
	FString CurrentCredentialType;
	int32 LoginAttemptSerial = 0;
};
