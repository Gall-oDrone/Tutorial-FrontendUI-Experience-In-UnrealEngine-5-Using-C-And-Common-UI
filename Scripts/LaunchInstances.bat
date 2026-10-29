@echo off
REM Diego Gallo All Rights Reserved
REM
REM Launches N independent game instances of this project (default 2) for testing the
REM Null OnlineSubsystem session flow: Host in one window, Server Browser + Join in the
REM other. Each instance is its own process with its own online subsystem, which is what
REM the Null LAN beacon needs. See Plugins/FrontendMultiplayer/Docs/SessionBackend.md.
REM
REM Usage:  LaunchInstances.bat            (2 instances)
REM         LaunchInstances.bat 3          (3 instances)
REM
REM Edit UE_ROOT if your engine lives elsewhere. The project path is derived from this
REM script's location (Scripts\ sits next to FrontendUI.uproject).

setlocal EnableDelayedExpansion

set "UE_ROOT=D:\EpicGames\UE_5.6"
set "PROJECT=%~dp0..\FrontendUI.uproject"
set "EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"

set COUNT=%1
if "%COUNT%"=="" set COUNT=2

if not exist "%EDITOR%" (
	echo UnrealEditor.exe not found at "%EDITOR%". Edit UE_ROOT in this script.
	pause
	exit /b 1
)

REM 1280x720 windows tiled left-to-right so both are visible on one screen.
set /a X=0
for /l %%i in (1,1,%COUNT%) do (
	start "FrontendUI instance %%i" "%EDITOR%" "%PROJECT%" -game -windowed -ResX=1280 -ResY=720 -WinX=!X! -WinY=60 -log LOG=Instance%%i.log
	set /a X+=1300
)

endlocal
