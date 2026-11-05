# Starting a new game from this repository

This repository is a **template**, not only a tutorial result. All code lives in three
plugins; the project itself has no game module (`"Modules": []` in the `.uproject`), two
test maps and the config that wires the plugins together. Starting a game from it means
keeping the plugins, renaming the shell, and replacing the reference content with yours.

Allow an afternoon for the steps below. Everything after step 3 works in any order.

## What you keep, copy, or delete

| Path | Role | Do |
|---|---|---|
| `Plugins/FrontendUI` | Widget stacks, loading-screen holds, options / key remap, modals, input routing. Game-agnostic. | **Keep as is.** Update it from this repo later by copying the folder. |
| `Plugins/FrontendMultiplayer` (+ `FrontendMultiplayerOSS`) | Session subsystem, backend seam, Host / Browser / Leaderboard / Debug / Sign-in screens, LAN and EOS backends. Game-agnostic. | **Keep as is.** Swap or add backends (`Docs/SessionBackend.md`), never edit screens for game needs: subclass them. |
| `Plugins/GameUI` | The *reference* game: player controller, game modes, HUD, pause, lobby, settings registries, brand assets, input mappings, widget Blueprints. | **Copy and own.** Rename to `<YourGame>UI` or merge into your game module. This is where your changes go. |
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

Rename the project shell (no C++ module to rename, only files and strings):

| Rename | To |
|---|---|
| `FrontendUI.uproject` | `MyGame.uproject` |
| `Source/FrontendUI.Target.cs`, class `FrontendUITarget`, `ExtraModuleNames` (empty) | `MyGame.Target.cs`, `MyGameTarget` |
| `Source/FrontendUIEditor.Target.cs`, class `FrontendUIEditorTarget` | `MyGameEditor.Target.cs`, `MyGameEditorTarget` |
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
| Dedicated servers | Phase 8 of this repo (GameLift). Until then, the seam is `UFrontendSessionBackend`; see `Plugins/FrontendMultiplayer/Docs/SessionBackend.md` → "Writing your own backend". |

## 5. Package once before building features

Package a Development Win64 build and run `Scripts\LaunchInstances.bat 2 LAN PACKAGED`.
Everything in this repo is tested that way; doing it on day one means a cooking problem
is yours to notice when the diff is small. Common causes when the packaged frontend
differs from PIE:

| Symptom | Cause |
|---|---|
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
