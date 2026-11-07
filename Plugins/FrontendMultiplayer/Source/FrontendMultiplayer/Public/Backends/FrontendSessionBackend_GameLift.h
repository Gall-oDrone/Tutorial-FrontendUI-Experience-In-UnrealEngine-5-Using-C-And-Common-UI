// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Backends/FrontendSessionBackend.h"
#include "Engine/DeveloperSettings.h"
#include "Containers/Ticker.h"
#include "Interfaces/IHttpRequest.h"
#include "FrontendSessionBackend_GameLift.generated.h"

class IWebSocket;
class FJsonObject;

/**
 * Project settings for UFrontendSessionBackend_GameLift.
 * Project Settings > Game > Frontend Multiplayer GameLift.
 *
 * Supports both stages of the GameLift workflow without an external SDK:
 *   - Development / Anywhere: client signs requests directly to gamelift.<Region>.amazonaws.com
 *     (SigV4) using credentials from the command line, environment variables, or the
 *     git-ignored Config/UserGame.ini.
 *   - Production / Shipping: client sends the same JSON requests to ApiEndpointUrl (an
 *     API Gateway + Lambda proxy that holds the IAM role), and direct client credentials
 *     are compiled out.
 *
 * On a dedicated server (-server), the same class connects to the GameLift WebSocket
 * (GAMELIFT_SDK_WEBSOCKET_URL / -websocketurl=) to run the Server SDK 5.x lifecycle:
 * ActivateServerProcess, HeartbeatServerProcess, CreateGameSession -> ActivateGameSession,
 * AcceptPlayerSession, RemovePlayerSession, and TerminateServerProcess.
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Frontend Multiplayer GameLift"))
class FRONTENDMULTIPLAYER_API UFrontendGameLiftSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFrontendGameLiftSettings();

	static const UFrontendGameLiftSettings* Get() { return GetDefault<UFrontendGameLiftSettings>(); }

	//~ Begin Client & Fleet configuration
	/** AWS region of the GameLift fleet (e.g. "us-east-1"). Overridable with -GameLiftRegion= or AWS_REGION / GAMELIFT_REGION. */
	UPROPERTY(Config, EditAnywhere, Category = "Fleet")
	FString AwsRegion = TEXT("us-east-1");

	/** GameLift Fleet ID (e.g. "fleet-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"). Overridable with -GameLiftFleetId= or GAMELIFT_SDK_FLEET_ID. */
	UPROPERTY(Config, EditAnywhere, Category = "Fleet")
	FString FleetId;

	/** Optional GameLift Alias ID ("alias-..."). Used instead of FleetId when set. Overridable with -GameLiftAliasId=. */
	UPROPERTY(Config, EditAnywhere, Category = "Fleet")
	FString AliasId;

	/**
	 * Optional location filter (e.g. "custom-location-1" for an Anywhere fleet). Required
	 * by DescribeGameSessions on Anywhere fleets; leave empty for single-region managed fleets.
	 * Overridable with -GameLiftLocation=.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Fleet")
	FString Location;

	/**
	 * Optional HTTPS URL of a backend proxy (API Gateway / Lambda) that forwards GameLift
	 * JSON-RPC requests. Required in Shipping builds so AWS credentials never ship in the client.
	 * When empty in non-Shipping builds, the client talks directly to https://gamelift.<AwsRegion>.amazonaws.com/.
	 * Overridable with -GameLiftEndpoint=.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Endpoint")
	FString ApiEndpointUrl;

	/** Seconds to poll DescribeGameSessions while a newly created game session is in ACTIVATING state. */
	UPROPERTY(Config, EditAnywhere, Category = "Client", meta = (ClampMin = "1", UIMin = "1", UIMax = "60"))
	float SessionActivationTimeoutSeconds = 15.f;
	//~ End Client & Fleet configuration

	//~ Begin Development-only direct AWS credentials (ignored in Shipping builds; keep in Config/UserGame.ini or env vars)
	/** Development only: AWS Access Key ID when calling GameLift directly without ApiEndpointUrl. Prefer AWS_ACCESS_KEY_ID or Config/UserGame.ini. */
	UPROPERTY(Config, EditAnywhere, Category = "Development Credentials")
	FString AccessKeyId;

	/** Development only: AWS Secret Access Key when calling GameLift directly. Prefer AWS_SECRET_ACCESS_KEY or Config/UserGame.ini. */
	UPROPERTY(Config, EditAnywhere, Category = "Development Credentials")
	FString SecretAccessKey;

	/** Development only: optional AWS STS Session Token. Prefer AWS_SESSION_TOKEN or Config/UserGame.ini. */
	UPROPERTY(Config, EditAnywhere, Category = "Development Credentials")
	FString SessionToken;
	//~ End Development-only direct AWS credentials

	//~ Begin Dedicated Server (GameLift Anywhere / Container Fleet WebSocket)
	/**
	 * GameLift Server WebSocket URL (e.g. "wss://us-east-1.api.amazongamelift.com").
	 * On Managed Container Fleets and Anywhere scripts this comes from GAMELIFT_SDK_WEBSOCKET_URL
	 * or -websocketurl= on the command line.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server")
	FString ServerWebSocketUrl;

	/** Compute / Host ID registered with GameLift (GAMELIFT_SDK_HOST_ID or -hostid=). */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server")
	FString ServerHostId;

	/** Server process ID (GAMELIFT_SDK_PROCESS_ID or -processid=). Auto-generated if empty. */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server")
	FString ServerProcessId;

	/** Short-lived compute auth token from GetComputeAuthToken (GAMELIFT_SDK_AUTH_TOKEN or -authtoken=). */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server")
	FString ServerAuthToken;

	/** When true on a GameLift-connected server, reject PreLogin attempts that do not carry a ?PlayerSessionId= option. */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server")
	bool bRequirePlayerSessionIdOnServer = true;
	//~ End Dedicated Server

	FString ResolveAwsRegion() const;
	FString ResolveFleetId() const;
	FString ResolveAliasId() const;
	FString ResolveLocation() const;
	FString ResolveApiEndpointUrl() const;
	bool ResolveAwsCredentials(FString& OutAccessKey, FString& OutSecretKey, FString& OutSessionToken) const;

	FString ResolveServerWebSocketUrl() const;
	FString ResolveServerHostId() const;
	FString ResolveServerProcessId() const;
	FString ResolveServerAuthToken() const;
};

/**
 * Cached entry from the last DescribeGameSessions call so JoinSession(Index) can request
 * a player slot for the chosen GameSessionId.
 */
struct FFrontendGameLiftSessionSummary
{
	FString GameSessionId;
	FString DisplayName;
	FString HostName;
	FString MapName;
	FString IpAddress;
	int32 Port = 0;
	int32 CurrentPlayers = 0;
	int32 MaxPlayers = 0;
};

/**
 * Amazon GameLift session backend (Anywhere, Managed Container Fleets, Managed EC2).
 *
 * Client side:
 *   - Host:  GameLift.CreateGameSession -> (polls DescribeGameSessions if ACTIVATING) ->
 *            GameLift.CreatePlayerSession -> ReportSessionCreatedRemote("ip:port?PlayerSessionId=...")
 *   - Find:  GameLift.DescribeGameSessions (StatusFilter=ACTIVE) -> ReportSessionsFound
 *   - Join:  GameLift.CreatePlayerSession -> ReportSessionJoined(true, "ip:port?PlayerSessionId=...")
 *   - Leave: clears local session state -> ReportSessionLeft
 *
 * Server side (when running with -server and a GameLift WebSocket URL):
 *   - Connects to GameLift's Server SDK 5.x WebSocket endpoint on map load
 *   - Sends ActivateServerProcess (ProcessReady) and periodic HeartbeatServerProcess messages
 *   - Handles CreateGameSession -> updates lobby title/cap and replies ActivateGameSession
 *   - Validates joining clients with AcceptPlayerSession and frees slots on logout with RemovePlayerSession
 *   - Handles RefreshConnection and TerminateProcess cleanly
 */
UCLASS()
class FRONTENDMULTIPLAYER_API UFrontendSessionBackend_GameLift : public UFrontendSessionBackend
{
	GENERATED_BODY()

public:
	//~ Begin UFrontendSessionBackend Interface
	virtual void Initialize(UFrontendMultiplayerSubsystem* InOwningSubsystem) override;
	virtual void Shutdown() override;
	virtual void OnWorldLoaded(UWorld* LoadedWorld) override;
	virtual void CreateSession(FName SessionName, int32 MaxPlayers) override;
	virtual void FindSessions() override;
	virtual void JoinSession(int32 SessionIndex) override;
	virtual void LeaveSession() override;
	virtual void FetchLeaderboard(ELeaderboardCategory Category) override;
	virtual FString GetBackendDisplayName() const override;
	virtual bool GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const override;
	virtual bool AcceptPlayerSession(const FString& PlayerSessionId, FString& OutErrorMessage) override;
	virtual void RemovePlayerSession(const FString& PlayerSessionId) override;
	//~ End UFrontendSessionBackend Interface

private:
	//~ Begin Client HTTP helpers
	using FGameLiftJsonCallback = TFunction<void(bool bSuccess, TSharedPtr<FJsonObject> ResponseObject, const FText& ErrorText)>;

	void SendGameLiftRequest(const FString& TargetAction, const TSharedRef<FJsonObject>& PayloadObject, FGameLiftJsonCallback Callback);
	void PollActivatingGameSession();
	void RequestPlayerSessionForGameSession(const FString& GameSessionId, const FString& DisplayName, int32 MaxPlayers, EFrontendSessionOperation Operation);
	void CancelPendingClientRequests();
	FString BuildLocalPlayerId() const;
	//~ End Client HTTP helpers

	//~ Begin Server WebSocket helpers (GameLift Server SDK 5.x wire protocol)
	void StartServerWebSocketIfConfigured(UWorld* LoadedWorld);
	void ConnectServerWebSocket(const FString& WebSocketUrl, const FString& AuthToken);
	void CloseServerWebSocket();
	void SendServerWebSocketMessage(const TSharedRef<FJsonObject>& MessageObject);
	bool HandleServerHeartbeatTick(float DeltaTime);

	void HandleWebSocketConnected();
	void HandleWebSocketConnectionError(const FString& Error);
	void HandleWebSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void HandleWebSocketMessage(const FString& MessageString);
	//~ End Server WebSocket helpers

	/** Results of the last FindSessions call; JoinSession indexes into this. */
	TArray<FFrontendGameLiftSessionSummary> FoundSessions;

	/** Active HTTP request in flight on the client, if any. */
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveHttpRequest;

	/** Polls DescribeGameSessions while a newly created session transitions from ACTIVATING to ACTIVE. */
	FTSTicker::FDelegateHandle ActivationPollTickerHandle;
	FString PendingActivationGameSessionId;
	FString PendingActivationDisplayName;
	int32 PendingActivationMaxPlayers = 0;
	double ActivationPollDeadlineSeconds = 0.0;
	bool bActivationPollRequestInFlight = false;

	/** Session metadata for the match this client joined or this server is hosting. */
	FString ActiveSessionDisplayName;
	int32 ActiveSessionMaxPlayers = 0;
	FString ActiveGameSessionId;
	FString ActivePlayerSessionId;

	/** Server-side WebSocket state. */
	TSharedPtr<IWebSocket> ServerWebSocket;
	FTSTicker::FDelegateHandle ServerHeartbeatTickerHandle;
	FString ServerProcessId;
	FString ServerHostId;
	FString ServerFleetId;
	FString ServerGameSessionId;
	int32 ServerListeningPort = 7777;
	bool bServerProcessReady = false;
};
