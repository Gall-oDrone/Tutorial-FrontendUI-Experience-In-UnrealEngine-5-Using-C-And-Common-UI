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

### 1. `WBP_ListEntry_LobbyPlayer` — `GameUI/Content/UI/Screens`

Parent class **Widget_ListEntry_LobbyPlayer**.

| Widget | Name | Notes |
|---|---|---|
| Common Text Block | `CommonText_PlayerName` | required (BindWidget) |
| Image / Text | *ready tick* | optional; toggled in `BP_OnPlayerEntryUpdated` |
| Text | *"Host"* tag | optional; same event |

Implement **Event BP_OnPlayerEntryUpdated (bIsReady, bIsHost, bIsLocalPlayer)**: show/hide the
tick and host tag, tint the row when `bIsLocalPlayer`. Set **Is Focusable = false** on the
root; the list, not the row, handles navigation.

### 2. `WBP_CAW_LobbyScreen` — `GameUI/Content/UI/Screens`

Parent class **Widget_LobbyScreen**. Same frame/style as `WBP_CAW_HostSessionScreen`.

| Widget | Name | Notes |
|---|---|---|
| Common Text Block | `CommonText_SessionName` | header; receives the Host-screen name |
| Common Text Block | `CommonText_PlayerCount` | "2 / 4" |
| Common List View | `CommonListView_Players` | **Entry Widget Class** = `WBP_ListEntry_LobbyPlayer`; Selection Mode **None** |
| Frontend button | `CommonButton_Ready` | text set from `BP_OnReadyStateChanged` |
| Frontend button | `CommonButton_Start` | host only; C++ collapses it for clients and enables/disables it |
| Frontend button | `CommonButton_Leave` | |
| Text (optional) | countdown / status | driven by `BP_OnCountdownChanged` and `BP_OnStartAvailabilityChanged` |

Events to implement:

| Event | Do |
|---|---|
| `BP_OnReadyStateChanged(bIsLocalPlayerReady)` | Ready button text: *Ready* ↔ *Not Ready* (or *Cancel Ready*) |
| `BP_OnStartAvailabilityChanged(bCanStart, Reason)` | show `Reason` in the status text when `bCanStart` is false; clear it otherwise |
| `BP_OnCountdownChanged(SecondsRemaining, bIsActive)` | status text "Starting in 3…"; relabel Start to *Cancel* while active |
| `BP_OnHostStateChanged(bIsLocalPlayerHost)` | optional chrome ("You are the host") |

Class defaults: nothing required — Back handling and the Menu input config are set in C++.

### 3. Widget Map — Project Settings → Game → Frontend UI Settings

Add `Frontend.Widget.LobbyScreen` → `WBP_CAW_LobbyScreen`.

### 4. `WBP_HUD`

Implement **Event BP_OnMatchInfoChanged (SessionName, CurrentPlayers, MaxPlayers)** — a corner
text like `Alpha  2 / 4` is enough. Fired when the HUD lands and on every join/leave.

### 5. `BP_MatchGameMode`

No change needed: the C++ parent switched from `GameModeBase` to `GameMode` underneath it and
sets Game State / Player State classes. Open it once and confirm *Classes* shows
`GameUIMatchGameState` / `GameUIMatchPlayerState` (not overridden) and a **Default Pawn Class**
is set, so Start visibly spawns something.

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
