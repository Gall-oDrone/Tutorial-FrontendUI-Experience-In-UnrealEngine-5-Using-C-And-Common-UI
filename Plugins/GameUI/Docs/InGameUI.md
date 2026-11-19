# GameUI — In-game UI (pause menu and HUD)

Authoring spec for the in-match half of the UI. The C++ in `Source/GameUI` owns the flow;
the Blueprints below only provide layout and styling. Conventions follow
`FrontendMultiplayer/Docs/WidgetDesignTemplate.md`.

## What the C++ provides

| Class | Role |
|---|---|
| `AGameUIMatchPlayerController` | On BeginPlay: adds the match mapping context, creates the primary layout, pushes the HUD onto `GameHud` under a loading-screen hold. Binds the pause action. `OpenPauseMenu` / `ClosePauseMenu` / `TogglePauseMenu` are BlueprintCallable. |
| `AGameUIMatchGameMode` | Sets the controller above; an `AGameMode` whose `WaitingToStart` phase is the lobby (see `Lobby.md`). Pawn and rules are yours. |
| `UWidget_HUDBase` | Non-focusable, `Game` input mode. `BP_OnPauseMenuVisibilityChanged(bool)` fires when the pause menu opens/closes. Match content, each as an **optional** text block and as an event: session name and player count (`BP_OnMatchInfoChanged`, on arrival and on join/leave), match clock (`BP_OnMatchClockChanged`, once a second while the match runs), the local player's ping (`BP_OnPingChanged`). |
| `UWidget_PauseMenu` | Resume / Options / Return to Main Menu / Quit. Back = Resume. Options pushes the framework Options screen onto `GameMenu`. Destructive actions confirm on `Modal`. |
| `UGameUIFlowSubsystem` | `ReturnToFrontend()`; listens to network/travel failures, travels home and stores a message for `ShowPendingDisconnectMessage()`. A match connection that drops becomes a **Reconnect?** question when the session backend can rejoin (see *Reconnect* below); a kick shows the server's reason. |
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

Parent class `Widget_HUDBase`. No required widgets; put whatever your game needs.

```
[WBP_HUD]
└─ Overlay
   ├─ Common Text Block   "HUD"  (Style_Text_Default) — top-left, for the test
   └─ Vertical Box        top-right                    — optional match content
      ├─ CommonText_SessionName   [BIND, optional]  the session's title
      ├─ CommonText_PlayerCount   [BIND, optional]  "2 / 4"
      ├─ CommonText_MatchClock    [BIND, optional]  "1:23", "--:--" in the lobby
      └─ CommonText_Ping          [BIND, optional]  "42 ms"
```

- Class defaults: **Is Focusable** off (set by C++), **Is Back Handler** off.
- The four `CommonText_*` blocks are **Common Text Blocks named exactly so** (tick *Is Variable*);
  the C++ fills in the ones that exist and ignores the rest, so add only what the game wants.
  The same values arrive as events for anything that is not a plain text:

  | Event | When | Values |
  |---|---|---|
  | `BP On Match Info Changed` | arrival, a player joins or leaves | `SessionName`, `CurrentPlayers`, `MaxPlayers` (0 = unknown) |
  | `BP On Match Clock Changed` | once a second while the match runs; once when it starts or stops | `bIsMatchInProgress`, `ElapsedSeconds`, `ClockText` |
  | `BP On Ping Changed` | when the value changes (checked twice a second) | `PingInMilliseconds` |

  The clock is `AGameState::ElapsedTime`: it counts from the moment the match leaves the lobby,
  on the server and on each client, and a late joiner starts from the server's value. The ping
  is the local player's `APlayerState` ping, i.e. what the server measured, so it trails by a
  few seconds and is 0 without a network.
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
| `WBP_HUD` | none (optional: `CommonText_SessionName`, `CommonText_PlayerCount`, `CommonText_MatchClock`, `CommonText_Ping`) |

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
| HUD with the four `CommonText_*` blocks, in a session | title and "n / max" on arrival; clock `--:--` in the lobby, counting from `0:00` when the match starts; ping in ms after a few seconds |

## Reconnect

When the connection to a running match drops, `UGameUIFlowSubsystem` still takes the player
back to the frontend, but the dialog it leaves for `ShowPendingDisconnectMessage()` becomes a
question - *Disconnected: … Reconnect to the match?* - and **Yes** calls
`UFrontendMultiplayerSubsystem::RejoinLastSession()`. The loading screen covers the request; a
refusal (session gone, full) is the usual *Could Not Join* dialog. Nothing to wire in Blueprint:
`BP_FrontendController` already calls `ShowPendingDisconnectMessage`.

| Offered | Not offered |
|---|---|
| `ConnectionLost` / `ConnectionTimeout` on the connection of a client that was in the match, with a backend that can rejoin: GameLift (a new player session in the same game session) and FixedAddress | a join that never got in (refused, timed out on the way); a `FailureReceived` (the server ending things); travel failures; a **kick** - the server's reason is shown instead, under *Removed From Match*; listen-server sessions (the OnlineSubsystem backend does not rejoin) |

On the server nothing is special about a returning player: it is a new login. If it comes while
the game mode still holds the player's inactive state (minutes), the engine hands that state
back and `AGameUIMatchPlayerState::OverrideWith` takes the `PlayerSessionId` and Ready flag from
the new login. The match-completion points are given **once per account and match**
(`no leaderboard points for …: already rewarded in this match`), so dropping and returning does
not multiply them.

**Testing it** (Development / editor builds): in a match, open the console (`~`) on a client and
run `GameUI.SimulateConnectionLost`. The client reports a lost connection the way the engine
would; the server sees that client disconnect. Do it with **two players in the session and from
the one who did not host**: a GameLift server whose last player has left ends the session after
*Session Empty Exit Delay Seconds* (5 s), which is less than the trip to the frontend takes.

| Step | Expected |
|---|---|
| `GameUI.SimulateConnectionLost` in a GameLift match | client log `network failure ConnectionLost - Simulated connection loss … Returning to the frontend with a reconnect offer`; frontend; *Disconnected … Reconnect to the match?* |
| **Yes** | loading screen; `[gamelift] RejoinLastSession -> …`, `CreatePlayerSession -> psess-…` (a new id), travel; back in the match (or the lobby) with the others |
| Server log | the first visit's `RemovePlayerSession` on the disconnect; `player session psess-… verified` for the **new** id; when the player finally leaves: `no leaderboard points for …: already rewarded in this match` |
| **No** | stays on the main menu; the server browser still lists the session |
| Reconnect after the session ended | *Could Not Join* with GameLift's reason |

Verified 2026-11-19 (editor binaries, GameLift Anywhere, two Epic accounts, the non-host
simulating the loss mid-match): `network failure ConnectionLost … Returning to the frontend with
a reconnect offer`; the dialog 3 s later; Yes → `RejoinLastSession`, a new `psess-…` 0.25 s
later, `Welcomed by server` after 1.4 s and the HUD back. Server: the first player session
rewarded (+100, answered in 3 s) and removed at the disconnect; the **new** id accepted,
`verified` and, at the end, the one removed - the returning player's state uses the new login's
slot; `no leaderboard points for …: already rewarded in this match` on the final leave; two
awards in the table for the match, one per account. In the same run the four `CommonText_*`
blocks of `WBP_HUD` showed session name, player count, clock and ping. Not exercised: answering
No, a rejoin that is refused, the kick reason dialog, the FixedAddress backend, a reconnect while
still in the lobby (Ready flag).

### A real outage: the connection the player lost

After a real outage (cable, Wi-Fi, a crashed client) the server does not know the player is
gone: the old connection stays logged in until the server's own connection timeout runs out.
A rejoin that gets in before that used to meet the old entry as a second player with the same
name. Now the game mode removes it: once the session service has confirmed the identity of a
login (`VerifyPlayerSession` answered *matches* and
`UFrontendMultiplayerSubsystem::IsPlayerSessionVerified` is true), every **other** connection of
the same account is kicked - `AGameUIMatchGameMode::RemoveStaleConnectionsOf`, server log
`<name> is back on a new connection; removing the previous one.` The old connection goes through
the normal `Logout`, so its player session is released at GameLift like any leaver's.

- Only on the service's word. A unique id is what the client claims until the check confirms
  it; with a backend that does not verify (FixedAddress, a skipped check) nothing is removed and
  the old entry leaves with the timeout, as before.
- The returning player gets a fresh player state. The engine only hands back the state of a
  player who has **left**, and the old connection had not.
- Unchanged: a **full** session still answers the rejoin with `GameSessionFullException` until
  the timeout frees the slot - the refusal comes from GameLift, before the server sees a login.
- If the old connection is in fact alive (the same account signed in on a second machine), that
  one is told *Removed From Match: This account joined the match again from another connection.*
  The newest login wins.

**Testing it** without pulling a cable: with two players in a GameLift match (session size 3 or
more), start a **third** client signed in as the account of the player who did not host and join
the same session from the browser.

| Step | Expected |
|---|---|
| Third client joins | server log `player session psess-… verified`, then `<name> is back on a new connection; removing the previous one.` |
| The window that had that account | back in the frontend with *Removed From Match: This account joined the match again from another connection.* (no reconnect question - it was a kick) |
| Player list / HUD count | the account once, not twice |

Not run yet (2026-11-19): this path, and a rejoin after a real outage.
