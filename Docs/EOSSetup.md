# Epic Online Services setup

How to run the Host / Server Browser / Join flow over **Epic Online Services (EOS)**
instead of the LAN-only Null subsystem. Nothing in the UI or in
`UFrontendSessionBackend_OSS` changes between the two; EOS is a per-launch switch.

| | LAN (default) | EOS |
|---|---|---|
| Subsystem | `Null` | `EOS` via `-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=EOS` |
| Identity | none | Epic Account Services + EOS Connect, signed in lazily by Host / Find / Join |
| Session | LAN beacon | EOS **lobby** (`bUseLobbiesIfAvailable`), found with `SEARCH_LOBBIES` |
| Transport | `IpNetDriver` (host travels with `?bIsLanMatch`) | `NetDriverEOSBase` P2P sockets, connect string `EOS:<puid>:GameNetDriver:7777` |
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
| 4 | **Epic Account Services** → Create Application | Permissions: **Basic Profile**, **Online Presence** and **Friends** - the engine requests all three scopes by default and the sign-in page rejects an application that lacks one. **Linked Clients** → the client from step 3. Brand review is not needed while only organisation members sign in. |
| 5 | **Dev Auth Tool** | Portal → *SDK & Release Notes* → download the C SDK zip → `SDK/Tools/EOS_DevAuthTool-win32-x64-*.zip` → run `EOS_DevAuthTool.exe`. Port **6547**, then **Login** twice: two different Epic accounts, saved as credentials named `User1` and `User2` (case-sensitive; the launcher sends them as `-AUTH_PASSWORD`). The second account must be a member of your organisation. Accounts created through Google: give them a password at epicgames.com first, the tool's embedded browser cannot complete a Google sign-in. |

Why two accounts: EOS rejects the same Epic account signing in twice, so two game
windows on one PC need two users. There is still only **one** client (step 3); the
Dev Auth Tool names are user labels, not client ids.

The first time each account signs in to this application, Epic opens a browser page to
**consent** to the permissions, and may add a one-off **corrective action** (accept
updated terms, verify e-mail, pick a display name). Complete the page as that account;
after that the sign-in is silent. If the page is ignored, the pending sign-in is
abandoned after `Login Timeout Seconds` (120 by default) and Host / Find are released.

## 2. Product keys (`Config/UserEngine.ini`)

Create **`Config/UserEngine.ini`** next to `DefaultEngine.ini` (it does not exist yet)
and put **one** line in it with the ids from section 1, under the same section the
project already configures:

```ini
[/Script/OnlineSubsystemEOS.EOSSettings]
+Artifacts=(ArtifactName="FrontendUI",ClientId="xyza78...",ClientSecret="...",ProductId="...",SandboxId="...",DeploymentId="...",EncryptionKey="0000000000000000000000000000000000000000000000000000000000000000")
```

| Field | Value |
|---|---|
| `ArtifactName` | must equal `DefaultArtifactName` in `DefaultEngine.ini` (`FrontendUI`) |
| `EncryptionKey` | any 64 hex characters; only Player Data Storage uses it, but the field is required |
| Everything else | from the portal, step 1-3; each id is 32 characters |

Why this file: `User<Type>.ini` in the project `Config/` folder is the last layer of the
engine's config hierarchy. It is read before any plugin starts, it overrides
`DefaultEngine.ini`, and the engine **never rewrites it** - unlike
`Saved/Config/<Platform>/Engine.ini`, which every editor and game instance regenerates
on exit and which is merged too late for the EOS plugin to see. `.gitignore` excludes
`Config/User*.ini`, so the keys stay out of the repository.

### Packaged builds

`Config/UserEngine.ini` is **not staged** by the packaging step, so a packaged game
reads only `DefaultEngine.ini`. That file ships with the same `+Artifacts=` line
**commented out**; for your own packaged builds uncomment it and paste your ids.

What is and is not sensitive:

| Value | Sensitive? | Why |
|---|---|---|
| `ProductId`, `SandboxId`, `DeploymentId`, `ClientId` | No | They identify the application. Every shipped EOS game carries them in its binary; the portal shows them in plain text. |
| `ClientSecret` of a **Peer2Peer** client | No, by design | Epic's model: a *client* policy is limited to what a player may do anyway, and its secret is expected to be embedded in the game. The plugin refuses to start with an empty secret, so it must be present. |
| `ClientSecret` of a **TrustedServer** client (Phase 8 dedicated servers) | **Yes** | Grants server-side rights; lives only on the server / in the backend, never in a client build. |
| `EncryptionKey` | No (for this project) | Only Player Data Storage encrypts with it; any 64 hex chars. |

So it is fine to commit a `DefaultEngine.ini` with your ids in **your** game's repo.
This template keeps the line commented so the repository never carries anyone's ids.
If your pipeline prefers injection, `-ini:Engine:[/Script/OnlineSubsystemEOS.EOSSettings]:+Artifacts=(...)`
on the command line works in Development builds (not Shipping).

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
| 1 | Command line `-AUTH_TYPE= -AUTH_LOGIN= -AUTH_PASSWORD=` | `LaunchInstances.bat ... EOS` passes `developer / localhost:6547 / UserN` |
| 2 | `persistentauth` | the refresh token EOS cached after an earlier interactive sign-in on this machine |
| 3 | **Project Settings → Game → Frontend Multiplayer OSS** | `Credential Type` (default `accountportal` = browser sign-in), `Login Id`, `Login Token` |

The same settings page can switch off step 1 (`Allow Command Line Credentials`) or
step 2 (`Try Persistent Auth First`), sets `Login Timeout Seconds`, and holds the
`Session Bucket` string that every online session advertises and every search filters
on (EOS refuses a lobby search with no attribute; different bucket values also keep
builds apart). `UFrontendMultiplayerSubsystem::Login()` is `BlueprintCallable` for a
"Sign in" button; `OnLoginStateChanged` and `GetLocalPlayerDisplayName()` drive a
"Signed in as ..." label.

Every step is in the API debug panel:

```
[oss] Login(developer) on EOS
[oss] OnLoginComplete (developer) -> OK
[oss] Signed in as 'DiegoG'
[oss] CreateSession 'Alpha' (4 players, LAN=false, map=L_MatchTest)
[oss] OnCreateSessionComplete GameSession -> OK
ServerTravel /Game/Maps/L_MatchTest?listen
Net driver: NetDriverEOSBase (ListenServer)
```

The last line is printed after every travel. On EOS it must say `NetDriverEOSBase`;
`IpNetDriver` there means the engine fell back to IP sockets and EOS clients will not
be able to connect.

Sign-in happens **once per process**. After `OnLoginComplete -> OK` the access token
lives for two hours and the Dev Auth Tool is not consulted again, so deleting the
credential or closing the tool does not affect a window that is already signed in; the
next *launch* is what fails. If the developer login cannot complete, EOS SDK 1.17 falls
back to a browser device-code page on its own (`falling back to browser based flow` in
the log); a browser that already has an Epic session finishes it in one click.

## 5. Test plan

Dev Auth Tool running on 6547 with `User1` / `User2`, then `Scripts\LaunchInstances.bat 2 EOS`
from a command prompt (double-clicking the file runs LAN mode).

| # | Step | Expect |
|---|---|---|
| 1 | A starts | debug panel `Session backend: OnlineSubsystem (EOS)` |
| 2 | A: Host "Alpha", 4 | `Login(developer)` → (first time only: browser consent page) → `OnLoginComplete -> OK` → `Signed in as '<EpicName>'` → `OnCreateSessionComplete -> OK` → `ServerTravel /Game/Maps/L_MatchTest?listen` (no `bIsLanMatch`) → `Net driver: NetDriverEOSBase (ListenServer)`; lobby title "Alpha" |
| 3 | B: Server Browser → Refresh | B signs in as `User2`, then `FindSessions (LAN=false, lobbies=true, bucket=FrontendUI, max=50)` → `OnFindSessionsComplete -> 1 session(s)`; may take a few seconds, press Refresh again if empty |
| 4 | B: Join | `ClientTravel EOS:<puid>:GameNetDriver:7777` → `Net driver: NetDriverEOSBase (Client)`; lobby shows 2 / 4 |
| 5 | Ready / Start / pause / Return to Main Menu | identical to v0.6 |
| 6 | Stop the Dev Auth Tool, A: Host | `Login(developer)` fails → Ok modal "Could Not Host: Could not sign in to EOS (developer): ...". Explicit command-line credentials never fall back, by design; launch without the `-AUTH_*` args to exercise `persistentauth` → `accountportal` (browser sign-in, then the host continues) |
| 7 | `Scripts\LaunchInstances.bat 2 LAN` | unchanged Null flow: `Session backend: OnlineSubsystem (NULL)`, no `Login` lines, `ServerTravel ...?listen?bIsLanMatch`, `Net driver: IpNetDriver (ListenServer)`, B: `FindSessions (LAN=true, lobbies=false, bucket=-, ...)` → `ClientTravel <LAN-IP>:7777` → `Net driver: IpNetDriver (Client)` |
| 8 | Two PCs on different networks | B finds and joins A without port forwarding (EOS P2P relays) |
| 9 | Package Development Win64, uncomment `+Artifacts` in `DefaultEngine.ini`, `Scripts\LaunchInstances.bat 2 EOS PACKAGED` | same lines as rows 1-4 from the staged exe; `EOSSDK Platform Properties [... ProductId=...]` proves the ids were read from `DefaultEngine.ini` |
| 10 | Packaged exe launched by hand with no `-AUTH_*` args, Multiplayer → **Sign in** | browser opens once → `Login(accountportal) -> OK` → panel reads "Signed in as <EpicName>"; relaunch → `Login(persistentauth) -> OK` with no browser |

## 6. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| Browser opens on Host and shows a permissions error naming `friends_list` | the EAS application lacks the **Friends** permission (step 4). Enable it, host again, accept the consent page |
| Browser opens and the log repeats `authorization_pending ... has yet to visit and enter the verification code` | Epic asked for a **corrective action** or consent for this account; finish the page signed in as that account, or clear it at epicgames.com/account. The game gives up after `Login Timeout Seconds` |
| `OnLoginComplete (developer) -> FAILED: ... connection refused` | Dev Auth Tool not running or on a different port; `DEVAUTH_HOST` in the launcher |
| `-> FAILED: ... EOS_InvalidCredentials` on `User2` | that credential name does not exist in the tool, or the second account is not in your organisation |
| `OnCreateSessionComplete -> FAILED` right after a successful login | EAS application missing the **Online Presence** permission, or the client policy is not Peer2Peer |
| `Session backend: OnlineSubsystem (NULL)` in EOS mode, log shows `UEOSSettings::GetSelectedArtifactSettings ArtifactName=[FrontendUI] ... no settings found` | the `+Artifacts=` line was not read: wrong file (it must be `Config/UserEngine.ini`), `ArtifactName` differs from `DefaultArtifactName`, or a typo in the line. The EOS subsystem refused to initialise and the engine fell back to Null |
| `OnFindSessionsComplete -> FAILED` with `LogEOSLobby: Search query is empty, search invalid` / `EOS_InvalidParameters` | the search carried no lobby attribute. `Session Bucket` must be non-empty and identical on host and client |
| B finds nothing | both windows must be in the **same sandbox + deployment** and use the same `Session Bucket`; also check that A's `CreateSession` log says `LAN=false` |
| `Net driver: IpNetDriver` on an EOS host, or Join travels to an `EOS:` address but never connects | the net driver definition was not applied: `DefaultEngine.ini` must contain `!NetDriverDefinitions=ClearArray` followed by the `/Script/SocketSubsystemEOS.NetDriverEOSBase` entry (section 3 of the committed ini) |
| LAN mode broke after this change | the host URL must end in `?listen?bIsLanMatch`; that option is what keeps `NetDriverEOSBase` on IP sockets |
| `LogSockets: Warning: Unable to load SocketSubsystem module EOS` in LAN mode | harmless. `NetDriverEOSBase` is first in `NetDriverDefinitions`; with the Null subsystem it declines and the engine falls through to `IpNetDriver`, which is the intended LAN path |
| Hosting still works after deleting the Dev Auth Tool credential | expected: the window was already signed in (see section 4). Relaunch to see the failure |

## 7. What is next

The seam is service-agnostic. A dedicated server (AWS GameLift, Docker / Kubernetes) is a
new `UFrontendSessionBackend` subclass that reports an `IP:port` connect string; the
same `NetDriverEOSBase` passes non-EOS addresses straight through to IP sockets, so
player-hosted P2P and dedicated servers coexist. See the roadmap in the
[README](../README.md).
