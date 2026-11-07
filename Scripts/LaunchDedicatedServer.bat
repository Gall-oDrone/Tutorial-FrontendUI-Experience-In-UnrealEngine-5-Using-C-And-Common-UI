@echo off
REM Diego Gallo All Rights Reserved
REM
REM Runs one dedicated server of this project on this machine so the DEDICATED client mode
REM (LaunchInstances.bat N DEDICATED) has something to connect to. No source-built engine
REM needed: the launcher's editor binary runs headless as a server with -server.
REM See Docs/DedicatedServer.md.
REM
REM Usage:  LaunchDedicatedServer.bat                 (editor binary, port 7777, ServerDefaultMap)
REM         LaunchDedicatedServer.bat 7788            (another port)
REM         LaunchDedicatedServer.bat 7777 PACKAGED   (staged FrontendUIServer.exe, source engine only)
REM
REM The map comes from [/Script/EngineSettings.GameMapsSettings] ServerDefaultMap in
REM DefaultEngine.ini (L_MatchTest). Set MAP=/Game/Maps/Other to override.
REM Log: Saved\Logs\Server.log (editor binary) or the staged build's own Saved\Logs.

setlocal

set "UE_ROOT=D:\EpicGames\UE_5.6"
set "PROJECT=%~dp0..\FrontendUI.uproject"
set "EDITOR_CMD=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if "%PACKAGED_SERVER_EXE%"=="" set "PACKAGED_SERVER_EXE=%~dp0..\Saved\StagedBuilds\WindowsServer\FrontendUIServer.exe"

set PORT=%1
if "%PORT%"=="" set PORT=7777

set BUILD=%2
if "%BUILD%"=="" set BUILD=EDITOR

REM Both processes read the same project settings; the override makes the server-side
REM UFrontendMultiplayerSubsystem answer GetActiveSessionInfo with the FixedAddress title
REM and player cap, which the lobby shows to every client.
set "BACKEND=-ini:Game:[/Script/FrontendMultiplayer.FrontendMultiplayerSettings]:SessionBackendClass=/Script/FrontendMultiplayer.FrontendSessionBackend_FixedAddress"

if /i "%BUILD%"=="PACKAGED" (
	if not exist "%PACKAGED_SERVER_EXE%" (
		echo Packaged server not found at "%PACKAGED_SERVER_EXE%".
		echo Server targets need a source-built engine; see Docs/DedicatedServer.md.
		pause
		exit /b 1
	)
	echo Starting packaged dedicated server on port %PORT%.
	start "FrontendUI dedicated server (PACKAGED, %PORT%)" "%PACKAGED_SERVER_EXE%" %MAP% -log -port=%PORT% %BACKEND%
) else (
	if not exist "%EDITOR_CMD%" (
		echo UnrealEditor-Cmd.exe not found at "%EDITOR_CMD%". Edit UE_ROOT in this script.
		pause
		exit /b 1
	)
	echo Starting dedicated server from the editor binary on port %PORT%.
	start "FrontendUI dedicated server (EDITOR, %PORT%)" "%EDITOR_CMD%" "%PROJECT%" %MAP% -server -log -LOG=Server.log -port=%PORT% %BACKEND%
)

endlocal
