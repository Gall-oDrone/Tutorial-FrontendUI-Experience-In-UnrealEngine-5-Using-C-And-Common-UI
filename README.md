# FrontendUI

A from-scratch, AAA-style frontend and menu system for Unreal Engine 5, written in C++ on top of CommonUI. It demonstrates how to build a menu flow that scales past hand-wired widget references: a primary layout widget owns a set of CommonUI activatable widget stacks, every screen is addressed by a gameplay tag rather than a hard class reference, and screens are pushed asynchronously so their assets load on demand. The repository is both a working demo and a reusable plugin — the entire framework lives in `Plugins/FrontendUI/`, while the project root is just a test harness that exercises it.

## Requirements

- **Unreal Engine 5.6** (`EngineAssociation` in `FrontendUI.uproject`)
- A C++ toolchain for your platform (developed against Visual Studio 2022 on Windows)

The plugin declares two plugin dependencies in `FrontendUI.uplugin`, both shipped with the engine:

- `CommonUI`
- `EnhancedInput`

It also links these engine modules (`FrontendUI.Build.cs`): `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `GameplayTags`, `UMG`, `CommonUI`, `CommonInput`, `DeveloperSettings`, `PropertyPath`, and `PreLoadScreen`, plus `Slate` and `SlateCore` privately.

The demo project additionally enables `MediaMovieStreamer` for the startup movie and `ModelingToolsEditorMode` for the editor.

## Project structure

```
FrontendUI.uproject
Config/                            Project settings that drive the plugin
Content/                           Demo harness only
  Maps/FrontEndTestMap             The level you launch
  UI/                              BP_FrontendGameMode, BP_FrontendController
  Assets/                          Fonts/Cinzel, LevelVisuals (mannequins)
  Movies/                          Startup and loading movies
Plugins/FrontendUI/                The reusable framework
  FrontendUI.uplugin
  Source/FrontendUI/               Runtime module (Public/ and Private/)
  Content/
    UI/                            Widgets, styles, list entries, input data, string table
    Assets/                        ControllerIcons, Fonts/Din, Materials, Textures
Source/
  FrontendUI.Target.cs
  FrontendUIEditor.Target.cs
```

The split is deliberate: everything reusable is inside the plugin and references only its own content, so `Plugins/FrontendUI/` can be copied into another project as-is. What stayed at the project root is only what a host game supplies — the map, the game mode and player controller glue, and non-UI art.

## Architecture overview

A widget derived from `UWidget_PrimaryLayout` sits at the root of the UI and calls `RegisterWidgetStack` to associate each of its CommonUI activatable widget containers with a `Frontend.WidgetStack.*` tag, giving four layers: `Frontend`, `GameMenu`, `GameHud`, and `Modal`. Screens are never referenced by class from calling code. Instead each one is identified by a `Frontend.Widget.*` tag, and `FrontendWidgetMap` in Project Settings maps that tag to a soft widget class. To show a screen, callers pass a stack tag and a widget tag to `UFrontendUISubsystem::PushSoftWidgetToStackAsync` (or the Blueprint-facing `UAsyncAction_PushSoftWidget`), which resolves the tag, streams the class in, constructs the widget, and pushes it onto the requested stack. The result is that adding a screen means adding a tag and a map entry, with no code changes to the routing itself.

The options screen goes a step further and is data-driven rather than hand-built per row. Each setting is a `UListDataObject_Base` subclass — scalar, string, resolution, key remap — which knows how to read and write its value through `FOptionsDataInteractionHelper`. `UOptionsDataRegistry` assembles those into tabs, `DataAsset_DataListEntryMapping` maps each data object type to the row widget that renders it, and `UFrontendCommonListView` builds the list. Adding a new setting is a registry entry, not a new widget.

## Screens included

- **Press Any Key** — the initial attract screen that gates entry to the main menu.
- **Main Menu** — the hub, routing to every other screen.
- **Options** — data-driven settings across tabbed collections, including a **Key Remapping** tab built on Enhanced Input user settings.
- **Confirm** — a generic modal dialog with `Ok`, `YesNo`, or `OkCancel` button sets (`EConfirmScreenType`), reporting the result back via `EConfirmScreenButtonType`.
- **Credits** — a scrolling credits screen.
- **Story** — a narrative/intro screen.
- **Loading Screen** — driven by `UFrontendLoadingScreenSubsystem` and the `PreLoadScreen` module, with an optional startup movie.

## Using FrontendUI as a plugin in another project

1. Copy `Plugins/FrontendUI/` into the target project's `Plugins/` directory.
2. Enable it, either through Edit → Plugins or by adding `{ "Name": "FrontendUI", "Enabled": true }` to the `Plugins` array in the target `.uproject`. Make sure `CommonUI` and `EnhancedInput` are enabled too.
3. Regenerate project files and build.

The C++, content, and gameplay tags all travel with the plugin. What does **not** travel is configuration, because both settings classes are declared `Config = Game`, so their values live in the host project's `Config/`. Replicate the following, adjusting for your own screens:

```ini
; Config/DefaultGame.ini
[/Script/FrontendUI.FrontendDeveloperSettings]
FrontendWidgetMap=(((TagName="Frontend.Widget.PressAnyKeyScreen"), "/FrontendUI/UI/Widgets/WBP_CAW_PressAnyKey.WBP_CAW_PressAnyKey_C"),((TagName="Frontend.Widget.MainMenuScreen"), "/FrontendUI/UI/Widgets/WBP_CAW_MainMenu.WBP_CAW_MainMenu_C"),((TagName="Frontend.Widget.ConfirmScreen"), "/FrontendUI/UI/Widgets/WBP_CAW_ConfirmScreen.WBP_CAW_ConfirmScreen_C"),((TagName="Frontend.Widget.StoryScreen"), "/FrontendUI/UI/Widgets/WBP_CAW_StoryScreen.WBP_CAW_StoryScreen_C"),((TagName="Frontend.Widget.OptionsScreen"), "/FrontendUI/UI/Widgets/Options/WBP_CAW_OptionsScreen.WBP_CAW_OptionsScreen_C"),((TagName="Frontend.Widget.KeyRemapScreen"), "/FrontendUI/UI/Widgets/Options/WBP_CAW_KeyRemapScreen.WBP_CAW_KeyRemapScreen_C"),((TagName="Frontend.Widget.CreditScreen"), "/FrontendUI/UI/Widgets/WBP_CAW_CreditScreen.WBP_CAW_CreditScreen_C"))

[/Script/FrontendUI.FrontendLoadingScreenSettings]
SoftLoadingScreenWidgetClass=/FrontendUI/UI/Widgets/WBP_LoadingScreen.WBP_LoadingScreen_C

[/Script/CommonInput.CommonInputSettings]
InputData=/FrontendUI/UI/CommonInputData/InputData_Default.InputData_Default_C

[CommonInputPlatformSettings_Windows CommonInputPlatformSettings]
+ControllerData=/FrontendUI/UI/CommonInputData/ControllerData_MouseKeyboard.ControllerData_MouseKeyboard_C
+ControllerData=/FrontendUI/UI/CommonInputData/ControllerData_Gamepad.ControllerData_Gamepad_C
```

```ini
; Config/DefaultEngine.ini
[/Script/Engine.Engine]
GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient
GameUserSettingsClassName=/Script/FrontendUI.FrontendGameUserSettings
```

`GameViewportClientClassName` is required by CommonUI itself; `GameUserSettingsClassName` points the engine at `UFrontendGameUserSettings` so the options screen has something to read and write.

Finally, the host project supplies a layout and an entry point. Create a widget derived from `UWidget_PrimaryLayout` that registers a container for each `Frontend.WidgetStack.*` tag you intend to use, then have your player controller create it and push the first screen. `BP_FrontendController` and `BP_FrontendGameMode` in `Content/UI/` are the reference implementation.

## Roadmap

Next up is a multiplayer section, gated behind a global enable/disable setting so single-player projects can compile it out of the menu flow entirely, followed by an AWS GameLift integration for dedicated-server hosting and session placement.

## License

Released under the MIT License — see [LICENSE.txt](LICENSE.txt).

## Credits

Created by **Diego Gallo**.

The frontend framework was built by following [Unreal Engine 5 C++ Advanced Frontend UI Programming](https://deloittedevelopment.udemy.com/course/ureal-engine-5-cpp-advanced-frontend-ui-programming/) by **Vince Petrelli** on Udemy. Full credit for the architecture and techniques goes to that course. The conversion of the module into a standalone Unreal plugin is not part of the course material and was done separately for this repository.
