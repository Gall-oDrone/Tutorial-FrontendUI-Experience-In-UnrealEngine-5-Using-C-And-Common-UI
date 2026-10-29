# GameUI

The per-game UI layer. Everything that names *this* game lives here; the two framework
plugins underneath it stay reusable.

```
FrontendUI            framework: stacks, async push, loading screen, options engine, confirm, styles
FrontendMultiplayer   session + leaderboard screens over a pluggable UFrontendSessionBackend
GameUI  (this)        boot flow, main menu, brand, pause menu, HUD, input actions, backends
```

Dependencies point one way: `GameUI → FrontendMultiplayer → FrontendUI`. FrontendUI must
never reference anything in this plugin.

## What goes here

| Folder | Contents |
|---|---|
| `Content/Core` | `BP_FrontendController`, `BP_FrontendGameMode`, `WBP_CUW_PrimaryLayout` |
| `Content/UI/Screens` | `WBP_CAW_MainMenu`, `WBP_CAW_StoryScreen`, `WBP_CAW_CreditScreen`, future pause menu / HUD |
| `Content/Brand` | `T_StudioLogo`, `WBP_CAW_LogoScreen`, startup movies |
| `Content/Input` | `IMC_Default`, `IA_*` |
| `Source/GameUI/Settings` | `UGameUIOptionsDataRegistry`, `UGameUIGameUserSettings`, `UGameUIDeveloperSettings` |
| `Source/GameUI/Controllers`, `GameModes` | `AGameUIMatchPlayerController`, `AGameUIMatchGameMode` (gameplay maps) |
| `Source/GameUI/Widgets` | `UWidget_HUDBase`, `UWidget_PauseMenu` |
| `Source/GameUI/Subsystems` | `UGameUIFlowSubsystem` (return to frontend, network-failure recovery) |
| `Source/GameUI/Backends` | (later) OnlineSubsystem / GameLift `UFrontendSessionBackend` implementations |
| `Docs/InGameUI.md` | Authoring spec for `WBP_HUD`, `WBP_CAW_PauseMenu`, settings and test checklist |

## Activating the C++ extension points

Both are opt-in so the project behaves identically until you flip them.

- **Options:** Project Settings → Game → Frontend UI Settings → Options Screen →
  *Options Data Registry Class* = `GameUIOptionsDataRegistry`.
- **User settings:** this is an *engine* setting, not a plugin one. Project Settings →
  **Engine → General Settings → Default Classes → Advanced → Game User Settings Class**
  = `GameUIGameUserSettings`. Equivalent in `Config/DefaultEngine.ini`:
  ```ini
  [/Script/Engine.Engine]
  GameUserSettingsClassName=/Script/GameUI.GameUIGameUserSettings
  ```
  Takes effect after an editor restart. Saved `GameUserSettings.ini` values start from
  defaults once after the switch.

## Moving content in (Phase 1 of the plugin split)

Do every move in the **Content Browser**, then right-click the source folder →
**Fix Up Redirectors**, then commit. Never move `.uasset` files with git or Explorer.

1. `UI/Input/IA_*`, `IMC_Default` → `GameUI/Content/Input`
2. `T_StudioLogo`, `WBP_CAW_LogoScreen` → `GameUI/Content/Brand`
   (startup movies **stay** in `Content/Movies` — MoviePlayer only reads that folder)
3. `WBP_CAW_MainMenu`, `WBP_CAW_StoryScreen`, `WBP_CAW_CreditScreen` → `GameUI/Content/UI/Screens`
4. `BP_FrontendController`, `BP_FrontendGameMode`, `WBP_CUW_PrimaryLayout` → `GameUI/Content/Core`
5. Update `Config/DefaultGame.ini` `FrontendWidgetMap` paths (`/FrontendUI/...` → `/GameUI/...`)
   and `DefaultEngine.ini` GameMode / startup-movie references. PIE after every step.
