# Starting a new game from this repository

This repository is a **template**, not only a tutorial result. All code lives in three
plugins; the project itself has one deliberately empty primary game module
(`Source/FrontendUIGame`, required for a packaged executable), two test maps and the
config that wires the plugins together. Starting a game from it means keeping the
plugins, renaming the shell, and replacing the reference content with yours.

Allow an afternoon for the steps below. Everything after step 3 works in any order.

## What you keep, copy, or delete

| Path | Role | Do |
|---|---|---|
| `Plugins/FrontendUI` | Widget stacks, loading-screen holds, options / key remap, modals, input routing. Game-agnostic. | **Keep as is.** Update it from this repo later by copying the folder. |
| `Plugins/FrontendMultiplayer` (+ `FrontendMultiplayerOSS`) | Session subsystem, backend seam, Host / Browser / Leaderboard / Debug / Sign-in screens, LAN and EOS backends. Game-agnostic. | **Keep as is.** Swap or add backends (`Docs/SessionBackend.md`), never edit screens for game needs: subclass them. |
| `Plugins/GameUI` | The *reference* game: player controller, game modes, HUD, pause, lobby, settings registries, brand assets, input mappings, widget Blueprints. | **Copy and own.** Rename to `<YourGame>UI` or merge into your game module. This is where your changes go. |
| `Source/FrontendUIGame` | Primary game module. Empty apart from `IMPLEMENT_PRIMARY_GAME_MODULE`; a monolithic build needs exactly one. | **Rename** to `<YourGame>` (see below). Put code here only if it is truly project-specific. |
| `Content/Maps/FrontEndTestMap`, `L_MatchTest` | Two test maps: frontend camera scene and an empty match level. | Replace with yours; keep the GameMode overrides they demonstrate. |
| `Content/Movies`, `Content/Assets` | Startup movie placeholder, test assets. | Replace. |
| `Config/*.ini` | Plugin wiring (sections listed in the README), EOS sections, net driver definitions. | Keep; edit values, not structure. |
| `Scripts/LaunchInstances.bat` | Two-instance LAN / EOS / packaged launcher. | Keep; edit `UE_ROOT`. |
| `Docs/Demo.md`, `Docs/*Setup.md` | Walkthroughs for this repo's demo. | Delete `Demo.md`; keep `EOSSetup.md` if you use EOS. |

## 1. Clone and rename

```cmd
git clone https://github.com/Gall-oDrone/Tutorial-FrontendUI-Experience-In-UnrealEngine-5-Using-C-And-Common-UI.git MyGame
cd MyGame
git remote set-url origin <your-repo-url>
```

Rename the project shell:

| Rename | To |
|---|---|
| `FrontendUI.uproject` | `MyGame.uproject` |
| `Source/FrontendUIGame/` folder, `FrontendUIGame.Build.cs` class, `FrontendUIGame.h/.cpp`, `FFrontendUIGameModule` | `MyGame/`, `MyGame`, `MyGame.h/.cpp`, `FMyGameModule`; in `.cpp`: `IMPLEMENT_PRIMARY_GAME_MODULE(FMyGameModule, MyGame, "MyGame")` |
| `.uproject` → `"Modules"[0].Name` | `MyGame` |
| `Source/FrontendUI.Target.cs`, class `FrontendUITarget`, `ExtraModuleNames.Add("FrontendUIGame")` | `MyGame.Target.cs`, `MyGameTarget`, `ExtraModuleNames.Add("MyGame")` |
| `Source/FrontendUIEditor.Target.cs`, class `FrontendUIEditorTarget`, same `ExtraModuleNames` line | `MyGameEditor.Target.cs`, `MyGameEditorTarget` |
| `Config/DefaultGame.ini` → `ProjectName`, `ProjectID` | your name, a new GUID |
| `Config/DefaultEngine.ini` → `DefaultArtifactName=FrontendUI` (EOS only) | `MyGame`, and the same name in your `+Artifacts=` line |
| `Scripts/LaunchInstances.bat` → `FrontendUI.uproject`, `FrontendUI.exe`, window titles | your names |

The plugin names (`FrontendUI`, `FrontendMultiplayer`, `GameUI`) can stay; they are
namespaces, not your game's name. If you want `GameUI` renamed, do it now with a
find-and-replace over `Plugins/GameUI` (folder, `.uplugin`, `Build.cs`, module class,
`GAMEUI_API`, log category) **before** opening the editor once, so no redirectors are
needed.

Right-click the `.uproject` → **Generate Visual Studio project files**, build
**Development Editor**, open the editor. You should see the exact frontend this repo
ships with.

## 2. Make the frontend yours (editor work, no code)

In order of visibility:

1. **Brand**: replace `Plugins/GameUI/Content/Brand/T_StudioLogo` and the logo screen
   timing in `WBP_CAW_LogoScreen`.
2. **Main menu**: `WBP_CAW_MainMenu` buttons and their target screens. The Frontend
   Widget Map in Project Settings → Game → Frontend UI Settings decides which Blueprint
   answers each `Frontend.Widget.*` tag; add your own tags in
   `Plugins/GameUI/Source/GameUI/Private/GameUIGameplayTags.cpp` and register the widgets there.
3. **Options**: subclass `UOptionsDataRegistry` / `UFrontendGameUserSettings` in your
   module and point the two settings at them (`Plugins/GameUI/README.md`). Add your
   game's options there; the screens render whatever the registry declares.
4. **Input**: `Plugins/GameUI/Content/Input` holds the Enhanced Input actions and the
   Common Input data tables (icons per platform). Replace actions, keep the tables.
5. **Styles**: `Plugins/FrontendUI/Content/UI/Styles` holds the Common UI text and button
   styles every screen references. Edit them in place; screens pick the change up.

## 3. Make the match yours

- `BP_MatchGameMode` (parent `AGameUIMatchGameMode`) owns the ready-gated lobby and the
  countdown. Keep it as your match game mode's parent and override
  `HandleMatchHasStarted` / `HandleMatchHasEnded` for your rules
  (`Plugins/GameUI/Docs/Lobby.md`).
- `AGameUIMatchPlayerController` owns the HUD, pause menu and return-to-frontend flow.
  Your pawn, camera and gameplay input go in a subclass.
- `WBP_HUD` is deliberately minimal; it is the one widget you will rewrite entirely.

## 4. Pick the session backend

| You want | Do |
|---|---|
| LAN play only | Nothing. `DefaultPlatformService=Null` is the default. |
| Epic Online Services | `Docs/EOSSetup.md`. ~1 hour including the portal. |
| Steam | `DefaultPlatformService=Steam`, enable `OnlineSubsystemSteam`, Steam App ID in `DefaultEngine.ini`. Same backend class; untested in this repo, expected to work for sessions (not P2P transport). |
| Dedicated servers | `UFrontendSessionBackend_FixedAddress` ([Docs/DedicatedServer.md](DedicatedServer.md)) for a known IP:port or Docker server, or `UFrontendSessionBackend_GameLift` ([Docs/GameLift.md](GameLift.md)) for Amazon GameLift Anywhere and Managed Container Fleets. |

## 5. Package once before building features

Package a Development Win64 build and run `Scripts\LaunchInstances.bat 2 LAN PACKAGED`.
Everything in this repo is tested that way; doing it on day one means a cooking problem
is yours to notice when the diff is small.

```
"D:\EpicGames\UE_5.6\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="D:\EpicGames\Projects\UE_5.6\FrontendUI\FrontendUI.uproject" -platform=Win64 -clientconfig=Development -build -cook -stage -pak -nop4 -utf8output
```

The staged game lands in `Saved\StagedBuilds\Windows\` (the launcher's default
`PACKAGED_EXE`) and, because it runs in place, writes its logs to
`Saved\StagedBuilds\Windows\FrontendUI\Saved\Logs\`, not the project's `Saved\Logs`.
UBT sizes its parallelism by free memory (`Requested 1.5 GB memory per action ...
limiting max parallel actions to N` in the output), so close the editor and browsers
first on an 8 GB machine. Common causes when the packaged frontend differs from PIE:


| Symptom | Cause |
|---|---|
| Link errors for `GInternalProjectName`, `FMemory_Malloc`, `GIsGameAgnosticExe` | the project has no primary game module. The editor never needs one, a monolithic exe always does: keep `Source/<YourGame>/` with its `IMPLEMENT_PRIMARY_GAME_MODULE` and list the module in the `.uproject` and both `Target.cs` files. |
| A screen never appears | its Blueprint is only reachable through a soft reference that nothing cooks. Add the folder to **Project Settings → Packaging → Additional Asset Directories to Cook**, or reference it from the Frontend Widget Map (which is cooked). |
| Options tab missing | `bEnableUserSettings` or the registry class not set in the *packaged* config layer (`Default*.ini`, not `Saved/`). |
| EOS falls back to Null | keys are in `Config/UserEngine.ini`, which is not staged. See `Docs/EOSSetup.md` section 2. |

## 6. Housekeeping

- Delete `Docs/Demo.md`, `Plugins/GameUI/Content/Developers`, `Content/Assets` once you
  have your own.
- `LICENSE.txt` is MIT; keep the attribution line for the plugins or replace the file for
  your game's licence (the plugins stay MIT).
- Tags: this repo's `v0.x` / `v1.0` tags describe the template's history, not your
  game's. Start your own numbering.

## Where things are documented

| Topic | File |
|---|---|
| Plugin wiring, config sections, the three assets every game supplies | `README.md` |
| Widget trees for every screen (what to build in UMG and why) | `Plugins/FrontendMultiplayer/Docs/WidgetDesignTemplate.md` |
| Session seam, backends, debug panel lines | `Plugins/FrontendMultiplayer/Docs/SessionBackend.md` |
| In-match UI, pause, failure recovery | `Plugins/GameUI/Docs/InGameUI.md` |
| Lobby, ready-up, countdown | `Plugins/GameUI/Docs/Lobby.md` |
| EOS portal, keys, sign-in, troubleshooting | `Docs/EOSSetup.md` |
| Dedicated servers (FixedAddress, Server target, Docker) | `Docs/DedicatedServer.md` |
| Amazon GameLift (Anywhere, WebSocket SDK, Container Fleets) | `Docs/GameLift.md` |
