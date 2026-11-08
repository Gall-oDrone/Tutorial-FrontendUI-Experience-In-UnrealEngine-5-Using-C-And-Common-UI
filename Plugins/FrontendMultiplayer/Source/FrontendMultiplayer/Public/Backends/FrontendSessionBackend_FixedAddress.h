// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Backends/FrontendSessionBackend.h"
#include "Engine/DeveloperSettings.h"
#include "FrontendSessionBackend_FixedAddress.generated.h"

/**
 * Project settings for UFrontendSessionBackend_FixedAddress: the one dedicated server
 * every client talks to. Project Settings > Game > Frontend Multiplayer Fixed Address.
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Frontend Multiplayer Fixed Address"))
class FRONTENDMULTIPLAYER_API UFrontendFixedAddressSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFrontendFixedAddressSettings();

	static const UFrontendFixedAddressSettings* Get() { return GetDefault<UFrontendFixedAddressSettings>(); }

	/** "host:port" of the dedicated server. Overridable per launch with -ServerAddress=host:port. */
	UPROPERTY(Config, EditAnywhere, Category = "Server")
	FString ServerAddress = TEXT("127.0.0.1:7777");

	/** Title shown for the server in the browser row and the lobby. */
	UPROPERTY(Config, EditAnywhere, Category = "Server")
	FString SessionDisplayName = TEXT("Dedicated Server");

	/** Player cap advertised in the browser row and used by the lobby's "n / max" label. The server enforces its own. */
	UPROPERTY(Config, EditAnywhere, Category = "Server", meta = (ClampMin = "1", UIMin = "1", UIMax = "64"))
	int32 MaxPlayers = 4;

	/** The address actually used: the command line override if present, else ServerAddress. */
	FString ResolveServerAddress() const;
};

/**
 * Backend for a single, known dedicated server (a local docker container, an EC2 box,
 * a GameLift Anywhere compute): no matchmaking, no service, just an address.
 *
 *  - Host:   there is nothing to host locally; the request answers with
 *            ReportSessionCreatedRemote and the player becomes the server's first client.
 *  - Find:   returns exactly one row describing that server.
 *  - Join:   client-travels to the address.
 *
 * Identity is not required, so the Sign-in panel stays hidden. On the server process
 * the same class answers GetActiveSessionInfo so the lobby shows the configured title
 * and player cap. Useful on its own for LAN parties and CI, and as the first rung
 * before a service-backed dedicated-server backend (GameLift).
 */
UCLASS()
class FRONTENDMULTIPLAYER_API UFrontendSessionBackend_FixedAddress : public UFrontendSessionBackend
{
	GENERATED_BODY()

public:
	//~ Begin UFrontendSessionBackend Interface
	virtual void CreateSession(FName SessionName, int32 MaxPlayers) override;
	virtual void FindSessions() override;
	virtual void JoinSession(int32 SessionIndex) override;
	virtual void LeaveSession() override;
	virtual void FetchLeaderboard(ELeaderboardCategory Category) override;
	virtual FString GetBackendDisplayName() const override;
	virtual bool GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const override;
	//~ End UFrontendSessionBackend Interface

private:
	/** True while this process is connected to, or is, the dedicated server. */
	bool IsInNetworkedWorld() const;
};
