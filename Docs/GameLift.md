# Amazon GameLift (Anywhere & Container Fleets)

How `UFrontendSessionBackend_GameLift` hosts, finds, and joins matches on Amazon GameLift, how the dedicated server speaks the GameLift Server SDK 5.x WebSocket protocol without external C++ SDK binaries, and how to progress from a single-PC Anywhere test to an AWS Linux container fleet.

---

## 1. Architecture Overview

```
Client (Host / Server Browser / Join)
   │
   ├── Development: HTTPS POST (AWS SigV4) ──► https://gamelift.<region>.amazonaws.com/
   └── Production:  HTTPS POST             ──► ApiEndpointUrl (API Gateway + Lambda proxy)
                                                      │
                                                      ▼
                                            Amazon GameLift Service
                                                      │
                                                      │  WSS (GameLift Server SDK 5.x protocol)
                                                      ▼
                                            Dedicated Server (-server)
                                            UFrontendSessionBackend_GameLift
                                                      │
                                                      ▼
                                            AGameUIMatchGameMode
                                            (PreLogin -> AcceptPlayerSession,
                                             Logout   -> RemovePlayerSession)
```

Both sides live in `UFrontendSessionBackend_GameLift` (`Plugins/FrontendMultiplayer/Source/FrontendMultiplayer/Public/Backends/FrontendSessionBackend_GameLift.h`) using Unreal's built-in `HTTP`, `Json`, and `WebSockets` modules:

- **Zero external ThirdParty SDK binaries**: the project compiles out of the box on a stock Epic Games Launcher UE 5.6 installation as well as on a source-built UE 5.6 engine.
- **Client side**:
  - **Host (`CreateSession`)**: sends `GameLift.CreateGameSession` with the session name, max players, and `GameProperties` (`HostName`, `MapName`). If the session starts in `ACTIVATING`, polls `GameLift.DescribeGameSessions` until the server activates it (`ACTIVE`), then calls `GameLift.CreatePlayerSession` and travels via `ReportSessionCreatedRemote("ip:port?PlayerSessionId=...")`.
  - **Find (`FindSessions`)**: sends `GameLift.DescribeGameSessions` (`StatusFilter = "ACTIVE"`) against the configured `FleetId` or `AliasId` (and optional `Location`), mapping each active session into the Server Browser.
  - **Join (`JoinSession`)**: sends `GameLift.CreatePlayerSession` for the chosen `GameSessionId` and travels via `ReportSessionJoined(true, "ip:port?PlayerSessionId=...")`.
- **Dedicated server side (`-server`)**:
  - When `GAMELIFT_SDK_WEBSOCKET_URL` (or `-websocketurl=`) is present, connects to GameLift's Server SDK 5.x WebSocket on map load.
  - Sends `ActivateServerProcess` (`ProcessReady`) with the server's listening port and log path, and heartbeats every 55 seconds (`HeartbeatServerProcess`).
  - When GameLift places a match (`CreateGameSession` WebSocket event), updates the lobby title and max players on `AGameUIMatchGameState` and replies `ActivateGameSession`.
  - Validates joining players in `AGameUIMatchGameMode::PreLogin` (`AcceptPlayerSession`) and frees slots in `AGameUIMatchGameMode::Logout` (`RemovePlayerSession`).
  - Handles `RefreshConnection` and `TerminateProcess` cleanly.

---

## 2. Configuration (`UFrontendGameLiftSettings`)

Open **Project Settings → Game → Frontend Multiplayer GameLift**, or configure via `Config/UserGame.ini` (git-ignored), environment variables, or command-line flags.

| Setting | INI Key | Command Line | Environment Variable | Purpose |
|---|---|---|---|---|
| **Aws Region** | `AwsRegion` | `-GameLiftRegion=` | `GAMELIFT_REGION` / `AWS_REGION` | AWS region (default `us-east-1`) |
| **Fleet Id** | `FleetId` | `-GameLiftFleetId=` | `GAMELIFT_SDK_FLEET_ID` / `GAMELIFT_FLEET_ID` | Target GameLift Fleet ID (`fleet-...`) |
| **Alias Id** | `AliasId` | `-GameLiftAliasId=` | `GAMELIFT_ALIAS_ID` | Optional GameLift Alias ID (`alias-...`, takes precedence over `FleetId` on client requests) |
| **Location** | `Location` | `-GameLiftLocation=` | `GAMELIFT_LOCATION` | Custom location name (e.g. `custom-location-1`) for Anywhere fleets |
| **Api Endpoint Url** | `ApiEndpointUrl` | `-GameLiftEndpoint=` | `GAMELIFT_ENDPOINT_URL` | Optional HTTPS proxy URL (required in Shipping client builds) |
| **Access Key Id** | `AccessKeyId` | `-GameLiftAccessKey=` | `GAMELIFT_ACCESS_KEY` / `AWS_ACCESS_KEY_ID` | Dev-only IAM Access Key (compiled out in `UE_BUILD_SHIPPING`) |
| **Secret Access Key** | `SecretAccessKey` | `-GameLiftSecretKey=` | `GAMELIFT_SECRET_KEY` / `AWS_SECRET_ACCESS_KEY` | Dev-only IAM Secret Key (compiled out in `UE_BUILD_SHIPPING`) |
| **Session Token** | `SessionToken` | `-GameLiftSessionToken=` | `GAMELIFT_SESSION_TOKEN` / `AWS_SESSION_TOKEN` | Optional STS session token |
| **Server WebSocket Url** | `ServerWebSocketUrl` | `-websocketurl=` | `GAMELIFT_SDK_WEBSOCKET_URL` | GameLift Anywhere / Container Fleet WebSocket endpoint |
| **Server Host Id** | `ServerHostId` | `-hostid=` | `GAMELIFT_SDK_HOST_ID` | Compute name registered with the fleet |
| **Server Process Id** | `ServerProcessId` | `-processid=` | `GAMELIFT_SDK_PROCESS_ID` | Unique process ID (auto-generated if empty) |
| **Server Auth Token** | `ServerAuthToken` | `-authtoken=` | `GAMELIFT_SDK_AUTH_TOKEN` | Short-lived token from `aws gamelift get-compute-auth-token` |

### Example `Config/UserGame.ini` (git-ignored)

```ini
[/Script/FrontendMultiplayer.FrontendGameLiftSettings]
AwsRegion=us-east-1
FleetId=fleet-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
Location=custom-location-1
AccessKeyId=AKIA...
SecretAccessKey=...
```

---

## 3. Stage 1 — GameLift Anywhere on One PC (Launcher Engine, Zero Docker)

GameLift Anywhere lets the cloud GameLift control plane manage a dedicated server process running on your own workstation, even with the stock Epic Games Launcher engine (`UnrealEditor-Cmd.exe -server`).

### One-time AWS CLI setup

```bat
REM 1. Create a custom location
aws gamelift create-location --location-name custom-location-1 --region us-east-1

REM 2. Create an Anywhere fleet in that location
aws gamelift create-fleet ^
  --name FrontendUI-Anywhere ^
  --compute-type ANYWHERE ^
  --locations "Location=custom-location-1" ^
  --region us-east-1
REM Note the FleetId (fleet-...) and WebSocketUrl from describe-fleet-attributes / register-compute

REM 3. Register your development machine as a compute host (replace 127.0.0.1 with your LAN IP if testing across PCs)
aws gamelift register-compute ^
  --fleet-id fleet-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx ^
  --compute-name DevPC ^
  --location custom-location-1 ^
  --ip-address 127.0.0.1 ^
  --region us-east-1
```

`register-compute` returns `GameLiftServiceSdkEndpoint` (for example `wss://us-east-1.api.amazongamelift.com`).

### Launch the dedicated server and two clients

Put the fleet values in `Scripts\GameLiftEnv.bat` once (git-ignored; both launch scripts `call` it, so every new console window picks them up):

```bat
cd /d D:\EpicGames\Projects\UE_5.6\FrontendUI\Scripts
copy GameLiftEnv.example.bat GameLiftEnv.bat
notepad GameLiftEnv.bat        REM fleet id, compute name, websocket URL, location
```

Then:

```bat
LaunchDedicatedServer.bat 7777 GAMELIFT
LaunchInstances.bat 2 GAMELIFT
```

(`set GAMELIFT_SDK_FLEET_ID=…` etc. typed in the console still work and take precedence; they just do not survive a new window, which is the usual cause of `GameLift Fleet ID or Alias ID is not configured` the next day.)

Both scripts lean on the AWS CLI (`aws configure` once, with a user that may call `gamelift:*` on the fleet):

- `LaunchDedicatedServer.bat 7777 GAMELIFT` calls `aws gamelift get-compute-auth-token` when `GAMELIFT_SDK_FLEET_ID` and `GAMELIFT_SDK_HOST_ID` are set and `GAMELIFT_SDK_AUTH_TOKEN` is not, and passes the token to the server through the environment.
- `LaunchInstances.bat 2 GAMELIFT` fills `AWS_ACCESS_KEY_ID` / `AWS_SECRET_ACCESS_KEY` (and `AWS_REGION`) from `aws configure get …` when they are not already in the environment. The game does **not** read `~/.aws/credentials` itself; it reads those variables or the git-ignored `Config/UserGame.ini`. Keys travel to the game windows through the inherited environment, never on the command line, so they do not appear in `Saved/Logs/Instance*.log`.

### Expected Walkthrough (verified 2026-11-08 on a Launcher UE 5.6 editor binary)

Log lines are prefixed `[gamelift]` in `Saved\Logs\Server.log` / `Instance*.log` and in the in-game API debug panel.
`findstr /c:"[gamelift]" ..\Saved\Logs\Server.log` shows just those.

| # | Step | Expect |
|---|---|---|
| 1 | Server boots `L_MatchTest` | `Server.log`: `connecting server WebSocket wss://us-east-1.api.amazongamelift.com/ (fleet=… compute=DevPC process=ue-server-… port=7777)` → `server WebSocket connected -> ActivateServerProcess (port 7777)` → `ActivateServerProcess acknowledged; process is ACTIVE on the fleet` |
| 2 | Client A: Multiplayer → Host → "Alpha", 4 → Create | `Instance1.log`: `CreateGameSession 'Alpha' (4 players)` → `CreateGameSession -> arn:aws:gamelift:…:gamesession/… (ACTIVATING)` → `CreatePlayerSession -> psess-…; travelling to 127.0.0.1:7777?PlayerSessionId=psess-…`. `Server.log`: `CreateGameSession received (… 'Alpha' 4 players) -> ActivateGameSession` → `AcceptPlayerSession psess-…`. Lobby title **Alpha**, 1 / 4. |
| 3 | AWS: `aws gamelift describe-game-sessions --fleet-id <fleet> --location <location>` | `Alpha` / `ACTIVE` / `CurrentPlayerSessionCount: 1`; `describe-player-sessions --game-session-id <arn>` shows A as `ACTIVE` (GameLift only marks it `ACTIVE` once the server accepted it; a missing `AcceptPlayerSession` leaves it `RESERVED` → `TIMEDOUT` after 60 s) |
| 4 | Client B: Server Browser → Refresh → Join | `DescribeGameSessions -> 1 active session(s)`; row `Alpha` 1/4, map `L_MatchTest`; Join reserves a second player session and lands in the lobby (2 / 4); server logs a second `AcceptPlayerSession` |
| 5 | Both Ready | Countdown (3.0 s) → `InProgress`, both players spawn |
| 6 | B: Return to Main Menu | `Server.log`: `RemovePlayerSession psess-…`; AWS: B's player session flips to `COMPLETED` |
| 7 | A: Return to Main Menu (last player) | `Server.log`: `last player left game session …; ending it and exiting in 5s so the fleet can recycle this process` → `TerminateServerProcess (game session empty) -> exiting`. The server **process exits**; the `LaunchDedicatedServer.bat` window restarts it with a fresh auth token (`Run 2: starting dedicated server…`). AWS: game session `TERMINATED` |
| 8 | A: Host again | Works as in step 2 against the fresh process |

### Why the process exits when the session empties

GameLift attaches **one game session per server process**. While that session exists the process cannot receive another one, even with zero players, so a new Host would fail with `GameLift error (400): Unable to reserve a process (No active and available server processes found on the fleet)`. The standard pattern is to end the session and exit, and let the fleet start a new process:

| Where | Who restarts the process |
|---|---|
| Anywhere on your PC | the restart loop in `LaunchDedicatedServer.bat … GAMELIFT` (set `NO_RESTART=1` for a single run) |
| Docker / Compose | `restart: unless-stopped` in `docker-compose.yml` |
| Managed Container / EC2 fleets | GameLift itself, per the runtime configuration |

`UFrontendSessionBackend_GameLift::HandleServerSessionEmpty` implements it: after **Session Empty Exit Delay Seconds** (default 5 s; a player arriving in that window cancels the exit) it sends `TerminateServerProcess` and requests engine exit. Turn **Exit Process When Session Empty** off in *Project Settings → Game → Frontend Multiplayer GameLift* to keep the old behaviour (the game mode resets the match to the lobby, and the game session stays attached until the process ends). The FixedAddress backend is unaffected: it has no session service, so the server simply returns to the lobby.

---

## 4. Stage 2 — Building the Linux Dedicated Server & Docker Image

Building `FrontendUIServer` for Linux requires the Unreal Engine 5.6 source build stored on the 500 GB AWS EBS snapshot (`snap-0777feef308067690` in `us-east-1`, managed by the companion repository `UnrealEngine5DedicatedServersWithAWSAndGameLift`).

### Checking the EBS Snapshot Restore Status

```bash
cd ../UnrealEngine5DedicatedServersWithAWSAndGameLift/infrastructure/scripts/snapshot
./manage_snapshot.sh status
```

Ensure `Storage tier` is `standard` or `Last tier op` shows `temporary-restore-completed 100%` before running `terraform apply` on the build workstation.

### Cooking and Staging `LinuxServer` on the EC2 Build Box

```bat
cd /d D:\UE_5.6
GenerateProjectFiles.bat -project="D:\Projects\FrontendUI\FrontendUI.uproject" -game -engine
Engine\Build\BatchFiles\RunUAT.bat BuildCookRun ^
  -project="D:\Projects\FrontendUI\FrontendUI.uproject" ^
  -platform=Linux -server -serverconfig=Development -noclient ^
  -build -cook -allmaps -stage -pak -archive ^
  -archivedirectory="D:\Projects\FrontendUI\Saved\StagedBuilds" ^
  -nop4 -utf8output
```

This produces `Saved/StagedBuilds/LinuxServer/FrontendUIServer.sh` and `FrontendUI/Binaries/Linux/FrontendUIServer`.

### Building and Running the Docker Container

The root `Dockerfile`, `docker-compose.yml`, and `.dockerignore` package `Saved/StagedBuilds/LinuxServer` into an `ubuntu:22.04` image running as non-root user `ue`:

```bash
# Run locally as a FixedAddress dedicated server on UDP 7777:
docker compose up --build

# Or run the container connected to a GameLift Anywhere fleet:
SERVER_SESSION_BACKEND=/Script/FrontendMultiplayer.FrontendSessionBackend_GameLift \
GAMELIFT_SDK_WEBSOCKET_URL=wss://us-east-1.api.amazongamelift.com \
GAMELIFT_SDK_FLEET_ID=fleet-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx \
GAMELIFT_SDK_HOST_ID=DevPC \
GAMELIFT_SDK_AUTH_TOKEN=$(aws gamelift get-compute-auth-token \
  --fleet-id fleet-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx \
  --compute-name DevPC --query AuthToken --output text) \
docker compose up
```

---

## 5. Stage 3 — Amazon GameLift Managed Container Fleets

Once the image runs cleanly in Docker, push it to Amazon ECR and deploy a Managed Container Fleet:

```bash
# 1. Authenticate and push to ECR
aws ecr create-repository --repository-name frontendui-server --region us-east-1
aws ecr get-login-password --region us-east-1 | \
  docker login --username AWS --password-stdin <account-id>.dkr.ecr.us-east-1.amazonaws.com

docker tag frontendui-server:latest <account-id>.dkr.ecr.us-east-1.amazonaws.com/frontendui-server:latest
docker push <account-id>.dkr.ecr.us-east-1.amazonaws.com/frontendui-server:latest
```

When creating the GameLift Container Group Definition:
- **Image URI**: `<account-id>.dkr.ecr.us-east-1.amazonaws.com/frontendui-server:latest`
- **Container port**: `7777` (`UDP`)
- **Command override**:
  ```json
  ["/Game/Maps/L_MatchTest", "-log", "-unattended", "-NullRHI", "-port=7777", "-ini:Game:[/Script/FrontendMultiplayer.FrontendMultiplayerSettings]:SessionBackendClass=/Script/FrontendMultiplayer.FrontendSessionBackend_GameLift"]
  ```
- GameLift Managed Container Fleets automatically inject `GAMELIFT_SDK_WEBSOCKET_URL`, `GAMELIFT_SDK_FLEET_ID`, `GAMELIFT_SDK_HOST_ID`, `GAMELIFT_SDK_PROCESS_ID`, and `GAMELIFT_SDK_AUTH_TOKEN` into the container environment at launch.
- On the client, set `FleetId` (or `AliasId`) to the container fleet and clear `Location` (managed fleets in a single region do not need a custom location filter).

---

## 6. Shipping Builds — Backend Proxy (`ApiEndpointUrl`)

In `UE_BUILD_SHIPPING` builds, `UFrontendGameLiftSettings::ResolveAwsCredentials` intentionally refuses raw IAM credentials so AWS keys never ship in player builds. Point `ApiEndpointUrl` (`-GameLiftEndpoint=`) at an Amazon API Gateway + AWS Lambda endpoint whose execution role grants `gamelift:CreateGameSession`, `gamelift:DescribeGameSessions`, and `gamelift:CreatePlayerSession` on your fleet/alias.

Because the client sends the standard `X-Amz-Target` header (`GameLift.CreateGameSession`, `GameLift.DescribeGameSessions`, `GameLift.CreatePlayerSession`) and JSON payload to `ApiEndpointUrl`, a minimal Python 3.12 Lambda proxy is only ~25 lines:

```python
import json, os, boto3

gamelift = boto3.client("gamelift", region_name=os.environ.get("AWS_REGION", "us-east-1"))
ALLOWED = {
    "GameLift.CreateGameSession": gamelift.create_game_session,
    "GameLift.DescribeGameSessions": gamelift.describe_game_sessions,
    "GameLift.CreatePlayerSession": gamelift.create_player_session,
}

def handler(event, context):
    headers = {k.lower(): v for k, v in (event.get("headers") or {}).items()}
    target = headers.get("x-amz-target", "")
    fn = ALLOWED.get(target)
    if not fn:
        return {"statusCode": 400, "body": json.dumps({"message": f"Unsupported target: {target}"})}
    body = json.loads(event.get("body") or "{}")
    try:
        resp = fn(**body)
        resp.pop("ResponseMetadata", None)
        return {"statusCode": 200, "body": json.dumps(resp, default=str)}
    except Exception as exc:
        return {"statusCode": 400, "body": json.dumps({"message": str(exc)})}
```

---

## 7. Troubleshooting

| Symptom | Cause / Fix |
|---|---|
| `GameLift Fleet ID or Alias ID is not configured` | Set `FleetId` in `Config/UserGame.ini`, pass `-GameLiftFleetId=fleet-...`, or export `GAMELIFT_SDK_FLEET_ID`. |
| `No GameLift API Endpoint URL or AWS credentials found` | Export `AWS_ACCESS_KEY_ID` and `AWS_SECRET_ACCESS_KEY` (or set them in `Config/UserGame.ini`) for non-Shipping builds, or configure `ApiEndpointUrl`. |
| `GameLift error (400): Unable to reserve a process (No active and available server processes found on the fleet)` | Either no server process has completed `ActivateServerProcess` (check `Server.log` for `ActivateServerProcess acknowledged`), or the only process is still attached to an earlier game session. With **Exit Process When Session Empty** on, the server exits a few seconds after its last player leaves and the launch script / fleet restarts it; if you turned that off, restart the server by hand. |
| `Server WebSocket connection error` / `403 Forbidden` | The Anywhere compute auth token expired (tokens from `GetComputeAuthToken` last about 3 hours). Re-run `LaunchDedicatedServer.bat 7777 GAMELIFT` with `GAMELIFT_SDK_AUTH_TOKEN` unset so it fetches a fresh token via `aws gamelift get-compute-auth-token`. |
| `GameLift server requires a valid ?PlayerSessionId= option to join` | Client connected directly by IP without calling `CreatePlayerSession`. Use the Host or Server Browser Join flow in `GAMELIFT` mode, or disable `bRequirePlayerSessionIdOnServer` when testing raw `open 127.0.0.1:7777`. |
