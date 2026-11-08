# Dedicated servers

How the frontend talks to a server that is not one of the players, how to test that on
one PC today, and how the Linux server binary and Docker container get built. The
Amazon GameLift workflow (Anywhere, Server SDK 5.x WebSocket protocol, and Managed
Container Fleets) is documented in [Docs/GameLift.md](GameLift.md).

## 1. What changes versus a listen server

| | Listen server (v0.5 - v1.0) | Dedicated server |
|---|---|---|
| Who runs the match | the player who pressed Host | a headless process (docker, EC2, GameLift) |
| Host button | `ReportSessionCreated` → `ServerTravel ...?listen` | `ReportSessionCreatedRemote(address)` → `ClientTravel address`; the "host" is just the first client |
| Join button | `ClientTravel` to the host's address | same |
| Start button | the host presses it | nobody is host: the lobby starts itself when everyone is ready (**Game UI → Lobby → Auto Start When All Ready**, on by default) |
| Session info in the lobby | from the host's online session | from the backend's settings, read on the server and on every client |

The game mode and lobby already handle the "no host player" case
(`AGameUIMatchGameMode::HandleLobbyStateChanged`); nothing in GameUI changes.

## 2. The FixedAddress backend

`UFrontendSessionBackend_FixedAddress` (FrontendMultiplayer module, no online subsystem
required) is the simplest possible dedicated-server backend: one address from settings.

| Request | Answer |
|---|---|
| Host | `ReportSessionCreatedRemote(address)` |
| Find | one row: *Session Display Name*, host column = the address, *Max Players*, map = Host Map |
| Join | `ReportSessionJoined(true, address)` |
| Leave | `ReportSessionLeft` (nothing registered anywhere) |
| Sign-in | not required; the Sign-in panel collapses |

Settings: **Project Settings → Game → Frontend Multiplayer Fixed Address** (`Server Address`,
`Session Display Name`, `Max Players`), or `-ServerAddress=host:port` on the command line.
It is selected like any backend (`Session Backend Class` in Frontend Multiplayer Settings)
or per launch by `LaunchInstances.bat N DEDICATED`, which passes the `-ini:` override.

The **server process** runs the same subsystem and backend; its `GetActiveSessionInfo`
feeds the lobby title and player cap, so both sides read the same ini and agree.

## 3. Test on one PC, launcher engine, no docker

The editor binary runs headless as a dedicated server with `-server`; no Server target
and no source engine needed for this.

```
cd /d D:\EpicGames\Projects\UE_5.6\FrontendUI\Scripts
LaunchDedicatedServer.bat
LaunchInstances.bat 2 DEDICATED
```

| # | Step | Expect |
|---|---|---|
| 1 | Server window opens | `Saved\Logs\Server.log`: `LogNet: ... IpNetDriver listening on port 7777`, `Game class is 'BP_MatchGameMode_C'`, `Match State Changed from EnteringMap to WaitingToStart` |
| 2 | Client A: Multiplayer → Host → anything | debug panel `Session backend: FixedAddress (127.0.0.1:7777)`, `[fixed] CreateSession ... -> joining the dedicated server`, `ClientTravel 127.0.0.1:7777`, `Net driver: IpNetDriver (Client)`; lobby title "Dedicated Server", 1 / 4 |
| 3 | Client B: Server Browser → Refresh → Join | one row "Dedicated Server", `[fixed] JoinSession index 0 -> 127.0.0.1:7777`, lobby 2 / 4 |
| 4 | Both Ready | `start countdown began (3.0s)` in `Server.log` → `Match State Changed from WaitingToStart to InProgress` on all three (Auto Start When All Ready is on by default) |
| 5 | A: Return to Main Menu | B stays in the lobby; server log shows A's logout |
| 6 | Close the server window | both clients: `Host closed the connection.` → connection-lost modal → frontend |

`Server.log` is the project's `Saved\Logs\Server.log` for the editor binary.

## 4. The real server binary

`Source/FrontendUIServer.Target.cs` is the dedicated-server target. Two facts drive
everything below:

- **Launcher engines cannot build Server targets** (project-file generation skips the
  target, `BuildCookRun -server` errors). A source-built engine is required.
- **Nothing about building it needs a GPU or a desktop.** Cooking runs
  `UnrealEditor-Cmd` as a commandlet; compiling is UBT. Both are CPU and RAM.

### Build box (EC2) that does not need DCV

| Item | Choice | Why |
|---|---|---|
| Instance | `c6i.4xlarge` or larger (16+ vCPU, 32+ GB) | compile + cook are CPU-bound; a `g5`/`g6` GPU buys nothing unless you open the editor window there, which this workflow never does |
| OS | Windows Server AMI | the restored engine volume is a Windows source build with the Linux cross-toolchain |
| Volumes | root: fresh AMI; data (`/dev/sdf`, D:): the restored UE 5.6 snapshot | the other repo's compute module already models this (`data_volume_snapshot_id`) |
| Access | SSM Session Manager, RDP at most once | no interactive editor → no DCV, port 8443 stays closed |
| Provisioning | VS 2022 Build Tools (C++ workload), Linux cross-toolchain **v25** for 5.6 (`LINUX_MULTIARCH_ROOT`), git | the DCV section of `user_data.ps1` is simply not run |

### Commands on the build box

```
git clone <this repo> D:\Projects\FrontendUI
cd /d D:\UE_5.6
GenerateProjectFiles.bat -project="D:\Projects\FrontendUI\FrontendUI.uproject" -game -engine
Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -project="D:\Projects\FrontendUI\FrontendUI.uproject" ^
  -platform=Linux -server -serverconfig=Development -noclient -build -cook -stage -pak -nop4 -utf8output
```

Output: `Saved\StagedBuilds\LinuxServer\` with `FrontendUIServer.sh`. That folder is what the
Dockerfile copies. A Windows server for local use is the same command with
`-platform=Win64`, staged to `Saved\StagedBuilds\WindowsServer\`, which
`LaunchDedicatedServer.bat 7777 PACKAGED` runs.

Check the engine version on the restored volume before building; client and server
should be the same 5.6 release:

```
type D:\UE_5.6\Engine\Build\Build.version
```

### Where the editor is still needed

Only on your own PC, for assets: the dedicated path reuses `L_MatchTest`,
`BP_MatchGameMode` and every existing widget. **Auto Start When All Ready** ships on in
`DefaultGame.ini` (it only applies when there is no host player), so there is no
editor-side item at all for this phase.

## 5. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| Client log `Could Not Host: No dedicated server address is configured` | `Server Address` empty and no `-ServerAddress=`; set one |
| Client travels, then `Connection Failed` / timeout | no server on that address/port; check `Server.log` for `listening on port`, firewall for UDP 7777 |
| Server boots into `FrontEndTestMap` | `ServerDefaultMap` missing or the launch passed a map explicitly; it must be `L_MatchTest` |
| Lobby title says "Match" and 0 / 0 | the server did not get the FixedAddress backend: `LaunchDedicatedServer.bat` passes the `-ini:` override, a hand-written command must too (or set `Session Backend Class` permanently) |
| Everyone is Ready, nothing happens | **Auto Start When All Ready** is off; with no host player it is the only way a dedicated match starts |
| `LaunchDedicatedServer.bat 7777 PACKAGED`: "Packaged server not found" | the Server target needs a source engine; use the editor-binary mode on a launcher engine |
