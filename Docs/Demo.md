# Recording the demo

A repeatable recipe for a 2–3 minute video that shows the whole loop: boot → main menu →
host → lobby → ready → countdown → match → pause → return. Two game instances on one PC,
captured side by side.

## Setup (once)

| Step | Detail |
|---|---|
| Build | Development Editor, latest `master`. The `-game` instances use the same binaries as PIE. |
| Project Settings | Frontend Multiplayer Settings: *Show API Debug Panel* **on** (it makes a good "under the hood" overlay); Game UI Settings → Lobby: countdown **3 s**. |
| Instances | `Scripts/LaunchInstances.bat` — two 1280×720 windows at (0, 60) and (1300, 60). Both fit a 1080p desktop with a few pixels of overlap; for a cleaner frame edit `-ResX=960 -ResY=540` and `X+=970` in the script. |
| OBS | Scene with two **Window Capture** sources (one per instance window — pick by title *FrontendUI (64-bit Development PCD3D_SM6)*, there will be two). Canvas 1920×1080, 60 fps, CQP 18 or CBR 12 Mbps. Disable *Capture Cursor* on the host window if you want a clean lobby shot; keep it on the client so clicks are visible. |
| Frame rate | In each instance's console (`~`): `t.MaxFPS 60`. Development builds otherwise run uncapped and the capture stutters. |
| Firewall | Allow `UnrealEditor.exe` on private networks the first time (Null discovery is a UDP broadcast). |

Optional flags for the launcher, appended after `-log`:

| Flag | Effect |
|---|---|
| `-NoSplash` | No engine splash window at start |
| `-NoVerifyGC` | Fewer hitches in Development |
| `-ExecCmds="t.MaxFPS 60"` | Sets the cap without typing it |

## Shot list

| # | Left (host A) | Right (client B) | What it demonstrates |
|---|---|---|---|
| 1 | Launch → logo → Press Any Key → Main Menu | same | boot flow, loading screen holds |
| 2 | Options → change brightness → Back | — | data-driven options, confirm on exit if dirty |
| 3 | Multiplayer → Host → "Alpha", 4 → Create | — | `[oss] CreateSession` in the debug panel, `ServerTravel ?listen` |
| 4 | Lobby: "Alpha 1 / 4", HOST tag | Multiplayer → Server Browser → row "Alpha 1/4" → Join | LAN discovery, join, replicated player list "2 / 4" |
| 5 | — | Ready | ready tick replicates to A; A's status "Waiting for everyone (1 / 2)" |
| 6 | Ready → Start | — | countdown 3·2·1 on both, lobby closes, pawns spawn, HUD "Alpha 2 / 4" |
| 7 | P → pause menu → Options → Back → Resume | — | in-match menu over a live listen server |
| 8 | — | P → Return to Main Menu → Yes | client leaves cleanly; A's HUD "1 / 4" |
| 9 | P → Return to Main Menu → Yes | — | `DestroySession`, back in the frontend |

Keep the API debug panel open on A during 3–4; it is the only visible proof that a real
session exists.

## Log lines worth overlaying

If you add captions, these are the moments:

```
[oss] OnCreateSessionComplete GameSession -> OK
[oss] OnFindSessionsComplete -> 1 session(s)
[oss] OnJoinSessionComplete GameSession -> Success
LogGameUI: start countdown began (3.0s).
LogGameMode: Match State Changed from WaitingToStart to InProgress
[oss] OnDestroySessionComplete GameSession -> OK
```

Each instance writes `Saved/Logs/Instance1.log` / `Instance2.log`.

## Common capture problems

| Symptom | Fix |
|---|---|
| Second window shows black in OBS | Window Capture → *Capture Method* **Windows 10 (1903 and up)** |
| Mouse cursor disappears in the host after Start | Expected: the HUD captures the mouse (`CapturePermanently`). P brings it back. |
| Server Browser empty | Wait the full 5 s LAN query; check the firewall prompt was accepted |
| Logo plays twice | You pressed Return to Main Menu twice; the flow restarts from the logo by design (`bShowLogoScreen`) |
