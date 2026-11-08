@echo off
REM Diego Gallo All Rights Reserved
REM
REM Runs one dedicated server of this project on this machine so the DEDICATED client mode
REM (LaunchInstances.bat N DEDICATED) has something to connect to. No source-built engine
REM needed: the launcher's editor binary runs headless as a server with -server.
REM See Docs/DedicatedServer.md and Docs/GameLift.md.
REM
REM Usage:  LaunchDedicatedServer.bat                          (editor binary, port 7777, FixedAddress)
REM         LaunchDedicatedServer.bat 7788                     (another port)
REM         LaunchDedicatedServer.bat 7777 PACKAGED            (staged FrontendUIServer.exe, FixedAddress)
REM         LaunchDedicatedServer.bat 7777 GAMELIFT            (editor binary, GameLift Anywhere WebSocket)
REM         LaunchDedicatedServer.bat 7777 GAMELIFT PACKAGED   (staged FrontendUIServer.exe, GameLift Anywhere)
REM
REM GAMELIFT reads GAMELIFT_SDK_WEBSOCKET_URL, GAMELIFT_SDK_FLEET_ID and GAMELIFT_SDK_HOST_ID
REM from the environment and, when GAMELIFT_SDK_AUTH_TOKEN is not already set, asks the AWS
REM CLI for a compute auth token (aws gamelift get-compute-auth-token). AWS_REGION or
REM GAMELIFT_REGION selects the region for that call (default us-east-1).
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

set MODE=DEDICATED
set BUILD=EDITOR
if /i "%2"=="GAMELIFT" set MODE=GAMELIFT
if /i "%3"=="GAMELIFT" set MODE=GAMELIFT
if /i "%2"=="PACKAGED" set BUILD=PACKAGED
if /i "%3"=="PACKAGED" set BUILD=PACKAGED

REM Both processes read the same project settings; the override makes the server-side
REM UFrontendMultiplayerSubsystem answer GetActiveSessionInfo with the FixedAddress or
REM GameLift session title and player cap, which the lobby shows to every client.
set "BACKEND=-ini:Game:[/Script/FrontendMultiplayer.FrontendMultiplayerSettings]:SessionBackendClass=/Script/FrontendMultiplayer.FrontendSessionBackend_FixedAddress"
if /i not "%MODE%"=="GAMELIFT" goto :launch

REM ---- GAMELIFT ---------------------------------------------------------------------
set "BACKEND=-ini:Game:[/Script/FrontendMultiplayer.FrontendMultiplayerSettings]:SessionBackendClass=/Script/FrontendMultiplayer.FrontendSessionBackend_GameLift"

if "%GAMELIFT_SDK_WEBSOCKET_URL%"=="" echo WARNING: GAMELIFT_SDK_WEBSOCKET_URL is not set; the server will run without GameLift.
if "%GAMELIFT_SDK_FLEET_ID%"=="" echo WARNING: GAMELIFT_SDK_FLEET_ID is not set.
if "%GAMELIFT_SDK_HOST_ID%"=="" echo WARNING: GAMELIFT_SDK_HOST_ID is not set.

if not "%GAMELIFT_SDK_AUTH_TOKEN%"=="" goto :gamelift_token_ready
if "%GAMELIFT_SDK_FLEET_ID%"=="" goto :gamelift_token_ready
if "%GAMELIFT_SDK_HOST_ID%"=="" goto :gamelift_token_ready

set "GL_REGION=%GAMELIFT_REGION%"
if "%GL_REGION%"=="" set "GL_REGION=%AWS_REGION%"
if "%GL_REGION%"=="" set "GL_REGION=us-east-1"
set "GL_TOKEN_FILE=%TEMP%\frontendui_gamelift_token.txt"

echo Fetching GameLift compute auth token for %GAMELIFT_SDK_HOST_ID% (%GL_REGION%)...
aws gamelift get-compute-auth-token --fleet-id "%GAMELIFT_SDK_FLEET_ID%" --compute-name "%GAMELIFT_SDK_HOST_ID%" --region "%GL_REGION%" --query AuthToken --output text > "%GL_TOKEN_FILE%"
if errorlevel 1 (
	echo ERROR: aws gamelift get-compute-auth-token failed. Is the AWS CLI installed and configured ^(aws configure^)?
	del "%GL_TOKEN_FILE%" >nul 2>&1
	pause
	exit /b 1
)
set /p GAMELIFT_SDK_AUTH_TOKEN=<"%GL_TOKEN_FILE%"
del "%GL_TOKEN_FILE%" >nul 2>&1
if "%GAMELIFT_SDK_AUTH_TOKEN%"=="" (
	echo ERROR: the AWS CLI returned an empty auth token.
	pause
	exit /b 1
)
echo Auth token acquired.

:gamelift_token_ready
echo GameLift: fleet=%GAMELIFT_SDK_FLEET_ID% compute=%GAMELIFT_SDK_HOST_ID% websocket=%GAMELIFT_SDK_WEBSOCKET_URL%

:launch
if /i "%BUILD%"=="PACKAGED" goto :launch_packaged

if not exist "%EDITOR_CMD%" (
	echo UnrealEditor-Cmd.exe not found at "%EDITOR_CMD%". Edit UE_ROOT in this script.
	pause
	exit /b 1
)
echo Starting dedicated server from the editor binary (%MODE%) on port %PORT%.
start "FrontendUI dedicated server (%MODE%, EDITOR, %PORT%)" "%EDITOR_CMD%" "%PROJECT%" %MAP% -server -log -LOG=Server.log -port=%PORT% %BACKEND%
goto :eof

:launch_packaged
if not exist "%PACKAGED_SERVER_EXE%" (
	echo Packaged server not found at "%PACKAGED_SERVER_EXE%".
	echo Server targets need a source-built engine; see Docs/DedicatedServer.md.
	pause
	exit /b 1
)
echo Starting packaged dedicated server (%MODE%) on port %PORT%.
start "FrontendUI dedicated server (%MODE%, PACKAGED, %PORT%)" "%PACKAGED_SERVER_EXE%" %MAP% -log -port=%PORT% %BACKEND%
goto :eof
