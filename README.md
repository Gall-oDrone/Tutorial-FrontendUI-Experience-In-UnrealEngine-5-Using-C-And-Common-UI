# FrontendUI

A from-scratch, AAA-style frontend and menu system for Unreal Engine 5, written in C++ on top of CommonUI. It demonstrates how to build a menu flow that scales past hand-wired widget references: a primary layout widget owns a set of CommonUI activatable widget stacks, every screen is addressed by a gameplay tag rather than a hard class reference, and screens are pushed asynchronously so their assets load on demand.

The repository is split into three plugins with a one-way dependency chain, so the framework can be dropped into another game unchanged:

```
GameUI  ──►  FrontendMultiplayer  ──►  FrontendUI  ──►  CommonUI, EnhancedInput
(this game)   (session UI)             (framework)
```

- **`FrontendUI`** — the reusable framework. Stacks, async push, loading screen, data-driven options with key remapping, confirm dialog, styles and controller glyphs. References nothing above it.
- **`FrontendMultiplayer`** — host / server browser / leaderboard / API debug screens, built on FrontendUI, talking to a pluggable `UFrontendSessionBackend`. Ships with a simulated stub backend.
- **`GameUI`** — everything that names *this* game: boot flow, main menu, logo, input actions, and the game's subclasses of the framework extension points. A new game replaces this plugin and keeps the other two.

## Requirements

- **Unreal Engine 5.6** (`EngineAssociation` in `FrontendUI.uproject`)
- A C++ toolchain for your platform (developed against Visual Studio 2022 on Windows)

`FrontendUI.uplugin` declares two engine plugin dependencies, `CommonUI` and `EnhancedInput`, and links these modules (`FrontendUI.Build.cs`): `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `GameplayTags`, `UMG`, `CommonUI`, `CommonInput`, `DeveloperSettings`, `PropertyPath`, and `PreLoadScreen`, plus `Slate` and `SlateCore` privately.

The demo project additionally enables `MediaMovieStreamer` for the startup movie and `ModelingToolsEditorMode` for the editor.

## Project structure

```
FrontendUI.uproject
Config/                              Project settings that drive the plugins
Content/
  Maps/FrontEndTestMap               The level you launch
  Movies/                            Startup and loading movies (must stay under Content/)
  Assets/                            LevelVisuals (mannequins), non-UI art
Plugins/
  FrontendUI/                        The reusable framework
    Source/FrontendUI/               Runtime module
    Content/UI/                      Press Any Key, Options, Confirm, Loading, styles, input data
    Content/Assets/                  ControllerIcons, Fonts/Din, Materials, Textures
  FrontendMultiplayer/               Session + leaderboard screens, IFrontendSessionBackend
    Source/FrontendMultiplayer/      Widgets, subsystem, Backends/ (Stub)
    Content/UI/                      Multiplayer hub, Host, Server Browser, Leaderboard, API Debug
    Docs/WidgetDesignTemplate.md     Authoring spec for the multiplayer Blueprints
  GameUI/                            This game's UI layer
    Source/GameUI/Settings/          UGameUIOptionsDataRegistry, UGameUIGameUserSettings
    Content/Core/                    BP_FrontendController, BP_FrontendGameMode, WBP_CUW_PrimaryLayout
    Content/UI/Screens/              Main Menu, Story, Credits
    Content/Brand/                   Studio logo screen and texture
    Content/Input/                   IMC_Default, IA_*
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
| Session backend | `UFrontendSessionBackend` subclass → **Frontend Multiplayer Settings → Session Backend Class** | `UFrontendSessionBackend_Stub` (framework default) |
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

## Roadmap

- In-game half of `GameUI`: pause menu and HUD base on the `GameMenu` / `GameHud` stacks, input-mode switching, return-to-menu through the loading screen.
- A real `UFrontendSessionBackend` (OnlineSubsystem / EOS), then AWS GameLift for dedicated-server hosting and session placement, selected purely from Project Settings.
- Studio logo as a startup movie or `FPreLoadScreenBase` so it precedes the loading screen.

## License

Released under the MIT License — see [LICENSE.txt](LICENSE.txt).

## Credits

Created by **Diego Gallo**.

The frontend framework was built by following [Unreal Engine 5 C++ Advanced Frontend UI Programming](https://deloittedevelopment.udemy.com/course/ureal-engine-5-cpp-advanced-frontend-ui-programming/) by **Vince Petrelli** on Udemy. Full credit for the architecture and techniques goes to that course. The conversion of the module into standalone plugins, the multiplayer UI, the session backend seam, and the loading-screen hold mechanism are not part of the course material and were done separately for this repository.
