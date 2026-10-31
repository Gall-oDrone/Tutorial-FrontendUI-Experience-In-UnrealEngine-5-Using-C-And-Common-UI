@echo off
REM Diego Gallo All Rights Reserved
REM
REM Launches N independent game instances of this project (default 2) for testing the
REM session flow: Host in one window, Server Browser + Join in the other. Each instance
REM is its own process with its own online subsystem, which is what the Null LAN beacon
REM needs and what EOS needs to sign in two different users.
REM See Plugins/FrontendMultiplayer/Docs/SessionBackend.md and Docs/EOSSetup.md.
REM
REM Usage:  LaunchInstances.bat                (2 instances, LAN)
REM         LaunchInstances.bat 3              (3 instances, LAN)
REM         LaunchInstances.bat 2 EOS          (2 instances on Epic Online Services)
REM
REM LAN  = Null OnlineSubsystem, exactly what DefaultEngine.ini ships with.
REM EOS  = overrides DefaultPlatformService on the command line and signs each window in
REM        as a different Dev Auth Tool user (Client1, Client2, ...). Start the Dev Auth
REM        Tool on port 6547 first and add credentials named Client1 and Client2.
REM
REM Edit UE_ROOT if your engine lives elsewhere. The project path is derived from this
REM script's location (Scripts\ sits next to FrontendUI.uproject).

setlocal EnableDelayedExpansion

set "UE_ROOT=D:\EpicGames\UE_5.6"
set "PROJECT=%~dp0..\FrontendUI.uproject"
set "EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
set "DEVAUTH_HOST=localhost:6547"

set COUNT=%1
if "%COUNT%"=="" set COUNT=2

set MODE=%2
if "%MODE%"=="" set MODE=LAN

if /i not "%MODE%"=="LAN" if /i not "%MODE%"=="EOS" (
	echo Unknown mode "%MODE%". Use LAN or EOS.
	pause
	exit /b 1
)

if not exist "%EDITOR%" (
	echo UnrealEditor.exe not found at "%EDITOR%". Edit UE_ROOT in this script.
	pause
	exit /b 1
)

echo Launching %COUNT% instance(s) in %MODE% mode.

REM 1280x720 windows tiled left-to-right so both are visible on one screen.
set /a X=0
for /l %%i in (1,1,%COUNT%) do (
	set "EXTRA="
	if /i "%MODE%"=="EOS" (
		set "EXTRA=-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=EOS -AUTH_TYPE=developer -AUTH_LOGIN=%DEVAUTH_HOST% -AUTH_PASSWORD=Client%%i"
	)
	start "FrontendUI instance %%i (%MODE%)" "%EDITOR%" "%PROJECT%" -game -windowed -ResX=1280 -ResY=720 -WinX=!X! -WinY=60 -log -LOG=Instance%%i.log !EXTRA!
	set /a X+=1300
)

endlocal
