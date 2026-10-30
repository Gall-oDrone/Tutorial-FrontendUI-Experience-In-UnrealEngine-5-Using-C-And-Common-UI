# GameUI — Lobby

Authoring spec for the pre-match lobby. The C++ in `Source/GameUI` owns state and flow;
the two Blueprints below only provide layout and styling. Read `InGameUI.md` first — the
lobby sits on top of the HUD / pause-menu plumbing described there.

## How it works

`AGameUIMatchGameMode` is an `AGameMode` with `bDelayedStart`. The engine's match states
are the lobby state machine:

| Match state | Meaning | UI |
|---|---|---|
| `WaitingToStart` | lobby — nobody has a pawn | `WBP_CAW_LobbyScreen` on `GameMenu`, HUD underneath |
| `InProgress` | match — `StartMatch()` spawned everyone; late joiners spawn on arrival | lobby closed, HUD + pause menu as before |

```
Host clicks Start ──► server: all ready? MinPlayers met? ──► countdown (replicated end time)
                                                          │
                       any player un-readies / leaves ◄───┘ cancel
                                                          │
                                                 timer 0 ─┴─► StartMatch()
```

| Class | Role |
|---|---|
| `AGameUIMatchGameState` | Replicates session name / max players / countdown end time. One delegate (`OnLobbyStateChanged`) for join, leave, ready and host changes; `OnMatchPhaseChanged`; `OnStartCountdownChanged`. `CanStartMatch(Reason)` is evaluated identically on server and clients. |
| `AGameUIMatchPlayerState` | `bIsReady`, `bIsHost` (replicated; set only on the server). |
| `AGameUIMatchGameMode` | Flags the listen host in `PostLogin`, copies session info from `UFrontendMultiplayerSubsystem::GetActiveSessionInfo` into the game state, owns the countdown. |
| `AGameUIMatchPlayerController` | Pushes the lobby while `WaitingToStart`, closes it on `InProgress`. `SetReady / ToggleReady / RequestStartMatch / CancelStartMatch / LeaveMatch` (BlueprintCallable) → server RPCs. Pause action is ignored while the lobby is open. |
| `UWidget_LobbyScreen` | The screen. Back = Leave (confirm). |
| `UWidget_ListEntry_LobbyPlayer` / `UGameUILobbyPlayerEntryData` | One row per player. |

## Settings — Project Settings → Game → Game UI Settings → **Lobby**

| Setting | Default | Notes |
|---|---|---|
| Use Lobby | on | Off restores the v0.5 behaviour: spawn on arrival, no lobby. |
| Min Players To Start | 1 | Host cannot start below this. |
| Start Countdown Seconds | 3 | 0 = start immediately on click. |
| Auto Start When All Ready | off | Only matters on a dedicated server (no host player). |

## Assets to create

Same conventions as `FrontendMultiplayer/Docs/WidgetDesignTemplate.md`: `WBP_Template_Layout`
as the shell, `WBP_Button_Default` for buttons, `SizeBox_ListEntry` + `Style_Text_ListEntry_Default`
for rows, type-prefixed names for every `[BIND]` widget (exact match, or the Blueprint will not
compile). Build the lobby by **duplicating `WBP_CAW_ServerBrowserScreen`** and the row by
**duplicating `WBP_ListEntry_Session`** — then reparent, rename and trim.

### 1. `WBP_ListEntry_LobbyPlayer` — `GameUI/Content/UI/Screens`

Parent class **Widget_ListEntry_LobbyPlayer** (File → Reparent Blueprint after duplicating
`WBP_ListEntry_Session`; delete `CommonButton_Join` and the old graph).

```
[WBP_ListEntry_LobbyPlayer]
└─ SizeBox_ListEntry                                   — same component the other rows use
   └─ Border_Row                 (Is Variable)         — Brush Color white, alpha 0 (tinted for the local player)
      └─ Horizontal Box
         ├─ CommonText_PlayerName      [BIND]          — Fill, V Center, Style_Text_ListEntry_Default
         ├─ CommonText_HostTag         (Is Variable)   — Auto, V Center, text "HOST",  Padding Left 16, Collapsed
         └─ CommonText_ReadyTag        (Is Variable)   — Auto, V Center, text "READY", Padding Left 16, Collapsed
```

- Text tags rather than icons: `Style_Text_ListEntry_Default` already exists and the Din font
  has no check-mark glyph. Swap for `Image` widgets later if you make brushes.
- Root (`SizeBox_ListEntry` → Details → Behavior): **Is Focusable off**. The list view owns
  navigation; the row is display only. There is no Join button and no hover highlight to wire.
- Class defaults: nothing.

**EventGraph** — one event driving **three independent chains**, no variables:

```
Event BP_OnPlayerEntryUpdated (Is Ready, Is Host, Is Local Player)
  │
  ├─exec─► Set Visibility    Target = CommonText_ReadyTag
  │          In Visibility ◄─ Select   Index = Is Ready        False = Collapsed   True = Visible
  │
  ├─exec─► Set Visibility    Target = CommonText_HostTag
  │          In Visibility ◄─ Select   Index = Is Host         False = Collapsed   True = Visible
  │
  └─exec─► Set Brush Color   Target = Border_Row
             In Brush Color ◄─ Select  Index = Is Local Player  False = (1,1,1,0)   True = (1,1,1,0.08)
```

Chain the execs in sequence (event → Set Visibility → Set Visibility → Set Brush Color).

How to wire a `Select` so it comes out typed correctly: **drag backwards from the input pin it
should feed** (e.g. `In Visibility`), type *Select*; the node is created with `ESlateVisibility`
options. Then plug the event's bool into **Index** — the options relabel to **False / True**. A
Select created from empty space is a Wildcard, and plugging bools into its *Options* turns it
into the wrong thing (a bool-returning Select with three options).

Common mistakes: `Set Visibility` with Target = *self* hides the whole row; the targets are the
tag widgets. Delete any leftover `Tick` / `In Delta Time` node from the duplicated row.

Rows are rebuilt (new item objects) on every change, so this fires again each time; no
state to keep.

### 2. `WBP_CAW_LobbyScreen` — `GameUI/Content/UI/Screens`

Parent class **Widget_LobbyScreen**. Duplicate `WBP_CAW_ServerBrowserScreen`, reparent,
remove the `Border → Overlay → Background Blur` wrapper (the pause-menu rule applies: the
level stays visible behind a lobby), delete `DetailsView_SessionInfo`.

```
[WBP_CAW_LobbyScreen]
└─ WBP_Template_Layout
   ├─ NamedSlot_BoundActionExtendPoint
   │  └─ Common Bound Action Bar                       — Main Menu's settings; shows Back = Leave
   ├─ NamedSlot_DescriptionExtendPoint
   │  └─ WBP_Text_ButtonDescription
   ├─ NamedSlot_TabButtonsExtendPoint
   │  └─ Horizontal Box
   │     ├─ CommonText_SessionName     [BIND]          — Fill,  Style_Text_OptionsDetailsView_Title
   │     └─ CommonText_PlayerCount     [BIND]          — Auto, V Bottom, Style_Text_Default, Padding Left 24
   ├─ NamedSlot_MainLeftExtendPoint
   │  └─ Vertical Box
   │     ├─ Common Text Block                           "Players" (Style_Text_Default)
   │     └─ CommonListView_Players    [BIND]           — Fill
   └─ NamedSlot_MainRightExtendPoint
      └─ Vertical Box
         ├─ CommonButton_Ready        [BIND] (WBP_Button_Default)
         ├─ CommonButton_Start        [BIND] (WBP_Button_Default)      — top padding 8
         ├─ CommonButton_Leave        [BIND] (WBP_Button_Default)      — top padding 24
         └─ CommonText_Status         (Is Variable)                    — Style_Text_Default, Auto Wrap, top padding 24
```

Details to set:

| Widget | Setting |
|---|---|
| `CommonListView_Players` | **Entry Widget Class** `WBP_ListEntry_LobbyPlayer` · **Selection Mode** None · **Num Designer Preview Entries** 4 · plain `Common List View`, *not* `FrontendCommonListView` (same reason as the server browser: that one casts rows to `ListDataObject_Base`) |
| `CommonButton_Ready` | Button Display Text "Ready" · Description "Toggle your ready state" |
| `CommonButton_Start` | Display Text "Start Match" · Description "Start the match once everyone is ready" · leave **Visible**; C++ collapses it for non-hosts |
| `CommonButton_Leave` | Display Text "Leave" · Description "Return to the main menu" |
| `CommonText_Status` | empty text; **Is Variable** on |
| `CommonText_SessionName`, `CommonText_PlayerCount` | placeholder text only; C++ overwrites |

Class defaults: nothing. Back handling (`bIsBackHandler`), the action-bar entry, the
Menu/no-capture input config, and focus (`CommonButton_Ready`) are all set in C++.

**EventGraph** — four events. Each one is a straight line; no Tick, no timers, no casts.

```
Event BP_OnReadyStateChanged (bIsLocalPlayerReady)
 └─ CommonButton_Ready → Set Button Text  (Select: bIsLocalPlayerReady ? "Not Ready" : "Ready")

Event BP_OnStartAvailabilityChanged (bCanStart, Reason)
 └─ Branch (Is Counting Down?)   ← promote a bool variable "bCountingDown", see below
     ├─ True  → (do nothing; the countdown text owns the status line)
     └─ False → CommonText_Status → Set Text  (Select: bCanStart ? "Everyone is ready" : Reason)

Event BP_OnCountdownChanged (SecondsRemaining, bIsCountingDown)
 ├─ Set bCountingDown = bIsCountingDown
 ├─ CommonButton_Start → Set Button Text (Select: bIsCountingDown ? "Cancel" : "Start Match")
 └─ Branch (bIsCountingDown)
     ├─ True  → CommonText_Status → Set Text (Format Text "Starting in {Seconds}..." ← SecondsRemaining)
     └─ False → CommonText_Status → Set Text ("")   ← C++ re-sends the availability text right after

Event BP_OnHostStateChanged (bIsLocalPlayerHost)
 └─ (optional) WBP_Text_ButtonDescription or a small text: "You are the host" / ""
```

`Set Button Text` is the setter `WBP_Button_Default` exposes for its display text (the same
one Main Menu's buttons use); if your button Blueprint names it differently, use that.

### 3. Widget Map — Project Settings → Game → Frontend UI Settings

`Frontend.Widget.LobbyScreen` → `WBP_CAW_LobbyScreen` is **already in `DefaultGame.ini`** at
`/GameUI/UI/Screens/WBP_CAW_LobbyScreen`. Create the asset at exactly that path and nothing
else is needed; if you place it elsewhere, repoint the entry.

### 4. `WBP_HUD`

Add a `Common Text Block` (`CommonText_MatchInfo`, Is Variable) in a top-right corner of the
HUD's overlay, `Style_Text_Default`, and implement:

```
Event BP_OnMatchInfoChanged (SessionName, CurrentPlayers, MaxPlayers)
 └─ CommonText_MatchInfo → Set Text (Format Text "{Name}  {Cur} / {Max}")
```

Fired when the HUD lands and on every join/leave. `MaxPlayers` is 0 when the session had no
capacity (stub); show just `{Cur}` then if you care.

### 5. `BP_MatchGameMode`

No change needed: the C++ parent switched from `GameModeBase` to `GameMode` underneath it and
sets Game State / Player State classes. Open it once and confirm *Classes* shows
`GameUIMatchGameState` / `GameUIMatchPlayerState` (not overridden) and a **Default Pawn Class**
is set, so Start visibly spawns something.

| Blueprint | Required widget names |
|---|---|
| `WBP_ListEntry_LobbyPlayer` | `CommonText_PlayerName` |
| `WBP_CAW_LobbyScreen` | `CommonText_SessionName`, `CommonText_PlayerCount`, `CommonListView_Players`, `CommonButton_Ready`, `CommonButton_Start`, `CommonButton_Leave` |
| `WBP_HUD` | none |

## Test checklist (two instances via `Scripts/LaunchInstances.bat`)

| # | Step | Expect |
|---|---|---|
| 1 | A hosts "Alpha" (4) | lobby: "Alpha", "1 / 4", A's row with host mark; Start disabled with reason; mouse visible |
| 2 | B joins | both lists: A (host), B; "2 / 4" |
| 3 | B → Ready | B's tick on both screens; A's Start still disabled ("Waiting for everyone to be ready (1 / 2)") |
| 4 | A → Ready | A's Start enables |
| 5 | A → Start | "Starting in 3…2…1" on both; lobby closes on both; pawns spawn; HUD shows "Alpha 2 / 4"; mouse captured |
| 6 | redo 1–4, B un-readies during countdown | countdown cancels on both; Start disabled again |
| 7 | B → Leave → Yes | B in frontend; A's list "1 / 4"; A's Ready preserved |
| 8 | match running, C joins (`LaunchInstances.bat 3`) | C skips the lobby: pawn + HUD immediately |
| 9 | A → Leave during lobby | B returns to the frontend with the connection-lost message |
| 10 | P in lobby | nothing; after Start, P opens the pause menu |
| 11 | Backend = Stub, Host | lobby alone → Ready → Start → HUD |
| 12 | Use Lobby off | hosting lands straight in the HUD |

## Notes

- Player names come from `APlayerState::GetPlayerName()`. The Null subsystem generates
  machine-based names; EOS/Steam provide real display names with no code change here.
- The host leaving ends the session for everyone (no host migration). The Leave confirm
  says so to the host.
- The lobby never pauses the world, so the countdown and replication keep running even
  when *Pause Game While Menu Open* is on.
