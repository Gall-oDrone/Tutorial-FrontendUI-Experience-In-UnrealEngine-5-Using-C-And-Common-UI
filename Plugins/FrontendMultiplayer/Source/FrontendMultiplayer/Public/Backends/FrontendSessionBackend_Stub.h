// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Backends/FrontendSessionBackend.h"
#include "FrontendSessionBackend_Stub.generated.h"

/**
 * Default backend: no networking, simulated sessions and leaderboard rows, every call
 * logged to the API debug screen. Lets the multiplayer UI be built and exercised before
 * a real session layer exists, and remains the fallback if the configured backend
 * class fails to load.
 *
 * CreateSession reports success, so the Host flow still travels to HostMap as a
 * listen server (a convenient offline way to reach the match map). JoinSession
 * reports failure because there is nothing to connect to.
 */
UCLASS()
class FRONTENDMULTIPLAYER_API UFrontendSessionBackend_Stub : public UFrontendSessionBackend
{
	GENERATED_BODY()

public:
	//~ Begin UFrontendSessionBackend Interface
	virtual void CreateSession(FName SessionName, int32 MaxPlayers) override;
	virtual void FindSessions() override;
	virtual void JoinSession(int32 SessionIndex) override;
	virtual void LeaveSession() override;
	virtual void FetchLeaderboard(ELeaderboardCategory Category) override;
	virtual FString GetBackendDisplayName() const override { return TEXT("Stub (simulated, no networking)"); }
	//~ End UFrontendSessionBackend Interface
};
