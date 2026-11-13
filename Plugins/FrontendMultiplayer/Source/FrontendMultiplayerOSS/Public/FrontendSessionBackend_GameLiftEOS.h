// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Backends/FrontendSessionBackend_GameLift.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Containers/Ticker.h"
#include "FrontendSessionBackend_GameLiftEOS.generated.h"

/**
 * Amazon GameLift sessions for a player signed in to Epic Online Services.
 *
 * Everything GameLift (Host / Find / Join, the dedicated-server WebSocket, player-session
 * validation) is inherited from UFrontendSessionBackend_GameLift. This class adds the
 * identity half the Shipping path needs:
 *
 *   - local user 0 is signed in on the EOS OnlineSubsystem before Host / Find / Join run,
 *     using the same credential chain as UFrontendSessionBackend_OSS (command line for
 *     LaunchInstances.bat / Dev Auth Tool, persistentauth, Project Settings);
 *   - the same sign-in runs before the leaderboard is read through ApiEndpointUrl: a proxy
 *     that authenticates players refuses the read without a token, and needs the account to
 *     tell which row is the player's own;
 *   - the user's EOS access token is attached as `Authorization: Bearer ...` to every call
 *     the client makes to ApiEndpointUrl, where the proxy verifies it with Epic and derives
 *     the GameLift PlayerId from the verified account.
 *
 * Needs [OnlineSubsystem] DefaultPlatformService=EOS (LaunchInstances.bat adds it with
 * GAMELIFT_AUTH=EOS). On a dedicated server there is no local user and nothing here runs.
 *
 * Select it in Project Settings > Game > Frontend Multiplayer Settings > Session Backend Class.
 */
UCLASS()
class FRONTENDMULTIPLAYEROSS_API UFrontendSessionBackend_GameLiftEOS : public UFrontendSessionBackend_GameLift
{
	GENERATED_BODY()

public:
	//~ Begin UFrontendSessionBackend Interface
	virtual void Shutdown() override;
	virtual void CreateSession(FName SessionName, int32 MaxPlayers) override;
	virtual void FindSessions() override;
	virtual void JoinSession(int32 SessionIndex) override;
	virtual bool CancelPendingOperation() override;
	virtual void FetchLeaderboard(ELeaderboardCategory Category) override;
	virtual FString GetBackendDisplayName() const override;
	virtual bool RequiresLogin() const override { return true; }
	virtual bool IsLoggedIn() const override;
	virtual void Login() override;
	virtual FString GetLocalPlayerDisplayName() const override;
	//~ End UFrontendSessionBackend Interface

protected:
	//~ Begin UFrontendSessionBackend_GameLift Interface
	virtual FString GetClientAuthToken() const override;
	//~ End UFrontendSessionBackend_GameLift Interface

private:
	/** A request parked until the sign-in finishes. Not reflected. */
	enum class EDeferred : uint8
	{
		None,
		Create,
		Find,
		Join
	};

	IOnlineIdentityPtr GetIdentityInterface() const;

	/** Run Request now if signed in, otherwise sign in first and run it from FinishLogin. */
	void EnsureLoggedInThen(EDeferred Request);
	void StartLogin(const FOnlineAccountCredentials& Credentials);
	void FinishLogin(bool bWasSuccessful, const FString& Error);
	void ContinueAfterLogin(EDeferred Request);
	void FailDeferredRequest(EDeferred Request, const FText& ErrorText);
	/** Answer a leaderboard fetch that waited for the sign-in: the real fetch, or an empty board when the sign-in failed. */
	void ResolveDeferredLeaderboardFetch(bool bSignedIn);

	void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void HandleLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId);

	FDelegateHandle LoginCompleteHandle;
	FDelegateHandle LoginStatusChangedHandle;
	FTSTicker::FDelegateHandle LoginTimeoutHandle;

	EDeferred DeferredRequest = EDeferred::None;
	FName DeferredSessionName;
	int32 DeferredMaxPlayers = 0;
	int32 DeferredJoinIndex = INDEX_NONE;

	/**
	 * A leaderboard fetch waiting for the sign-in. Not an EDeferred value on purpose: it is not a
	 * session operation and may coexist with a parked Host / Find / Join, whose slot it must not take.
	 */
	bool bLeaderboardFetchDeferred = false;
	ELeaderboardCategory DeferredLeaderboardCategory = ELeaderboardCategory::Main;

	bool bLoginInFlight = false;
	bool bHasFallbackCredentials = false;
	FOnlineAccountCredentials FallbackCredentials;
	FString CurrentCredentialType;
	int32 LoginAttemptSerial = 0;
};
