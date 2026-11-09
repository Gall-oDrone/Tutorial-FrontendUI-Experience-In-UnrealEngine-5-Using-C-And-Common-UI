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

The replacement process takes a while to come back (30–60 s for the editor binary, a few seconds for a packaged server). Two things cover that window:

- **The client waits.** When `CreateGameSession` answers `FleetCapacityExceededException` (no free process), Host retries every 5 s for **Create Session Retry Timeout Seconds** (default 90 s) with the button in its busy state — `[gamelift] no free server process on the fleet yet; retrying CreateGameSession in 5s (attempt 1, 85s left)` — and only then fails with *No game server became available in time*.
- **Run a warm spare.** A real fleet keeps several processes per instance; Anywhere allows the same. Open a second server on another port and GameLift places the next match on whichever is free while the other restarts: `LaunchDedicatedServer.bat 7778 GAMELIFT` (the Windows firewall will prompt once for the new port).

---

## 4. Stage 2 — Building the Linux Dedicated Server & Docker Image (verified 2026-11-08)

Launcher engines cannot build Server targets, so the Linux server is built on a headless
EC2 box whose data volume is restored from the 500 GB snapshot of the **UE 5.6.1 source
build** (`snap-0777feef308067690`, `us-east-1`). Everything is driven from your machine over
SSM; no RDP, no DCV, no GPU. The companion repository
`UnrealEngine5DedicatedServersWithAWSAndGameLift` holds the Terraform and the scripts.

### One build session

```bash
# 0. The snapshot must be in the standard tier (restore takes 24-72 h from archive):
infrastructure/scripts/snapshot/manage_snapshot.sh status

# 1. Build box: c6i.4xlarge (16 vCPU / 32 GiB, ~$0.68/h) + 500 GB gp3 from the snapshot.
cd infrastructure/terraform/environments/dev && terraform apply        # ~1 min

# 2. Build + cook + stage, zip, upload. Everything runs on the box under SSM Run Command.
cd ../../../scripts/build
./run-build.sh --ref master --also-windows                             # see timings below

# 3. Stop paying. The snapshot keeps the engine; the box and volume are disposable.
cd ../../terraform/environments/dev && terraform destroy
```

`build-linux-server.ps1` (what runs on the box): brings the volume online, finds
`D:\UE_5_6` (checks `Engine\Build\Build.version` is 5.6), verifies the Linux cross toolchain
(`v25_clang-18.1.0-rockylinux8`, installs it from Epic's CDN if missing), clones/checks out the
ref into `D:\UE_5_6_Projects\FrontendUI`, then per platform:

```
RunUAT BuildCookRun -project=<uproject> -platform=Linux -server -noclient -serverconfig=Development
       -build -cook -stage -pak -nop4 -utf8output -unattended -nullrhi
```

and zips `Saved\StagedBuilds\LinuxServer` (and `WindowsServer`) to
`s3://ue5-artifacts-<account>-us-east-1/builds/FrontendUI-<Platform>Server-<sha>-<stamp>.zip`,
logs to `builds/logs/`.

| Phase (first run on the volume) | Time |
|---|---|
| Editor + engine modules for the project (912 actions, needed to cook) | ~30 min |
| Linux server target, cook (716 packages), stage, pak | ~5 min after the above is cached |
| Win64 server target (1000 actions, Server config not previously built) + cook/stage | ~26 min |
| Whole session incl. apply/destroy | 2 h 25 min, ≈ $1.90 |

Subsequent sessions reuse the compiled engine modules if the volume is kept; from a fresh
snapshot restore they recompile.

Three things the first run taught (all handled in the scripts now):

- **clang is stricter than MSVC.** `-Wrange-loop-construct` is an error in the Linux build; a
  range-for over `GetAllAvailableKeyProfiles()` with the wrong pair type compiled on Windows
  and failed here. Fixed in `OptionsDataRegistry.cpp` (`ad5a6a5`). Expect this class of error
  the first time any project is built for Linux.
- **Do not `-archive` into `Saved\StagedBuilds`.** The archive step cleans its target first;
  pointed at the stage directory it deletes the build it is about to copy. `-stage` alone
  leaves the result in `Saved\StagedBuilds\<Platform>Server`.
- **UAT leaves MSBuild node-reuse workers behind.** `Start-Process -Wait` waits for them too
  (15 min idle timeout). Wait on the UAT process object and set `MSBUILDDISABLENODEREUSE=1`.

### The Docker image

`Dockerfile` copies `Saved/StagedBuilds/LinuxServer` into `ubuntu:22.04` (+ `ca-certificates`,
`libatomic1`) as non-root user `ue`, exposes `7777/udp`, and runs `FrontendUIServer.sh` with
`-log -stdout -FullStdOutLogOutput -unattended -NullRHI -port=7777`. `.dockerignore` keeps
`*.debug`, `*.sym` and the Vulkan validation layers out of the image (≈2 GB of the 2.6 GB
stage; symbols stay in the S3 zip for crash symbolication).

```bash
aws s3 cp s3://ue5-artifacts-<account>-us-east-1/builds/FrontendUI-LinuxServer-<sha>-<stamp>.zip .
unzip -q FrontendUI-LinuxServer-*.zip -d Saved/StagedBuilds/        # creates LinuxServer/
docker build -t frontendui-server:<sha> .
```

### Running it on the Anywhere fleet (no client needed to verify)

Register the Docker host as a second compute so the PC's `DevPC` registration is untouched,
then run the container with the GameLift backend:

```bash
FLEET=fleet-a2a921a0-3c0f-4442-b469-5dc9c43d3792
aws gamelift register-compute --fleet-id $FLEET --compute-name LinuxDockerBox \
  --location custom-frontendui-dev --ip-address 127.0.0.1 --region us-east-1

docker run -d --name frontendui-server -p 7777:7777/udp \
  -e GAMELIFT_SDK_WEBSOCKET_URL=wss://us-east-1.api.amazongamelift.com \
  -e GAMELIFT_SDK_FLEET_ID=$FLEET -e GAMELIFT_SDK_HOST_ID=LinuxDockerBox \
  -e GAMELIFT_SDK_AUTH_TOKEN=$(aws gamelift get-compute-auth-token --fleet-id $FLEET \
       --compute-name LinuxDockerBox --region us-east-1 --query AuthToken --output text) \
  frontendui-server:<sha> /Game/Maps/L_MatchTest -log -stdout -FullStdOutLogOutput -unattended -NullRHI -port=7777 \
  "-ini:Game:[/Script/FrontendMultiplayer.FrontendMultiplayerSettings]:SessionBackendClass=/Script/FrontendMultiplayer.FrontendSessionBackend_GameLift"

docker logs frontendui-server | grep "\[gamelift\]"
#   connecting server WebSocket wss://us-east-1.api.amazongamelift.com/ (... compute=LinuxDockerBox ...)
#   server WebSocket connected -> ActivateServerProcess (port 7777)
#   ActivateServerProcess acknowledged; process is ACTIVE on the fleet      <- 1.2 s after launch

# Place a session on it straight from the CLI and watch GameLift flip it to ACTIVE
# (that only happens when the server answers ActivateGameSession):
aws gamelift create-game-session --fleet-id $FLEET --location custom-frontendui-dev \
  --maximum-player-session-count 4 --name Probe --region us-east-1 --query GameSession.GameSessionId --output text
aws gamelift describe-game-sessions --game-session-id <arn> --region us-east-1 --query 'GameSessions[0].Status'
#   "ACTIVE"   (within ~3 s; container log shows "CreateGameSession received ... -> ActivateGameSession")
```

`docker compose up` does the same with the variables from the environment
(`SERVER_SESSION_BACKEND=/Script/FrontendMultiplayer.FrontendSessionBackend_GameLift`); without
`GAMELIFT_SDK_*` it runs as a plain FixedAddress server for `LaunchInstances.bat N DEDICATED`.

The packaged Linux server initialises the engine in **0.7 s** (the editor binary used in
Stage 1 needs ~60 s), which is also why the Windows server zip is worth having on the PC:
`LaunchDedicatedServer.bat 7777 GAMELIFT PACKAGED` with `Saved\StagedBuilds\WindowsServer`
unzipped makes the recycle-and-rehost loop near instant.

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
| Client reaches the server, then `LogNet: Error: Server is incompatible with the local version of the game: RemoteNetworkVersion=… vs LocalNetworkVersion=…` / `OutdatedClient` | Client and server were built by different engine builds (Launcher editor vs. source-built 5.6.1 on the build box). The engine's default network version hashes the engine changelist. This project pins it to `ProjectVersion` instead (`FFrontendUIGameModule::GetProjectNetworkVersion`), so **both** sides must be built from a revision that has that override (≥ `v1.1.1`) and share the same `ProjectVersion` in `DefaultGame.ini`. If you bump `ProjectVersion`, rebuild the server artifacts. |
