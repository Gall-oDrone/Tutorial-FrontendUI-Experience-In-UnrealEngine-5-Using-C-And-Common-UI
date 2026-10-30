# GameUI — In-game UI (pause menu and HUD)

Authoring spec for the in-match half of the UI. The C++ in `Source/GameUI` owns the flow;
the Blueprints below only provide layout and styling. Conventions follow
`FrontendMultiplayer/Docs/WidgetDesignTemplate.md`.

## What the C++ provides

| Class | Role |
|---|---|
| `AGameUIMatchPlayerController` | On BeginPlay: adds the match mapping context, creates the primary layout, pushes the HUD onto `GameHud` under a loading-screen hold. Binds the pause action. `OpenPauseMenu` / `ClosePauseMenu` / `TogglePauseMenu` are BlueprintCallable. |
| `AGameUIMatchGameMode` | Sets the controller above; an `AGameMode` whose `WaitingToStart` phase is the lobby (see `Lobby.md`). Pawn and rules are yours. |
| `UWidget_HUDBase` | Non-focusable, `Game` input mode. `BP_OnPauseMenuVisibilityChanged(bool)` fires when the pause menu opens/closes; `BP_OnMatchInfoChanged(SessionName, Players, Max)` on arrival and on join/leave. |
| `UWidget_PauseMenu` | Resume / Options / Return to Main Menu / Quit. Back = Resume. Options pushes the framework Options screen onto `GameMenu`. Destructive actions confirm on `Modal`. |
| `UGameUIFlowSubsystem` | `ReturnToFrontend()`; listens to network/travel failures, travels home and stores a message for `ShowPendingDisconnectMessage()`. |
| `UGameUIDeveloperSettings` | **Project Settings → Game → Game UI Settings**: layout class, match mapping context, frontend map, pause action, pause-the-world toggle. |

Pausing the world is the controller's job (`bPauseGameWhileMenuOpen`), triggered when the
pause menu is pushed and cleared when the menu reports `OnCloseRequested`. Pushing Options
on top of the pause menu does **not** unpause.

## Assets to create

### 1. `IA_Pause` and `IMC_Match` — `GameUI/Content/Input`

- `IA_Pause`: Input Action, Value Type **Digital (bool)**. Tick **Trigger when Paused** —
  otherwise the action is swallowed while the world is paused.
- `IMC_Match`: Input Mapping Context. Map `IA_Pause` to **P** and **Gamepad Special Right**
  (Start). Avoid Escape as the only key: in PIE, Escape ends the session.

This action **opens** the menu. It cannot close it: once the menu is up, CommonUI puts the
viewport in Menu input mode and the player controller stops receiving game input. Closing is
CommonUI's job — Back (Escape / B) always works, and to make **the same key** close it:

- In `GameUI/Content/Input` create a Data Table with row structure **CommonInputActionDataBase**,
  name it `DT_GameUIInputActions`. Add a row `PauseAction`: Display Name *Resume*,
  Keyboard Input Type Info → Key **P**, Default Gamepad Input Type Info → Key **Gamepad Special Right**.
  (Don't edit FrontendUI's `DT_CommonInputKeyMapping` — the game owns this row.)
- In `WBP_CAW_PauseMenu` class defaults → *Game UI | Pause Menu* → **Toggle Action** = that row.
  It appears in the bound-action bar as *Resume* with the P / Start glyph.

### 2. `WBP_HUD` — `GameUI/Content/UI/Screens`

Parent class `Widget_HUDBase`. No `BindWidget` properties; put whatever your game needs.

```
[WBP_HUD]
└─ Overlay
   └─ Common Text Block   "HUD"  (Style_Text_Default) — top-left, for the test
```

- Class defaults: **Is Focusable** off (set by C++), **Is Back Handler** off.
- Graph (optional): implement **Event BP On Pause Menu Visibility Changed** → set the
  Overlay's Render Opacity to `0.3` when open, `1.0` when closed, so you can see the hook work.

### 3. `WBP_CAW_PauseMenu` — `GameUI/Content/UI/Screens`

Parent class `Widget_PauseMenu`. Same shell as `WBP_CAW_MainMenu`: no blur wrapper (the
level should stay visible behind a pause menu), `WBP_Template_Layout` root.

```
[WBP_CAW_PauseMenu]
└─ WBP_Template_Layout
   ├─ NamedSlot_BoundActionExtendPoint
   │  └─ Common Bound Action Bar              (Main Menu's settings)
   ├─ NamedSlot_DescriptionExtendPoint
   │  └─ WBP_Text_ButtonDescription
   ├─ NamedSlot_TabButtonsExtendPoint
   │  └─ CommonTextBlock_ScreenTitle           "Paused" (Style_Text_OptionsDetailsView_Title)
   └─ NamedSlot_LeftButtonsExtendPoint
      └─ Vertical Box
         ├─ CommonButton_Resume              [BIND] (WBP_Button_Default)
         ├─ CommonButton_Options             [BIND] (WBP_Button_Default)
         ├─ CommonButton_ReturnToMainMenu    [BIND] (WBP_Button_Default)
         └─ CommonButton_Quit                [BIND] (WBP_Button_Default)
```

- Set each button's **Button Display Text** / **Button Description Text** in Details.
- Back handling, action-bar display, input config (Menu / no capture) and focus (Resume) are
  all set in C++. Nothing to tick in class defaults except the optional **Toggle Action** row.
  No graph work.

| Blueprint | Required widget names |
|---|---|
| `WBP_CAW_PauseMenu` | `CommonButton_Resume`, `CommonButton_Options`, `CommonButton_ReturnToMainMenu`, `CommonButton_Quit` |
| `WBP_HUD` | none |

### 4. `BP_MatchGameMode` — `GameUI/Content/Core`

Parent `GameUIMatchGameMode`. Default Pawn Class: `DefaultPawn` (fly camera) is enough for
the test; the controller class is already set by the parent.

### 5. `L_MatchTest` — `Content/Maps`

New level with lighting (Env. Light Mixer) and a floor. World Settings → GameMode Override =
`BP_MatchGameMode`. Place a Player Start.

## Project Settings

| Page | Setting | Value |
|---|---|---|
| Game → Frontend UI Settings → Frontend Widget Map | `Frontend.Widget.HUD` | `WBP_HUD` |
| | `Frontend.Widget.PauseMenu` | `WBP_CAW_PauseMenu` |
| Game → Game UI Settings → Match | Match Primary Layout Class | `WBP_CUW_PrimaryLayout` |
| | Match Mapping Context | `IMC_Match` |
| Game → Game UI Settings → Maps | Frontend Map | `FrontEndTestMap` |
| Game → Game UI Settings → Pause Menu | Pause Action | `IA_Pause` |
| | Pause Game While Menu Open | ✔ |

## Blueprint wiring in existing assets

- **`WBP_CAW_MainMenu`** — add `Button_Play` (`WBP_Button_Default`) above Story. On click:
  **Open Level (by Object Reference)** → `L_MatchTest`. The loading screen subsystem covers
  the travel on its own.
- **`BP_FrontendController`** — on the push node's **After Push**, after
  `Clear Loading Screen Hold`, add **Game UI Flow Subsystem → Show Pending Disconnect Message**.
  It is a no-op unless a failure sent the player back, in which case it pushes an Ok confirm
  with the reason onto the Modal stack.

## Test checklist (PIE is fine; Standalone for the loading screen)

| Step | Expected |
|---|---|
| Main Menu → Play | loading screen → `L_MatchTest` with `WBP_HUD` visible, mouse captured |
| Press **P** | pause menu, world paused, HUD dimmed, focus on Resume, bound-action bar shows Back |
| Back / Resume / **P** (if Toggle Action is set) | menu closes, world resumes, HUD restored |
| Press **P** again, several times | reopens every time (regression check for the stuck-key bug) |
| Options | framework Options screen on top of the pause menu; Back returns to the pause menu, world still paused |
| Return to Main Menu → Yes | loading screen → `FrontEndTestMap` → Press Any Key (unpaused) |
| Quit → No | back to pause menu |
| Gamepad: Start opens, B closes, D-pad navigates | focus and input routing |
| Simulated failure (console: `disconnect` while in a networked PIE session) | back at the frontend with a *Disconnected* confirm after the first screen appears |
