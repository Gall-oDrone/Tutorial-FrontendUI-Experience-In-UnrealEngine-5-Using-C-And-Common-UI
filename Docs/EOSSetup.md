# Epic Online Services setup

How to run the Host / Server Browser / Join flow over **Epic Online Services (EOS)**
instead of the LAN-only Null subsystem. Nothing in the UI or in
`UFrontendSessionBackend_OSS` changes between the two; EOS is a per-launch switch.

| | LAN (default) | EOS |
|---|---|---|
| Subsystem | `Null` | `EOS` via `-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=EOS` |
| Identity | none | Epic Account Services + EOS Connect, signed in lazily by Host / Find / Join |
| Session | LAN beacon | EOS **lobby** (`bUseLobbiesIfAvailable`), found with `SEARCH_LOBBIES` |
| Transport | `IpNetDriver` (host travels with `?bIsLanMatch`) | `NetDriverEOS` P2P sockets, connect string `EOS:<puid>:GameNetDriver:7777` |
| Launch | `Scripts\LaunchInstances.bat 2` | `Scripts\LaunchInstances.bat 2 EOS` |

Everything that identifies *your* EOS product stays out of git. The committed
[DefaultEngine.ini](../Config/DefaultEngine.ini) only enables the plugin, names the
artifact and picks the net driver.

## 1. Epic Developer Portal (once, ~30 min)

| # | Where | What |
|---|---|---|
| 1 | [dev.epicgames.com](https://dev.epicgames.com/portal) → **Create Product** | e.g. `FrontendUI`. Note the **Product ID**. |
| 2 | Product Settings → **Sandboxes** | Use the default *Dev* sandbox and its deployment. Note **Sandbox ID** and **Deployment ID**. |
| 3 | Product Settings → **Clients** → Add | Name `FrontendUIClient`. **Client Policy**: create one of type **Peer2Peer** (grants Sessions, Lobbies, P2P, Presence). Note **Client ID** and **Client Secret**. |
| 4 | **Epic Account Services** → Create Application | Permissions: **Basic Profile** and **Online Presence** (presence sessions fail without it). **Linked Clients** → the client from step 3. Brand review is not needed while only organisation members sign in. |
| 5 | **Dev Auth Tool** | Portal → *SDK & Release Notes* → download the C SDK zip → `SDK/Tools/EOS_DevAuthTool-win32-x64-*.zip`. Run it, port **6547**, sign in **two** Epic accounts as credentials named `Client1` and `Client2`. The second account must be a member of your organisation to use the unreviewed application. |

Why two accounts: EOS rejects the same Epic account signing in twice, so two game
windows on one PC need two users.

## 2. Product keys (`Saved/Config/WindowsEditor/Engine.ini`)

`Saved/` is git-ignored. Create the file if it does not exist and add **one** line
with the ids from section 1, under the same section the project already configures:

```ini
[/Script/OnlineSubsystemEOS.EOSSettings]
+Artifacts=(ArtifactName="FrontendUI",ClientId="xyza78...",ClientSecret="...",ProductId="...",SandboxId="...",DeploymentId="...",EncryptionKey="0000000000000000000000000000000000000000000000000000000000000000")
```

| Field | Value |
|---|---|
| `ArtifactName` | must equal `DefaultArtifactName` in `DefaultEngine.ini` (`FrontendUI`) |
| `EncryptionKey` | any 64 hex characters; only Player Data Storage uses it, but the field is required |
| Everything else | from the portal, step 1–3 |

For a packaged game the same line goes in a non-git copy of the project ini or in
your build pipeline; never commit it.

## 3. Build

`OnlineSubsystemEOS` is now enabled in the `.uproject` and the OSS module gained a
settings class, so after pulling:

1. Close the editor.
2. Right-click `FrontendUI.uproject` → **Generate Visual Studio project files**.
3. Build **Development Editor** in Visual Studio (full build, not Live Coding).

## 4. Sign-in behaviour

`UFrontendSessionBackend_OSS` signs user 0 in on demand, the first time Host, Find or
Join runs on a subsystem that needs an account (anything but Null). The player sees
nothing unless a browser window is required. Credentials are tried in this order,
first success wins:

| # | Source | When |
|---|---|---|
| 1 | Command line `-AUTH_TYPE= -AUTH_LOGIN= -AUTH_PASSWORD=` | `LaunchInstances.bat ... EOS` passes `developer / localhost:6547 / ClientN` |
| 2 | `persistentauth` | the refresh token EOS cached after an earlier interactive sign-in on this machine |
| 3 | **Project Settings → Game → Frontend Multiplayer OSS** | `Credential Type` (default `accountportal` = browser sign-in), `Login Id`, `Login Token` |

The same settings page can switch off step 1 (`Allow Command Line Credentials`) or
step 2 (`Try Persistent Auth First`). `UFrontendMultiplayerSubsystem::Login()` is
`BlueprintCallable` for a "Sign in" button; `OnLoginStateChanged` and
`GetLocalPlayerDisplayName()` drive a "Signed in as ..." label.

Every step is in the API debug panel:

```
[oss] Login(developer) on EOS
[oss] OnLoginComplete (developer) -> OK
[oss] Signed in as 'DiegoG'
[oss] CreateSession 'Alpha' (4 players, LAN=false, map=L_MatchTest)
```

## 5. Test plan

Dev Auth Tool running on 6547 with `Client1` / `Client2`, then `Scripts\LaunchInstances.bat 2 EOS`.

| # | Step | Expect |
|---|---|---|
| 1 | A starts | debug panel `Session backend: OnlineSubsystem (EOS)` |
| 2 | A: Host "Alpha", 4 | `Login(developer)` → `OnLoginComplete -> OK` → `Signed in as '<EpicName>'` → `OnCreateSessionComplete -> OK` → `ServerTravel /Game/Maps/L_MatchTest?listen` (no `bIsLanMatch`); lobby title "Alpha" |
| 3 | B: Server Browser → Refresh | B signs in as `Client2`, then `OnFindSessionsComplete -> 1 session(s)`; EOS lobby search may take a few seconds |
| 4 | B: Join | `ClientTravel EOS:<puid>:GameNetDriver:7777`; lobby shows 2 / 4 |
| 5 | Ready / Start / pause / Return to Main Menu | identical to v0.6 |
| 6 | Stop the Dev Auth Tool, A: Host | `Login(developer)` fails → Ok modal "Could Not Host: Could not sign in to EOS (developer): ...". Explicit command-line credentials never fall back, by design; launch without the `-AUTH_*` args to exercise `persistentauth` → `accountportal` (browser sign-in, then the host continues) |
| 7 | `Scripts\LaunchInstances.bat 2 LAN` | unchanged Null flow: no `Login` lines, `ServerTravel ...?listen?bIsLanMatch`, `ClientTravel 127.0.0.1:7777` |
| 8 | Two PCs on different networks | B finds and joins A without port forwarding (EOS P2P relays) |

## 6. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `OnLoginComplete (developer) -> FAILED: ... connection refused` | Dev Auth Tool not running or on a different port; `DEVAUTH_HOST` in the launcher |
| `-> FAILED: ... EOS_InvalidCredentials` on `Client2` | that credential name does not exist in the tool, or the second account is not in your organisation |
| `OnCreateSessionComplete -> FAILED` right after a successful login | EAS application missing the **Online Presence** permission, or the client policy is not Peer2Peer |
| `Session backend: OnlineSubsystem (NULL)` in EOS mode | the `+Artifacts=` line is missing / `ArtifactName` differs from `DefaultArtifactName`; the EOS subsystem refused to initialise and the engine fell back. Look for `LogEOSSDK` errors at the top of `Saved/Logs/Instance1.log` |
| B finds nothing | both windows must be in the **same sandbox + deployment**; also check that A's `CreateSession` log says `LAN=false` |
| Join travels to an `EOS:` address but never connects | `[/Script/OnlineSubsystemEOS.NetDriverEOS] bIsUsingP2PSockets=true` missing, or the net driver definition was not applied (`!NetDriverDefinitions=ClearArray` must precede the `+` line) |
| LAN mode broke after this change | the host URL must end in `?listen?bIsLanMatch`; that option is what keeps `NetDriverEOS` on IP sockets |

## 7. What is next

The seam is service-agnostic. A dedicated server (AWS GameLift, Docker / Kubernetes) is a
new `UFrontendSessionBackend` subclass that reports an `IP:port` connect string; the
same `NetDriverEOS` passes non-EOS addresses straight through to IP sockets, so
player-hosted P2P and dedicated servers coexist. See the roadmap in the
[README](../README.md).
