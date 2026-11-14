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
REM GameLift runs one game session per process, and the server exits when its session
REM empties (Frontend Multiplayer GameLift -> Exit Process When Session Empty). In GAMELIFT
REM mode this script therefore opens one window that restarts the server every time it
REM exits, fetching a fresh auth token each time, the way a fleet would. Close that window
REM to stop. Set NO_RESTART=1 to run a single instance instead.
REM
REM The map comes from [/Script/EngineSettings.GameMapsSettings] ServerDefaultMap in
REM DefaultEngine.ini (L_MatchTest). Set MAP=/Game/Maps/Other to override.
REM Log: Saved\Logs\Server.log (editor binary) or the staged build's own Saved\Logs.

setlocal

REM Per-developer GameLift values (fleet, compute, websocket, location); see GameLiftEnv.example.bat.
if exist "%~dp0GameLiftEnv.bat" call "%~dp0GameLiftEnv.bat"

set "UE_ROOT=D:\EpicGames\UE_5.6"
set "PROJECT=%~dp0..\FrontendUI.uproject"
set "EDITOR_CMD=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if "%PACKAGED_SERVER_EXE%"=="" set "PACKAGED_SERVER_EXE=%~dp0..\Saved\StagedBuilds\WindowsServer\FrontendUIServer.exe"

set PORT=%1
if "%PORT%"=="" set PORT=7777

set MODE=DEDICATED
set BUILD=EDITOR
set LOOP=
if /i "%2"=="GAMELIFT" set MODE=GAMELIFT
if /i "%3"=="GAMELIFT" set MODE=GAMELIFT
if /i "%2"=="PACKAGED" set BUILD=PACKAGED
if /i "%3"=="PACKAGED" set BUILD=PACKAGED
if /i "%4"=="LOOP" set LOOP=1

REM Both processes read the same project settings; the override makes the server-side
REM UFrontendMultiplayerSubsystem answer GetActiveSessionInfo with the FixedAddress or
REM GameLift session title and player cap, which the lobby shows to every client.
set "BACKEND=-ini:Game:[/Script/FrontendMultiplayer.FrontendMultiplayerSettings]:SessionBackendClass=/Script/FrontendMultiplayer.FrontendSessionBackend_FixedAddress"
REM Accept EOS-signed-in clients on this Null-subsystem server (also in DefaultEngine.ini; the
REM override covers packaged servers staged before that line existed).
set "BACKEND=%BACKEND% -ini:Engine:[/Script/OnlineSubsystemUtils.OnlineEngineInterfaceImpl]:CompatibleUniqueNetIdTypes=EOS"
if /i not "%MODE%"=="GAMELIFT" goto :resolve_exe

REM ---- GAMELIFT ---------------------------------------------------------------------
set "BACKEND=-ini:Game:[/Script/FrontendMultiplayer.FrontendMultiplayerSettings]:SessionBackendClass=/Script/FrontendMultiplayer.FrontendSessionBackend_GameLift -ini:Engine:[/Script/OnlineSubsystemUtils.OnlineEngineInterfaceImpl]:CompatibleUniqueNetIdTypes=EOS"

if "%GAMELIFT_SDK_WEBSOCKET_URL%"=="" echo WARNING: GAMELIFT_SDK_WEBSOCKET_URL is not set; the server will run without GameLift.
if "%GAMELIFT_SDK_FLEET_ID%"=="" echo WARNING: GAMELIFT_SDK_FLEET_ID is not set.
if "%GAMELIFT_SDK_HOST_ID%"=="" echo WARNING: GAMELIFT_SDK_HOST_ID is not set.

REM First invocation: hand off to a dedicated window that runs the restart loop.
if defined LOOP goto :resolve_exe
if "%NO_RESTART%"=="1" goto :resolve_exe
start "FrontendUI dedicated server (GAMELIFT, %BUILD%, %PORT%)" cmd /k ""%~f0" %PORT% GAMELIFT %BUILD% LOOP"
goto :eof

:resolve_exe
if /i "%BUILD%"=="PACKAGED" (
	set "SERVER_EXE=%PACKAGED_SERVER_EXE%"
	set "SERVER_ARGS=%MAP% -log -port=%PORT% %BACKEND%"
) else (
	set "SERVER_EXE=%EDITOR_CMD%"
	set "SERVER_ARGS="%PROJECT%" %MAP% -server -log -LOG=Server.log -port=%PORT% %BACKEND%"
)
if not exist "%SERVER_EXE%" (
	echo Server binary not found at "%SERVER_EXE%".
	if /i "%BUILD%"=="PACKAGED" (echo Server targets need a source-built engine; see Docs/DedicatedServer.md.) else (echo Edit UE_ROOT in this script.)
	pause
	exit /b 1
)

REM ---- single run (DEDICATED, or GAMELIFT with NO_RESTART=1) ------------------------
REM Note: inside ( ) blocks, parentheses in echo text must be escaped as ^( ^) or they end the block.
if not defined LOOP (
	if /i "%MODE%"=="GAMELIFT" call :gamelift_server_key
	if /i "%MODE%"=="GAMELIFT" call :gamelift_token || exit /b 1
	echo Starting dedicated server ^(%MODE%, %BUILD%^) on port %PORT%.
	start "FrontendUI dedicated server (%MODE%, %BUILD%, %PORT%)" "%SERVER_EXE%" %SERVER_ARGS%
	goto :eof
)

REM ---- GAMELIFT restart loop (runs inside the window opened above) -------------------
set RUN=0
call :gamelift_server_key
:loop
set /a RUN+=1
call :gamelift_token || goto :loop_wait
echo.
echo [%TIME%] Run %RUN%: starting dedicated server (GAMELIFT, %BUILD%) on port %PORT%.
"%SERVER_EXE%" %SERVER_ARGS%
echo [%TIME%] Server exited with code %ERRORLEVEL%.
:loop_wait
echo Restarting in 3 seconds (close this window to stop)...
timeout /t 3 /nobreak >nul
goto :loop

REM ---- subroutine: GAMELIFT_SERVER_KEY (leaderboard score submission) from SSM unless the caller set one ----
REM With the key the server adds leaderboard points through the proxy when a player finishes a
REM match (Docs/GameLift.md section 6). Optional: without it the server runs as before and adds
REM nothing. Needs GAMELIFT_ENDPOINT_URL (scores are kept behind the proxy). Never echoed, and
REM passed to the server through the environment only - never on its command line, which is logged.
:gamelift_server_key
if defined GAMELIFT_SERVER_KEY exit /b 0
if "%GAMELIFT_ENDPOINT_URL%"=="" (
	echo Leaderboard: GAMELIFT_ENDPOINT_URL is not set in this window; this server will not add scores.
	echo              Set it before starting ^(or in Scripts\GameLiftEnv.bat^): the server reports scores through the proxy.
	exit /b 0
)
set "GL_KEY_REGION=%GAMELIFT_REGION%"
if "%GL_KEY_REGION%"=="" set "GL_KEY_REGION=%AWS_REGION%"
if "%GL_KEY_REGION%"=="" set "GL_KEY_REGION=us-east-1"
if "%GAMELIFT_SERVER_KEY_PARAM%"=="" set "GAMELIFT_SERVER_KEY_PARAM=/frontendui/leaderboard/server-key"
set "GL_KEY_FILE=%TEMP%\frontendui_gamelift_server_key.txt"
aws ssm get-parameter --name "%GAMELIFT_SERVER_KEY_PARAM%" --with-decryption --region "%GL_KEY_REGION%" --query Parameter.Value --output text > "%GL_KEY_FILE%" 2>nul
if errorlevel 1 (
	del "%GL_KEY_FILE%" >nul 2>&1
	echo Leaderboard: no server key ^(%GAMELIFT_SERVER_KEY_PARAM% is not readable^); this server will not add scores.
	exit /b 0
)
set /p GAMELIFT_SERVER_KEY=<"%GL_KEY_FILE%"
del "%GL_KEY_FILE%" >nul 2>&1
if defined GAMELIFT_SERVER_KEY echo Leaderboard: server key loaded from SSM ^(not shown^); finished matches add points.
exit /b 0

REM ---- subroutine: GAMELIFT_SDK_AUTH_TOKEN from the AWS CLI unless the caller set one ----
:gamelift_token
if "%GAMELIFT_SDK_FLEET_ID%"=="" exit /b 0
if "%GAMELIFT_SDK_HOST_ID%"=="" exit /b 0
if defined GL_TOKEN_FROM_USER exit /b 0
if not "%GAMELIFT_SDK_AUTH_TOKEN%"=="" if not defined GL_TOKEN_FROM_CLI (
	set GL_TOKEN_FROM_USER=1
	exit /b 0
)
set "GL_REGION=%GAMELIFT_REGION%"
if "%GL_REGION%"=="" set "GL_REGION=%AWS_REGION%"
if "%GL_REGION%"=="" set "GL_REGION=us-east-1"
set "GL_TOKEN_FILE=%TEMP%\frontendui_gamelift_token.txt"
echo Fetching GameLift compute auth token for %GAMELIFT_SDK_HOST_ID% (%GL_REGION%)...
aws gamelift get-compute-auth-token --fleet-id "%GAMELIFT_SDK_FLEET_ID%" --compute-name "%GAMELIFT_SDK_HOST_ID%" --region "%GL_REGION%" --query AuthToken --output text > "%GL_TOKEN_FILE%"
if errorlevel 1 (
	echo ERROR: aws gamelift get-compute-auth-token failed. Is the AWS CLI installed and configured ^(aws configure^)?
	del "%GL_TOKEN_FILE%" >nul 2>&1
	exit /b 1
)
set "GAMELIFT_SDK_AUTH_TOKEN="
set /p GAMELIFT_SDK_AUTH_TOKEN=<"%GL_TOKEN_FILE%"
del "%GL_TOKEN_FILE%" >nul 2>&1
if "%GAMELIFT_SDK_AUTH_TOKEN%"=="" (
	echo ERROR: the AWS CLI returned an empty auth token.
	exit /b 1
)
set GL_TOKEN_FROM_CLI=1
echo Auth token acquired. GameLift: fleet=%GAMELIFT_SDK_FLEET_ID% compute=%GAMELIFT_SDK_HOST_ID% websocket=%GAMELIFT_SDK_WEBSOCKET_URL%
exit /b 0
