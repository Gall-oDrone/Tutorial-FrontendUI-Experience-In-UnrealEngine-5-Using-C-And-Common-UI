// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Backends/FrontendSessionBackend.h"
#include "Engine/DeveloperSettings.h"
#include "Containers/Ticker.h"
#include "FrontendSessionBackend_GameLift.generated.h"

class IWebSocket;
class IHttpRequest;
class FJsonObject;
class UWorld;

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
	 * Optional GameLift game session queue (name or ARN). When set, Host uses
	 * StartGameSessionPlacement on the queue instead of CreateGameSession on FleetId/AliasId:
	 * GameLift waits for capacity (auto-scaling, several fleets/regions) and reserves the host's
	 * player slot in the same request. Find / Join still use FleetId/AliasId.
	 * Overridable with -GameLiftQueue= or GAMELIFT_QUEUE_NAME.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Fleet")
	FString GameSessionQueueName;

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

	/**
	 * Seconds to poll DescribeGameSessionPlacement before giving up on a queued Host. Should be
	 * a little above the queue's own TimeoutInSeconds so GameLift's TIMED_OUT verdict is seen.
	 * Measured 2026-11-10: a cold scale-out on a container fleet (new instance -> container ->
	 * ActivateServerProcess) took ~7 min, so the dev queue uses 480 s and this 510 s.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Client", meta = (ClampMin = "5", UIMin = "5", UIMax = "900"))
	float PlacementTimeoutSeconds = 510.f;

	/**
	 * How long Host keeps retrying CreateGameSession (every 5 s) while GameLift answers
	 * FleetCapacityExceededException (no free server process). Covers the window where a
	 * recycled dev server is still booting, or a managed fleet is scaling up. 0 disables.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Client", meta = (ClampMin = "0", UIMin = "0", UIMax = "300"))
	float CreateSessionRetryTimeoutSeconds = 90.f;

	/**
	 * Rows the Leaderboard screen asks ApiEndpointUrl for (Leaderboard.GetTop; the proxy caps a
	 * request at 100). The player's own row is added to these when it ranks below them.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Client", meta = (ClampMin = "1", ClampMax = "100", UIMin = "1", UIMax = "100"))
	int32 LeaderboardMaxRows = 50;
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

	/**
	 * Short-lived compute auth token from GetComputeAuthToken (GAMELIFT_SDK_AUTH_TOKEN or -authtoken=).
	 * Managed Container Fleets provide none: the server then signs the WebSocket URL with SigV4 using
	 * the ECS task-role credentials and uses the ECS task id as its host id (Server SDK >= 5.2 behaviour).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server")
	FString ServerAuthToken;

	/**
	 * When true on a GameLift-connected server, reject PreLogin attempts that do not carry a ?PlayerSessionId= option,
	 * and remove a player whose PlayerSessionId GameLift then refuses (AcceptPlayerSession answered with an error:
	 * expired, already used, unknown). That answer arrives after PreLogin has let the player in, so it is a kick.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server")
	bool bRequirePlayerSessionIdOnServer = true;

	/**
	 * GameLift runs one game session per process: while a session is attached the process
	 * cannot be given another one, even with zero players. When true, the server ends the
	 * session (TerminateServerProcess) and exits once the last player leaves, so the fleet
	 * (or LaunchDedicatedServer.bat / docker restart) brings up a fresh process.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server")
	bool bExitProcessWhenSessionEmpty = true;

	/** Grace period before the exit above, so a player who reconnects immediately keeps the session alive. */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server", meta = (ClampMin = "0", UIMin = "0", UIMax = "120"))
	float SessionEmptyExitDelaySeconds = 5.f;

	/**
	 * After ActivateGameSession, seconds to wait for the first AcceptPlayerSession before ending
	 * the game session and exiting, exactly as when the last player leaves. Covers sessions
	 * nobody ever joins: a host whose client died between placement and travel, a placement the
	 * client abandoned, a CLI probe session, a RESERVED player slot GameLift expired after 60 s.
	 * GameLift only detaches a game session when the process exits, so without this such a
	 * session pins the process (and, at one session per instance, the instance) for good.
	 * Requires Exit Process When Session Empty; 0 disables.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server", meta = (ClampMin = "0", UIMin = "0", UIMax = "600"))
	float SessionNoPlayerExitDelaySeconds = 120.f;

	/**
	 * After a player with a PlayerSessionId logs in, ask GameLift (DescribePlayerSessions) which
	 * PlayerId the slot was reserved for and kick the player if it is not the identity that
	 * connected. Clients send their unique net id as PlayerId (the proxy in auth_mode=eos replaces
	 * it with the verified Epic account id, which is the first half of an EOS unique id), so a
	 * stolen or guessed ?PlayerSessionId= only works for the account it was created for. Clients
	 * older than v1.6 sent their display name instead and are kicked when this is on.
	 * A player who connects without any unique net id (not signed in) is kicked as well: the slot
	 * is reserved for an account and none was presented.
	 * Fails open (player stays) when GameLift does not answer within 10 s.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Dedicated Server")
	bool bVerifyPlayerSessionIdentity = true;
	//~ End Dedicated Server

	FString ResolveAwsRegion() const;
	FString ResolveFleetId() const;
	FString ResolveAliasId() const;
	FString ResolveLocation() const;
	FString ResolveGameSessionQueueName() const;
	FString ResolveApiEndpointUrl() const;
	bool ResolveAwsCredentials(FString& OutAccessKey, FString& OutSecretKey, FString& OutSessionToken) const;

	FString ResolveServerWebSocketUrl() const;
	FString ResolveServerHostId() const;
	FString ResolveServerProcessId() const;
	FString ResolveServerAuthToken() const;
	/**
	 * Key a dedicated server presents to the proxy to add leaderboard points (Leaderboard.AddScore).
	 * Environment only (GAMELIFT_SERVER_KEY): not a Config property, because an .ini is packaged
	 * into client builds, and not a command-line switch, because the command line is logged.
	 */
	FString ResolveLeaderboardServerKey() const;

	/** True on a Managed Container Fleet (GAMELIFT_COMPUTE_TYPE=CONTAINER with ECS task credentials available). */
	static bool IsContainerCompute();
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
 *            With GameSessionQueueName: GameLift.StartGameSessionPlacement -> polls
 *            DescribeGameSessionPlacement until FULFILLED (host slot included); a placement
 *            abandoned while PENDING is cancelled with GameLift.StopGameSessionPlacement
 *   - Find:  GameLift.DescribeGameSessions (StatusFilter=ACTIVE) -> ReportSessionsFound
 *   - Join:  GameLift.CreatePlayerSession -> ReportSessionJoined(true, "ip:port?PlayerSessionId=...")
 *   - Leave: clears local session state -> ReportSessionLeft
 *   - Leaderboard: Leaderboard.GetTop on ApiEndpointUrl (a table behind the proxy; GameLift has
 *            no leaderboard). Without a proxy, or with one that has no leaderboard, the policy
 *            is UFrontendMultiplayerSettings::bSimulateLeaderboardWhenUnsupported
 *
 * Server side (when running with -server and a GameLift WebSocket URL):
 *   - Connects to GameLift's Server SDK 5.x WebSocket endpoint on map load
 *   - Sends ActivateServerProcess (ProcessReady) and periodic HeartbeatServerProcess messages
 *   - Handles CreateGameSession -> updates lobby title/cap and replies ActivateGameSession
 *   - Validates joining clients with AcceptPlayerSession and frees slots on logout with RemovePlayerSession
 *   - Ends the session and exits (TerminateServerProcess) when the last player leaves, or when
 *     no player joined within SessionNoPlayerExitDelaySeconds of activation, so the fleet can
 *     hand the recycled process a new session
 *   - Handles RefreshConnection and TerminateProcess cleanly
 *   - AwardLeaderboardPoints: Leaderboard.AddScore on ApiEndpointUrl with the server key, for
 *     players whose identity it verified (the proxy credits the account GameLift holds for the slot)
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
	virtual bool CancelPendingOperation() override;
	virtual void FetchLeaderboard(ELeaderboardCategory Category) override;
	virtual FString GetBackendDisplayName() const override;
	virtual bool GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const override;
	virtual bool AcceptPlayerSession(const FString& PlayerSessionId, FString& OutErrorMessage) override;
	virtual void RemovePlayerSession(const FString& PlayerSessionId) override;
	virtual bool HandleServerSessionEmpty() override;
	virtual void VerifyPlayerSession(const FString& PlayerSessionId, const FString& UniqueNetIdString, FOnFrontendPlayerSessionVerified OnVerified) override;
	virtual void AwardLeaderboardPoints(const FString& PlayerSessionId, const FString& PlayerDisplayName, int32 Points, ELeaderboardCategory Category) override;
	//~ End UFrontendSessionBackend Interface

protected:
	/**
	 * Bearer token sent as `Authorization` with every request to ApiEndpointUrl, so the proxy
	 * can authenticate the player (the proxy ignores it when it runs without auth). Empty by
	 * default; UFrontendSessionBackend_GameLiftEOS returns the signed-in user's EOS token.
	 */
	virtual FString GetClientAuthToken() const { return FString(); }

private:
	//~ Begin Client HTTP helpers
	using FGameLiftJsonCallback = TFunction<void(bool bSuccess, TSharedPtr<FJsonObject> ResponseObject, const FText& ErrorText)>;

	void SendGameLiftRequest(const FString& TargetAction, const TSharedRef<FJsonObject>& PayloadObject, FGameLiftJsonCallback Callback);
	void SendCreateGameSessionRequest();
	bool HandleCreateRetryTick(float DeltaTime);
	bool HandleActivationPollTick(float DeltaTime);
	void PollActivatingGameSession();
	/** Queue path: StartGameSessionPlacement, then poll DescribeGameSessionPlacement until a terminal status. */
	void SendStartGameSessionPlacementRequest();
	bool HandlePlacementPollTick(float DeltaTime);
	void PollGameSessionPlacement();
	void HandlePlacementObject(const TSharedPtr<FJsonObject>& PlacementObject);
	/** Best-effort GameLift.StopGameSessionPlacement for a placement we stop waiting on while it may still be PENDING. */
	void StopAbandonedPlacement(const FString& PlacementId);
	/** bIsJoinRequest selects which Report* answers on failure: ReportSessionJoined (Join) or ReportSessionCreated (Host). */
	void RequestPlayerSessionForGameSession(const FString& GameSessionId, const FString& DisplayName, int32 MaxPlayers, bool bIsJoinRequest);
	void CancelPendingClientRequests();
	/** What GameLift knows this player as: the local unique net id (EOS "<account>|<puid>", Null machine id), or the display name without one. */
	FString BuildLocalPlayerId() const;
	/** What other players see in the browser's host column. */
	FString BuildLocalPlayerDisplayName() const;
	UWorld* GetGameWorld() const;
	//~ End Client HTTP helpers

	//~ Begin Leaderboard (Leaderboard.GetTop on the proxy)
	void HandleLeaderboardResponse(ELeaderboardCategory Category, int32 StatusCode, const FString& ResponseBody);
	/** No leaderboard service behind this configuration: simulated rows or an empty list, per bSimulateLeaderboardWhenUnsupported. */
	void ReportLeaderboardUnsupported(ELeaderboardCategory Category, const TCHAR* Reason);
	void CancelLeaderboardRequest();
	//~ End Leaderboard

	//~ Begin Server WebSocket helpers (GameLift Server SDK 5.x wire protocol)
	void StartServerWebSocketIfConfigured(UWorld* LoadedWorld);
	/** Container fleets: fetch ECS task credentials + task id, then connect with a SigV4-signed URL. */
	void BeginServerConnectWithContainerSigV4(const FString& WebSocketUrl);
	/** AuthToken wins; otherwise SigV4Query (already URL-encoded, without leading '&') is appended. */
	void ConnectServerWebSocket(const FString& WebSocketUrl, const FString& AuthToken, const FString& SigV4Query = FString());
	void CloseServerWebSocket();
	void ScheduleServerReconnect();
	bool HandleServerReconnectTick(float DeltaTime);
	void SendServerWebSocketMessage(const TSharedRef<FJsonObject>& MessageObject);
	bool HandleServerHeartbeatTick(float DeltaTime);
	bool HandleSessionEmptyExitTick(float DeltaTime);
	void CancelPendingServerExit();
	/** Armed when a game session is activated; disarmed by the first AcceptPlayerSession (returns true if it was armed). */
	void ArmSessionNoPlayerExit();
	bool CancelSessionNoPlayerExit();
	bool HandleSessionNoPlayerExitTick(float DeltaTime);
	void TerminateServerProcessAndExit(const TCHAR* Reason);
	/** DescribePlayerSessions reply (or its timeout) for a VerifyPlayerSession request. */
	void HandlePlayerSessionVerificationReply(const FString& RequestId, const TSharedPtr<FJsonObject>& ReplyObject);
	bool HandlePlayerSessionVerificationTimeout(float DeltaTime);
	static bool PlayerIdMatchesUniqueNetId(const FString& PlayerId, const FString& UniqueNetIdString);
	/** GameLift answered AcceptPlayerSession with an error: the player holding that id must go. */
	void HandlePlayerSessionAcceptRejected(const FString& PlayerSessionId);
	/** Answer OnVerified(false, Reason) on the next tick: a kick from inside PostLogin would destroy the controller the engine is still logging in. */
	void RejectPlayerSessionNextTick(const FOnFrontendPlayerSessionVerified& OnVerified, const FString& Reason);

	void HandleWebSocketConnected();
	void HandleWebSocketConnectionError(const FString& Error);
	void HandleWebSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void HandleWebSocketMessage(const FString& MessageString);
	//~ End Server WebSocket helpers

	/** Results of the last FindSessions call; JoinSession indexes into this. */
	TArray<FFrontendGameLiftSessionSummary> FoundSessions;

	/** Active HTTP request in flight on the client, if any. */
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveHttpRequest;

	/**
	 * Bumped by CancelPendingClientRequests. Each request remembers the value it was sent under;
	 * a completion that arrives under a newer one belongs to a cancelled or superseded request
	 * (new Host / Find / Join, Leave, the player's Cancel) and is dropped instead of reported.
	 */
	uint32 ClientRequestGeneration = 0;

	/**
	 * Leaderboard fetch in flight. Kept apart from ActiveHttpRequest and its generation: reading
	 * the board is not a session operation, so it must neither cancel a Host / Find / Join nor be
	 * cancelled by one. The serial is bumped per fetch (and on Shutdown); a completion that
	 * arrives under an older one was superseded and is dropped.
	 */
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> LeaderboardHttpRequest;
	uint32 LeaderboardRequestSerial = 0;

	/** Polls DescribeGameSessions while a newly created session transitions from ACTIVATING to ACTIVE. */
	FTSTicker::FDelegateHandle ActivationPollTickerHandle;
	FString PendingActivationGameSessionId;
	FString PendingActivationDisplayName;
	int32 PendingActivationMaxPlayers = 0;
	double ActivationPollDeadlineSeconds = 0.0;
	bool bActivationPollRequestInFlight = false;

	/** Host request being (re)sent while the fleet has no free process. */
	FTSTicker::FDelegateHandle CreateRetryTickerHandle;
	FString PendingCreateDisplayName;
	int32 PendingCreateMaxPlayers = 0;
	double CreateRetryDeadlineSeconds = 0.0;
	int32 CreateRetryAttempts = 0;

	/** Queued Host: placement being polled until FULFILLED / TIMED_OUT / CANCELLED / FAILED. */
	FTSTicker::FDelegateHandle PlacementPollTickerHandle;
	FString PendingPlacementId;
	double PlacementPollDeadlineSeconds = 0.0;
	bool bPlacementPollRequestInFlight = false;

	/** Session metadata for the match this client joined or this server is hosting. */
	FString ActiveSessionDisplayName;
	int32 ActiveSessionMaxPlayers = 0;
	FString ActiveGameSessionId;
	FString ActivePlayerSessionId;

	/** Server-side WebSocket state. */
	TSharedPtr<IWebSocket> ServerWebSocket;
	/** A failed/closed socket parked here until the next connect, so it is not destroyed inside its own delegate. */
	TSharedPtr<IWebSocket> RetiredServerWebSocket;
	FTSTicker::FDelegateHandle ServerHeartbeatTickerHandle;
	FTSTicker::FDelegateHandle ServerReconnectTickerHandle;
	FTSTicker::FDelegateHandle ServerSessionEmptyExitTickerHandle;
	FTSTicker::FDelegateHandle ServerSessionNoPlayerExitTickerHandle;

	/** VerifyPlayerSession requests waiting for GameLift's DescribePlayerSessions reply, by RequestId. */
	struct FPendingPlayerSessionVerification
	{
		FString PlayerSessionId;
		FString UniqueNetIdString;
		FOnFrontendPlayerSessionVerified OnVerified;
		double DeadlineSeconds = 0.0;
	};
	TMap<FString, FPendingPlayerSessionVerification> PendingPlayerSessionVerifications;
	FTSTicker::FDelegateHandle PlayerSessionVerificationTickerHandle;

	/** AcceptPlayerSession requests GameLift has not answered yet: RequestId -> PlayerSessionId. */
	TMap<FString, FString> PendingPlayerSessionAccepts;
	/** PlayerSessionIds GameLift refused, kept until that player is gone (RemovePlayerSession). */
	TSet<FString> RejectedPlayerSessionIds;
	/**
	 * How to remove the logged-in player holding a PlayerSessionId (the OnVerified handed to
	 * VerifyPlayerSession), for a refusal that arrives after the login completed. Cleared by RemovePlayerSession.
	 */
	TMap<FString, FOnFrontendPlayerSessionVerified> PlayerSessionKickers;
	/**
	 * PlayerSessionIds whose player passed the identity check (GameLift's PlayerId matched who
	 * connected), until RemovePlayerSession. Only these can earn leaderboard points.
	 */
	TSet<FString> VerifiedPlayerSessionIds;
	int32 ServerReconnectAttempts = 0;
	FString ServerProcessId;
	FString ServerHostId;
	FString ServerFleetId;
	FString ServerGameSessionId;
	int32 ServerListeningPort = 7777;
	bool bServerProcessReady = false;
	/** Container fleet mode: no auth token, (re)connects go through BeginServerConnectWithContainerSigV4. */
	bool bServerUsesSigV4 = false;
	/** In-flight ECS credentials / metadata request during the container bootstrap. */
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ServerBootstrapHttpRequest;
};
