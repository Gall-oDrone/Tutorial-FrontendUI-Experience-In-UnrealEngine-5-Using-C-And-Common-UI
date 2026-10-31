# Session Backend

How the multiplayer screens host, find and join real sessions, and how to point them
at a different online service.

```
Host / Server Browser / Leaderboard screens
        │  CreateSession / FindSessions / JoinSession / FetchLeaderboard
        ▼
UFrontendMultiplayerSubsystem          (FrontendMultiplayer module)
        │  forwards                     ▲ Report* results
        ▼                               │
UFrontendSessionBackend (abstract) ─────┘
        ├── UFrontendSessionBackend_Stub   simulated, no networking      (FrontendMultiplayer)
        └── UFrontendSessionBackend_OSS    IOnlineSession: Null/Steam/EOS (FrontendMultiplayerOSS)
```

Screens never see the backend. The subsystem owns everything that happens *after* a
request succeeds:

| Result | Subsystem action |
|---|---|
| `ReportSessionCreated(true)` | `World->ServerTravel(HostMap + "?listen")` — host becomes a listen server |
| `ReportSessionJoined(true, ConnectString)` | `FirstLocalPC->ClientTravel(ConnectString, TRAVEL_Absolute)` |
| any `Report*(false, ErrorText)` | Ok confirm screen on the Modal stack + `OnSessionOperationFailed` |
| `ReportOperationStarted(Op)` / completion | `OnSessionOperationChanged(Op, bInProgress)` — Host screen disables **Create**, Server Browser gets `BP_OnSearchStateChanged` |

Both travels go through the existing `UFrontendLoadingScreenSubsystem` map hooks.

## Project setup (Null subsystem, LAN)

1. **Plugins** (`.uproject`): `OnlineSubsystem`, `OnlineSubsystemUtils`, `OnlineSubsystemNull`,
   and `OnlineSubsystemEOS` (enabled but idle until EOS is selected; see
   [Docs/EOSSetup.md](../../../Docs/EOSSetup.md)).
2. **`Config/DefaultEngine.ini`**
   ```ini
   [OnlineSubsystem]
   DefaultPlatformService=Null

   [/Script/Engine.GameEngine]
   !NetDriverDefinitions=ClearArray
   +NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="OnlineSubsystemEOS.NetDriverEOS",DriverClassNameFallback="OnlineSubsystemUtils.IpNetDriver")
   ```
   `NetDriverEOS` passes through to IP sockets whenever the address is not an EOS one,
   and a LAN host travels with `?bIsLanMatch`, so this driver is safe for the Null path.
3. **Project Settings → Game → Frontend Multiplayer Settings**

   | Setting | Value |
   |---|---|
   | Enable Multiplayer | on |
   | Session Backend Class | `FrontendSessionBackend_OSS` |
   | Host Map | the match map (`L_MatchTest` here). Its game mode must handle networked players — `BP_MatchGameMode`. |
   | Allow Join In Progress | on (default) |
   | Max Search Results | 50 (default) |
   | Simulate Leaderboard When Unsupported | on — Null has no leaderboard interface |
   | Show API Debug Panel | on while testing |

4. Regenerate project files (new module), close the editor, full build.

## Behaviour of `UFrontendSessionBackend_OSS`

| Request | What happens |
|---|---|
| **Sign-in** | Only on subsystems that need an account (`RequiresLogin() = !IsLAN()`, i.e. anything but Null). Host / Find / Join call `EnsureLoggedInThen(Request)`: if `IOnlineIdentity::GetLoginStatus(0)` is not `LoggedIn`, the credential chain from **Project Settings → Game → Frontend Multiplayer OSS** runs (command line `-AUTH_TYPE/-AUTH_LOGIN/-AUTH_PASSWORD` → `persistentauth` → configured type, default `accountportal`) and the parked request continues from `OnLoginComplete`. A failure answers the parked request with the login error. `Login()` does the same explicitly under the `LoggingIn` operation; a later sign-out reaches the UI through `OnLoginStateChanged(false)`. |
| **Create** | If a `GameSession` already exists it is destroyed first (the service allows one). Session: `NumPublicConnections = MaxPlayers`, `bShouldAdvertise`, `bAllowJoinInProgress` (setting), `bIsLANMatch` = Null only, `bUsesPresence` / `bUseLobbiesIfAvailable` = not Null. Custom settings: `MAPNAME` (Host Map asset name), `FRONTEND_SESSION_DISPLAY_NAME` (text from the Host screen). On success `StartSession` is fired and the host travels; a LAN host adds `?bIsLanMatch` to the URL (`GetHostTravelOptions`). |
| **Find** | `FOnlineSessionSearch` with `bIsLanQuery` = Null only, `SEARCH_PRESENCE` + `SEARCH_LOBBIES` otherwise (EOS creates lobbies for `bUseLobbiesIfAvailable` hosts and only returns them when asked). Each result → `UFrontendSessionListEntryData`: display name from the custom setting (falls back to the host's user name), `Current = Max - NumOpenPublicConnections`, host name, map, ping. |
| **Join** | Index into the last search results. `JoinSession` → `GetResolvedConnectString` → client travel. `SessionIsFull`, `SessionDoesNotExist`, `CouldNotRetrieveAddress`, `AlreadyInSession` map to readable messages. |
| **Leave** | `DestroySession(GameSession)`. `UGameUIFlowSubsystem::ReturnToFrontend()` calls this before travelling back so the next Host/Join is clean. |
| **Leaderboard** | If the subsystem has no leaderboard interface (Null): debug line + simulated rows (or empty if the setting is off). |

The engine-level session name is always `NAME_GameSession`, which is what the default
`AGameSession` registers players against. The name the player types is a session
setting, not the session identifier.

Every step is written to the API debug panel with an `[oss]` prefix.

## Testing on one machine

Null discovery is a UDP LAN broadcast between **separate processes**. The editor's Play
button will not give you two of those: *Net Mode = Play Standalone* always launches a
single instance (Number of Players only applies to *Listen Server* / *Client*, and those
auto-connect to the current map, skipping the Host → Browse → Join flow entirely).

Launch the instances yourself instead:

1. Close PIE. Double-click **`Scripts/LaunchInstances.bat`** (edit `UE_ROOT` at the top
   if the engine is not under `D:\EpicGames\UE_5.6`). It starts two 1280x720 windows side
   by side, each with its own log (`Saved/Logs/Instance1.log`, `Instance2.log`).
   `LaunchInstances.bat 2 EOS` runs the same two windows on Epic Online Services
   ([Docs/EOSSetup.md](../../../Docs/EOSSetup.md)).
2. Allow the Windows Firewall prompt for private networks the first time — the LAN
   beacon needs it.
3. Treat the left window as **A** (host) and the right one as **B** (client).

Equivalent manual command, run twice:
```
"D:\EpicGames\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" "<path>\FrontendUI.uproject" -game -windowed -ResX=1280 -ResY=720 -log
```


| # | Step | Expect |
|---|---|---|
| 1 | A: Multiplayer → Host → "Alpha", 4 → Create | `[oss] CreateSession 'Alpha'` → `OnCreateSessionComplete GameSession -> OK` → `ServerTravel /Game/Maps/L_MatchTest?listen`; A is in the match |
| 2 | B: Multiplayer → Server Browser → Refresh | Row "Alpha" 1/4, map `L_MatchTest` |
| 3 | B: Join | `OnJoinSessionComplete GameSession -> Success` → `ClientTravel 127.0.0.1:7777`; B is in the match, A's log shows the join |
| 4 | B: pause → Return to Main Menu | B in frontend; A still hosting; B's Refresh shows "Alpha" 1/4 again |
| 5 | A: Return to Main Menu | `OnDestroySessionComplete -> OK`; B's Refresh → empty state |
| 6 | A: Host again, same name | Succeeds (destroy-before-create path) |
| 7 | Host Map = None, Host | Ok modal "Host Map is not set…", no travel |
| 8 | Leaderboard | Simulated rows + `Leaderboards are not supported by NULL` |
| 9 | Backend = Stub | Host still travels offline; Join shows the "simulated backend" modal |

## Switching services

| Service | Change |
|---|---|
| **EOS** | Already wired: [Docs/EOSSetup.md](../../../Docs/EOSSetup.md). Product keys in the git-ignored `Saved/Config/WindowsEditor/Engine.ini`, `LaunchInstances.bat 2 EOS` or `-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=EOS`. No code change. |
| **Steam** | Enable `OnlineSubsystemSteam`; `DefaultPlatformService=Steam`; add the `[OnlineSubsystemSteam]` block (`bEnabled`, `SteamDevAppId=480`) and the Steam net driver definition. Steam signs in through the running client, so the credential chain is a no-op. No code change. |
| **Dedicated / GameLift** | New `UFrontendSessionBackend` subclass that calls your matchmaking API and reports the connect string through `ReportSessionJoined`; the UI and travel logic stay the same. |

## Writing your own backend

Subclass `UFrontendSessionBackend`, override the five requests, and answer **every**
request with exactly one matching `Report*` call (success or failure). Call
`ReportOperationStarted` first so the screens disable their buttons. If your service
needs an account, override `IsLoggedIn` / `Login` / `GetLocalPlayerDisplayName` and
answer `Login` with `ReportLoginChanged`. Use `LogDebug` generously; it is free and
shows up in the API debug panel. Select the class in Frontend Multiplayer Settings —
nothing else needs to change.
