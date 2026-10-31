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
| `Content/UI/Screens` | `WBP_CAW_MainMenu`, `WBP_CAW_StoryScreen`, `WBP_CAW_CreditScreen`, `WBP_HUD`, `WBP_CAW_PauseMenu`, `WBP_CAW_LobbyScreen`, `WBP_ListEntry_LobbyPlayer` |
| `Content/Brand` | `T_StudioLogo`, `WBP_CAW_LogoScreen`, startup movies |
| `Content/Input` | `IMC_Default`, `IA_*` |
| `Source/GameUI/Settings` | `UGameUIOptionsDataRegistry`, `UGameUIGameUserSettings`, `UGameUIDeveloperSettings` |
| `Source/GameUI/Controllers`, `GameModes` | `AGameUIMatchPlayerController`, `AGameUIMatchGameMode` (AGameMode: WaitingToStart = lobby), `AGameUIMatchGameState`, `AGameUIMatchPlayerState` |
| `Source/GameUI/Widgets` | `UWidget_HUDBase`, `UWidget_PauseMenu`, `UWidget_LobbyScreen`, `UWidget_ListEntry_LobbyPlayer` |
| `Source/GameUI/Subsystems` | `UGameUIFlowSubsystem` (return to frontend, network-failure recovery) |
| `Docs/InGameUI.md` | Authoring spec for `WBP_HUD`, `WBP_CAW_PauseMenu`, settings and test checklist |
| `Docs/Lobby.md` | Authoring spec for the pre-match lobby, settings and test checklist |

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

## Scratch assets

`Content/Developers/` is excluded from cooking (`DirectoriesToNeverCook` in `DefaultGame.ini`).
Put test widgets and throwaway maps there (`WBP_CAW_Test` lives there) so they never ship.

How the content was originally moved into this plugin is recorded in `Docs/History.md`.
