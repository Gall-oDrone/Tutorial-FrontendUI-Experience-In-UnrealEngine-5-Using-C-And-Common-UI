# FrontendUI

A from-scratch, AAA-style frontend and menu system for Unreal Engine 5, written in C++ on top of CommonUI. It demonstrates how to build a menu flow that scales past hand-wired widget references: a primary layout widget owns a set of CommonUI activatable widget stacks, every screen is addressed by a gameplay tag rather than a hard class reference, and screens are pushed asynchronously so their assets load on demand.

The repository is split into three plugins with a one-way dependency chain, so the framework can be dropped into another game unchanged:

```
GameUI  ──►  FrontendMultiplayer  ──►  FrontendUI  ──►  CommonUI, EnhancedInput
(this game)   (session UI)             (framework)
```

- **`FrontendUI`** — the reusable framework. Stacks, async push, loading screen, data-driven options with key remapping, confirm dialog, styles and controller glyphs. References nothing above it.
- **`FrontendMultiplayer`** — host / server browser / leaderboard / API debug screens, built on FrontendUI, talking to a pluggable `UFrontendSessionBackend`. Ships with a simulated stub and, in a separate `FrontendMultiplayerOSS` module, a real OnlineSubsystem backend (Null / Steam / EOS by config).
- **`GameUI`** — everything that names *this* game: boot flow, main menu, logo, input actions, and the game's subclasses of the framework extension points. A new game replaces this plugin and keeps the other two.

> **Starting a game from this repository?** Read [Docs/StartingANewGame.md](Docs/StartingANewGame.md): what to keep, what to rename, what to replace, and why to package on day one.

## Requirements

- **Unreal Engine 5.6** (`EngineAssociation` in `FrontendUI.uproject`)
- A C++ toolchain for your platform (developed against Visual Studio 2022 on Windows)

`FrontendUI.uplugin` declares two engine plugin dependencies, `CommonUI` and `EnhancedInput`, and links these modules (`FrontendUI.Build.cs`): `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `GameplayTags`, `UMG`, `CommonUI`, `CommonInput`, `DeveloperSettings`, `PropertyPath`, and `PreLoadScreen`, plus `Slate` and `SlateCore` privately.

The demo project additionally enables `MediaMovieStreamer` for the startup movie and `ModelingToolsEditorMode` for the editor.

## Project structure

```
FrontendUI.uproject
Config/                              Project settings that drive the plugins
Source/FrontendUIGame/               Primary game module (empty by design; a packaged exe needs one)
Content/
  Maps/FrontEndTestMap, L_MatchTest  Frontend map and the match map the lobby/HUD run in
  Movies/                            Startup and loading movies (must stay under Content/)
  Assets/                            LevelVisuals (mannequins), non-UI art
Plugins/
  FrontendUI/                        The reusable framework
    Source/FrontendUI/               Runtime module
    Content/UI/                      Press Any Key, Options, Confirm, Loading, styles, input data
    Content/Assets/                  ControllerIcons, Fonts/Din, Materials, Textures
  FrontendMultiplayer/               Session + leaderboard screens, UFrontendSessionBackend
    Source/FrontendMultiplayer/      Widgets, subsystem, Backends/ (Stub, FixedAddress, GameLift)
    Source/FrontendMultiplayerOSS/   UFrontendSessionBackend_OSS (IOnlineSession)
    Content/UI/                      Multiplayer hub, Host, Server Browser, Leaderboard, API Debug
    Docs/WidgetDesignTemplate.md     Authoring spec for the multiplayer Blueprints
    Docs/SessionBackend.md           Backend contract, OSS setup, LAN test plan
  GameUI/                            This game's UI layer
    Source/GameUI/Settings/          UGameUIOptionsDataRegistry, UGameUIGameUserSettings, UGameUIDeveloperSettings
    Source/GameUI/GameModes/         AGameUIMatchGameMode / GameState / PlayerState (lobby + match)
    Source/GameUI/Controllers/       AGameUIMatchPlayerController
    Source/GameUI/Widgets/           HUD base, pause menu, lobby screen and row
    Content/Core/                    BP_FrontendController, BP_FrontendGameMode, BP_MatchGameMode, WBP_CUW_PrimaryLayout
    Content/UI/Screens/              Main Menu, Story, Credits, HUD, Pause Menu, Lobby/
    Content/Brand/                   Studio logo screen and texture
    Content/Input/                   IMC_Default, IMC_Match, IA_*, DT_GameUIInputActions
    Content/Developers/              Scratch assets, never cooked
    Docs/InGameUI.md                 Pause menu + HUD authoring spec
    Docs/Lobby.md                    Pre-match lobby authoring spec
    Docs/History.md                  How content was moved in during the split
Docs/Demo.md                         Recording the demo video
Docs/EOSSetup.md                     Epic Online Services: portal, keys, sign-in, test plan
Docs/DedicatedServer.md              Dedicated servers: FixedAddress backend, one-PC test, Server target, build box
Docs/GameLift.md                     Amazon GameLift: Anywhere, Server WebSocket SDK, Docker & Container Fleets
Docs/StartingANewGame.md             Using the repository as a template for a new game
Dockerfile / docker-compose.yml      Linux dedicated server container image and local/Anywhere compose runner
Scripts/LaunchInstances.bat          N game instances, LAN / EOS / DEDICATED / GAMELIFT, editor binary or packaged exe
Scripts/LaunchDedicatedServer.bat    One local dedicated server (FixedAddress or GameLift Anywhere)
```

## Architecture overview

A widget derived from `UWidget_PrimaryLayout` sits at the root of the UI and calls `RegisterWidgetStack` to associate each of its CommonUI activatable widget containers with a `Frontend.WidgetStack.*` tag, giving four layers: `Frontend`, `GameMenu`, `GameHud`, and `Modal`. Screens are never referenced by class from calling code. Instead each one is identified by a `Frontend.Widget.*` tag, and `FrontendWidgetMap` in Project Settings maps that tag to a soft widget class. To show a screen, callers pass a stack tag and a widget tag to `UFrontendUISubsystem::PushSoftWidgetToStackAsync` (or the Blueprint node **Push Soft Widget To Widget Stack**), which resolves the tag, streams the class in, constructs the widget, and pushes it onto the requested stack. Adding a screen means adding a tag and a map entry; the routing never changes. `FrontendMultiplayer` adds five screens this way without touching `FrontendUI`.

The options screen is data-driven rather than hand-built per row. Each setting is a `UListDataObject_Base` subclass — scalar, string, resolution, key remap — which reads and writes its value through `FOptionsDataInteractionHelper`. `UOptionsDataRegistry` assembles those into tabs, `DataAsset_DataListEntryMapping` maps each data object type to the row widget that renders it, and `UFrontendCommonListView` builds the list.

The loading screen is a `UGameInstanceSubsystem` that stays up while the engine reports reasons (level loading, world init, texture streaming) and additionally while any system holds it via `RegisterLoadingScreenHold(Tag, Reason)`. The controller uses that to keep the screen up until the first frontend widget is on the stack, so there is never a frame of bare level between loading and UI.

### Extension points

The framework is extended by subclassing, never by editing it. Each seam has a Project Settings slot and falls back to the framework default when empty.

| Seam | Where to plug in | GameUI's implementation |
|---|---|---|
| Options tabs | `UOptionsDataRegistry` subclass → **Frontend UI Settings → Options Data Registry Class** | `UGameUIOptionsDataRegistry` adds the demo image row to the Gameplay tab |
| Applying settings | `UFrontendGameUserSettings` subclass → `GameUserSettingsClassName` in `DefaultEngine.ini`; override `ApplyAudioSettings()`, bind `OnGameDifficultyChanged` | `UGameUIGameUserSettings` |
| Session backend | `UFrontendSessionBackend` subclass → **Frontend Multiplayer Settings → Session Backend Class** + **Host Map** | `UFrontendSessionBackend_OSS` over the Null subsystem (LAN) or EOS, chosen per launch (`UFrontendSessionBackend_Stub` is the framework default) |
| Screens | tag + `FrontendWidgetMap` entry | main menu, story, credits, logo |
| Loading screen | `RegisterLoadingScreenHold` / `ClearLoadingScreenHold` from any system | controller holds until the first screen is pushed |

Dev-only switches (`Show Logo Screen`, `Show API Debug Panel`) are compiled to fixed values in Shipping so configuration cannot ship a build without the splash or with debug UI.

## Screens included

| Plugin | Screen | Notes |
|---|---|---|
| FrontendUI | **Press Any Key** | identity/attract gate before the main menu |
| FrontendUI | **Options** | tabbed, data-driven; **Controls** tab built on Enhanced Input user settings |
| FrontendUI | **Confirm** | modal `Ok` / `YesNo` / `OkCancel` (`EConfirmScreenType`), result via `EConfirmScreenButtonType` |
| FrontendUI | **Loading Screen** | `UFrontendLoadingScreenSubsystem` + `PreLoadScreen`, optional startup movie |
| FrontendMultiplayer | **Multiplayer hub**, **Host Session**, **Server Browser**, **Leaderboard**, **API Debug** | gated by *Enable Multiplayer*; backend-agnostic |
| GameUI | **Main Menu**, **Story**, **Credits**, **Logo** | this game's screens |
| GameUI | **HUD**, **Pause Menu** | in-match, on the `GameHud` / `GameMenu` stacks; pause reuses the framework Options screen |

## Using FrontendUI in your game

This is the host contract, verified by installing `Plugins/FrontendUI` alone into a blank UE 5.6 Blueprint project and booting it to Press Any Key → Options → Confirm. Every item is required; every misconfiguration below now fails with a message that names the fix.

### 1. Install

1. Copy `Plugins/FrontendUI/` (`.uplugin`, `Source/`, `Content/`, `Resources/` — not `Binaries/` or `Intermediate/`) into your project's `Plugins/`.
2. Add `{ "Name": "FrontendUI", "Enabled": true }` to the `Plugins` array in your `.uproject`. `CommonUI` and `EnhancedInput` are enabled automatically as dependencies.
3. Open the project and accept the prompt to rebuild the missing module. Content-only projects work; the editor compiles the plugin against your installed engine.

### 2. Configure — before the editor launches

These are read once at startup. Add them with the editor **closed**, or restart after.

```ini
; Config/DefaultEngine.ini
[/Script/Engine.Engine]
GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient
GameUserSettingsClassName=/Script/FrontendUI.FrontendGameUserSettings

; Config/DefaultInput.ini
[/Script/Engine.InputSettings]
DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput
DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent

[/Script/EnhancedInput.EnhancedInputDeveloperSettings]
bEnableUserSettings=True
```

`GameViewportClientClassName` is required by CommonUI; `GameUserSettingsClassName` gives the options screen something to read and write; `bEnableUserSettings` is what key remapping is built on (without it the Controls tab is omitted with a warning).

### 3. Configure — in Project Settings

Set these through the UI rather than pasting ini lines; the editor writes them correctly and applies them immediately.

| Page | Setting | Value |
|---|---|---|
| Game → Frontend UI Settings | **Frontend Widget Map** | `PressAnyKeyScreen → WBP_CAW_PressAnyKey`, `OptionsScreen → WBP_CAW_OptionsScreen`, `KeyRemapScreen → WBP_CAW_KeyRemapScreen`, `ConfirmScreen → WBP_CAW_ConfirmScreen`, `MainMenuScreen → your main menu` |
| Game → Frontend Loading Screen Settings | **Soft Loading Screen Widget Class** | `WBP_LoadingScreen` |
| Game → Common Input Settings | **Input Data** | `InputData_Default` |
| Game → Common Input Settings → Platform Input → Windows | **Controller Data** | `ControllerData_MouseKeyboard`, `ControllerData_Gamepad`; tick *Supports Gamepad* |
| Game → Common UI Settings | **Common Button Accept Key Handling** | `TriggerClick` |

`MainMenuScreen` is whatever your game pushes after Press Any Key. While prototyping you can point it at `WBP_CAW_OptionsScreen`.

### 4. Supply three assets

- **Primary layout** — a Widget Blueprint whose parent is `Widget Primary Layout`: an `Overlay` with four `Common Activatable Widget Stack` children (`GameHud`, `GameMenu`, `Frontend`, `Modal`, bottom to top), each marked **Is Variable**. On **Event On Initialized**, call `Register Widget Stack` once per stack with its `Frontend.WidgetStack.*` tag.
- **Player controller** — parent `Frontend Player Controller`, implementing `Frontend Loading Screen Interface`. On **BeginPlay**:
  `Register Loading Screen Hold("Frontend", …)` → `Create Widget` (your layout) → `Add to Viewport` → `Register Created Primary Layout` → `Push Soft Widget To Widget Stack` (`PressAnyKeyScreen`, `Frontend.WidgetStack.Frontend`) → on **After Push**, `Clear Loading Screen Hold("Frontend")`.
- **Game mode** — Player Controller Class = the above, Default Pawn Class = `Spectator Pawn`. Set it as the map's GameMode Override, and place a Camera Actor tagged `Default` for the controller to look through.

`Plugins/GameUI/Content/Core/` holds the reference versions of all three.

### 5. Extend

Subclass `UOptionsDataRegistry` and `UFrontendGameUserSettings` in your game module and point the two settings in the table above at them. See `Plugins/GameUI/README.md` for the pattern and `Plugins/GameUI/Source/GameUI/Settings/` for the code.

## Releases

Each tag is a working, tested state. Pick one to see how a feature was added.

| Tag | Adds | Read |
|---|---|---|
| `v0.3-plugin-split` | Three-plugin split; `FrontendUI` proven standalone in a blank project; loading-screen holds; session backend seam with a stub | `Plugins/GameUI/README.md`, `Plugins/FrontendMultiplayer/Docs/WidgetDesignTemplate.md` |
| `v0.4-in-game-ui` | Match player controller / game mode, HUD, pause menu, return-to-frontend and network-failure recovery | `Plugins/GameUI/Docs/InGameUI.md` |
| `v0.5-session-backend` | `FrontendMultiplayerOSS`: real host / find / join over the OnlineSubsystem (Null on LAN), subsystem-owned travel, `Scripts/LaunchInstances.bat` | `Plugins/FrontendMultiplayer/Docs/SessionBackend.md` |
| `v0.6-lobby` | Ready-gated pre-match lobby on `AGameMode` match states, replicated countdown, lobby widgets | `Plugins/GameUI/Docs/Lobby.md` |
| `v0.6.1` | Polish: per-module log categories, EOS SDK disabled until used, readable lobby names, demo recipe | `Docs/Demo.md` |
| `v0.7-eos` | Epic Online Services by config: lazy sign-in with a credential chain (`Login` / `IsLoggedIn` / `OnLoginStateChanged` on the subsystem), EOS lobbies, `NetDriverEOSBase` P2P with IP fallback, `LaunchInstances.bat 2 EOS`; product keys stay out of git | `Docs/EOSSetup.md` |
| `v1.0` | Template readiness: verified on a packaged Development build in LAN and EOS modes (`LaunchInstances.bat 2 LAN PACKAGED` / `2 EOS PACKAGED`), EOS ids for packaged games, `persistentauth` → `accountportal` sign-in without a Dev Auth Tool, Sign-in panel (`Widget_SignInPanel`, `RequiresLogin`), primary game module, hardening pass (LOCTEXT, delegate/async lifetimes, Shipping guards), new-game guide | `Docs/StartingANewGame.md` |
| `v1.0.1` | Host advertises its name as `FRONTEND_SESSION_HOST_NAME` (EOS leaves `OwningUserName` empty for searchers); one disconnect = one return to the frontend (`bReturnToFrontendInFlight`); `DisableNativeTick` meta typo fixed in every widget header | `Plugins/FrontendMultiplayer/Docs/SessionBackend.md` |
| `v1.1-gamelift-anywhere` | Dedicated servers: `FrontendUIServer` target, `ReportSessionCreatedRemote` on the backend contract, `UFrontendSessionBackend_FixedAddress` (known IP:port), `UFrontendSessionBackend_GameLift` (client SigV4 / proxy JSON-RPC, Server SDK 5.x WebSocket protocol, `PlayerSessionId` validation in `PreLogin`/`Logout`, one-session-per-process recycling, capacity retry) with no external SDK binaries, Linux `Dockerfile` / `docker-compose.yml`, `LaunchDedicatedServer.bat` and `GAMELIFT` launch mode; verified end-to-end on a GameLift Anywhere fleet with the Launcher engine | `Docs/DedicatedServer.md`, `Docs/GameLift.md` |
| `v1.1.1-linux-server` | Packaged Linux and Windows dedicated servers built from the UE 5.6.1 source snapshot on a headless EC2 box over SSM (companion repo `infrastructure/scripts/build`); Docker image 1.2 GB, engine binary as PID 1, `TerminateServerProcess` on shutdown; network version pinned to `ProjectVersion` so Launcher-built clients and source-built servers interoperate; clang `-Wrange-loop-construct` fix; Linux container and Windows packaged server both verified on the Anywhere fleet | `Docs/GameLift.md` §4 |
| `v1.2-container-fleet` | Amazon GameLift **Managed Container Fleet** support: env-driven `docker/entrypoint.sh` (same image for Anywhere, container fleets and local), Server SDK 5.2-style SigV4 WebSocket handshake for container computes (ECS task-role credentials + task id as host id, no auth token), `sdkVersion=5.2.0`; CGD / fleet / smoke-test / teardown CLI recipe; verified Host -> Join -> Return -> Host again on a c6i.large container fleet from Launcher-built clients | `Docs/GameLift.md` §5 |
| `v1.2.1-shipping-proxy` | Shipping-build path verified: API Gateway HTTP API + Lambda SigV4 pass-through proxy (companion repo `modules/gamelift-proxy`), fleet pinned server-side, GameLift error `__type` preserved so the capacity-retry keeps working; `ApiEndpointUrl` / `-GameLiftEndpoint=` / `GAMELIFT_ENDPOINT_URL`; Host/Join/re-Host/Join from Launcher clients with 10/10 requests through the proxy | `Docs/GameLift.md` §6 |
| `v1.3-eos-identity` | Shipping path hardened: `UFrontendSessionBackend_GameLiftEOS` (GameLift + EOS sign-in, EOS access token as `Authorization: Bearer` to the proxy), proxy `auth_mode=eos` (Epic `tokenInfo` introspection, client-id check, `PlayerId`/`CreatorId` pinned to the verified account, `401` otherwise), GameLift alias for clients and proxy, dedicated servers accept EOS unique ids, AWS budget; verified Host/Join/Return/Host/Join with two Dev Auth Tool users, `CreatorId` = Epic account id | `Docs/GameLift.md` §6-§7 |
| `v1.4-queue-placement` | Host through a GameLift **game session queue**: `GameSessionQueueName` (`-GameLiftQueue=` / `GAMELIFT_QUEUE_NAME`) switches Host to `StartGameSessionPlacement` + `DescribeGameSessionPlacement` (GameLift waits for capacity, host slot reserved in the same call, `PlacementTimeoutSeconds`), Find / Join unchanged; proxy allows and pins the placement calls; companion repo gains `container-fleet.sh` (create / status / delete with a `PercentAvailableGameSessions` target policy and alias switch) and a CloudWatch dashboard + alarms; cold scale-out measured at 409 s on a 1-session-per-instance fleet, hence queue 480 s / client 510 s and the sizing guidance; verified from the CLI on a container fleet and from Launcher clients on the Anywhere fleet | `Docs/GameLift.md` §6 |
| `v1.5-session-guards` | Abandoned-session guards: the server ends a game session that gets **no player within `SessionNoPlayerExitDelaySeconds`** (120 s) of activation and recycles the process like an empty one (crashed hosts, abandoned placements, CLI probes, expired `RESERVED` slots); the client sends **`StopGameSessionPlacement`** when it walks away from a `PENDING` placement (proxy allow-list + IAM). Lobby: the Ready button toggles **on the server** (`ServerToggleReady`, two quick clicks count as two), the list refreshes when a `PlayerName` replicates (a joining row no longer sits blank), ready flips and the roster are logged. `ProjectVersion` **1.2.0** because a replicated class gained an RPC - packaged servers and the container image must be rebuilt from this revision before they accept these clients (done 2026-11-11: `FrontendUI-LinuxServer/WindowsServer-5a0dbd7-*.zip`, image `frontendui-server:5a0dbd7`, CGD version 4, plus the first **Shipping client package** - `Docs/GameLift.md` §4). Fleet shape measured: `c6i.xlarge` runs 3 sessions per instance (now the `container-fleet.sh` default). Verified on the Anywhere fleet: probe session self-terminated at 120.0 s, un-ready cancelled the countdown twice, both players listed on both clients | `Docs/GameLift.md` §3 / §6, `Plugins/GameUI/Docs/Lobby.md` |
| `v1.5.1-shipping-client` | **Shipping client verified end to end** on the packaged 1.2.0 server (2026-11-13): `run-build.sh --windows-client` bakes `ApiEndpointUrl` / `AliasId` / `Location` / `GameSessionQueueName` and the EOS artifact into a Shipping package that cannot sign AWS requests at all; Host went `StartGameSessionPlacement` → `FULFILLED` in 3.2 s, Join through `DescribeGameSessions` / `CreatePlayerSession`, exactly 4 proxy calls (all `200`) in the API Gateway access log, both players ready, session `TERMINATED` 5 s after the clients quit. Two lessons baked in: a packaged Windows build ships **no CA bundle**, so the project now stages `Content/Certificates/cacert.pem`; and **a URL in an .ini must be quoted** - the parser cuts an unquoted value at `//`, which turned the endpoint into `https:` in the first package (the backend now refuses such a value loudly). `bUseLoggingInShipping` so Shipping clients write `%LOCALAPPDATA%\FrontendUI\Saved\Logs\` | `Docs/GameLift.md` §4 / §8 |

## Roadmap

The dedicated-server and Amazon GameLift path is complete end to end: `FrontendUIServer` target, `UFrontendSessionBackend_FixedAddress` ([Docs/DedicatedServer.md](Docs/DedicatedServer.md)), `UFrontendSessionBackend_GameLift` / `_GameLiftEOS` (client SigV4 or proxy JSON-RPC, Server SDK 5.x WebSocket protocol), Linux `Dockerfile` / `docker-compose.yml`, GameLift Anywhere, Managed Container Fleets with auto-scaling, the Shipping proxy with EOS identity, alias and queue placement ([Docs/GameLift.md](Docs/GameLift.md)). What is left, in the order it is planned:

- ~~Server artifacts at `ProjectVersion` 1.2.0 and the Shipping client package~~ - done (`v1.5.1-shipping-client`): CGD `frontendui-server` v4, Windows server zip, Shipping client proven through the proxy.
- **In-app Cancel for a queued Host**: a placement can stay `PENDING` for up to the queue timeout and today the only ways out are quitting or waiting (Back hides the screen; Host / Refresh are gated on the operation in flight). A Cancel on the busy Host button should end the operation and send the Stop.
- **Server-side identity cross-check**: `PreLogin` compares GameLift's `PlayerId` (the proxy-verified Epic account id) with the connecting EOS unique id.
- ~~CI for the server image~~ - done: companion `.github/workflows/server-image.yml` (`workflow_dispatch` → EC2 box → ECR → new CGD version → optional fleet roll-out → destroy + audit, GitHub OIDC role, no keys), [Docs/GameLift.md](Docs/GameLift.md) §4.
- **Leaderboard** backed by DynamoDB through the same proxy and EOS identity (`FetchLeaderboard` is still simulated).
- Studio logo as `FPreLoadScreenBase` so it precedes the loading screen; reconnect flow and match-specific HUD content.
- Later, when there is a reason: FlexMatch on top of the queue, a second region, Spot fleets.

## License

Released under the MIT License — see [LICENSE.txt](LICENSE.txt).

## Credits

Created by **Diego Gallo**.

The frontend framework was built by following [Unreal Engine 5 C++ Advanced Frontend UI Programming](https://deloittedevelopment.udemy.com/course/ureal-engine-5-cpp-advanced-frontend-ui-programming/) by **Vince Petrelli** on Udemy. Full credit for the architecture and techniques goes to that course. The conversion of the module into standalone plugins, the multiplayer UI, the session backend seam, and the loading-screen hold mechanism are not part of the course material and were done separately for this repository.
