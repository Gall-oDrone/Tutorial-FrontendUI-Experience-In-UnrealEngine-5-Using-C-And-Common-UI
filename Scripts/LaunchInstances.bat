@echo off
REM Diego Gallo All Rights Reserved
REM
REM Launches N independent game instances of this project (default 2) for testing the
REM session flow: Host in one window, Server Browser + Join in the other. Each instance
REM is its own process with its own online subsystem, which is what the Null LAN beacon
REM needs and what EOS needs to sign in two different users.
REM See Plugins/FrontendMultiplayer/Docs/SessionBackend.md and Docs/EOSSetup.md.
REM
REM Usage:  LaunchInstances.bat                      (2 instances, LAN, editor binary)
REM         LaunchInstances.bat 3                    (3 instances, LAN)
REM         LaunchInstances.bat 2 EOS                (2 instances on Epic Online Services)
REM         LaunchInstances.bat 2 LAN PACKAGED       (2 instances of the packaged exe)
REM         LaunchInstances.bat 2 EOS PACKAGED
REM
REM LAN  = Null OnlineSubsystem, exactly what DefaultEngine.ini ships with.
REM EOS  = overrides DefaultPlatformService on the command line and signs each window in
REM        as a different Dev Auth Tool user (User1, User2, ...). Start the Dev Auth
REM        Tool on port 6547 first and add credentials named User1 and User2.
REM
REM PACKAGED runs the staged Development build instead of UnrealEditor.exe -game. Package
REM first (File > Package Project > Windows, or RunUAT BuildCookRun) and point PACKAGED_EXE
REM at the result if it is not in the default Saved\StagedBuilds location. Logs still go to
REM Saved\Logs\InstanceN.log next to the exe's own Saved folder.
REM
REM Edit UE_ROOT if your engine lives elsewhere. The project path is derived from this
REM script's location (Scripts\ sits next to FrontendUI.uproject).

setlocal EnableDelayedExpansion

set "UE_ROOT=D:\EpicGames\UE_5.6"
set "PROJECT=%~dp0..\FrontendUI.uproject"
set "EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
set "DEVAUTH_HOST=localhost:6547"
if "%PACKAGED_EXE%"=="" set "PACKAGED_EXE=%~dp0..\Saved\StagedBuilds\Windows\FrontendUI.exe"

set COUNT=%1
if "%COUNT%"=="" set COUNT=2

set MODE=%2
if "%MODE%"=="" set MODE=LAN

set BUILD=%3
if "%BUILD%"=="" set BUILD=EDITOR

if /i not "%MODE%"=="LAN" if /i not "%MODE%"=="EOS" (
	echo Unknown mode "%MODE%". Use LAN or EOS.
	pause
	exit /b 1
)

if /i not "%BUILD%"=="EDITOR" if /i not "%BUILD%"=="PACKAGED" (
	echo Unknown build "%BUILD%". Use PACKAGED or leave empty for the editor binary.
	pause
	exit /b 1
)

if /i "%BUILD%"=="PACKAGED" (
	if not exist "%PACKAGED_EXE%" (
		echo Packaged exe not found at "%PACKAGED_EXE%".
		echo Package the project first, or set PACKAGED_EXE to the staged FrontendUI.exe.
		pause
		exit /b 1
	)
	set "LAUNCH="%PACKAGED_EXE%""
) else (
	if not exist "%EDITOR%" (
		echo UnrealEditor.exe not found at "%EDITOR%". Edit UE_ROOT in this script.
		pause
		exit /b 1
	)
	set "LAUNCH="%EDITOR%" "%PROJECT%" -game"
)

echo Launching %COUNT% instance(s) in %MODE% mode (%BUILD%).

REM 1280x720 windows tiled left-to-right so both are visible on one screen.
set /a X=0
for /l %%i in (1,1,%COUNT%) do (
	set "EXTRA="
	if /i "%MODE%"=="EOS" (
		set "EXTRA=-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=EOS -AUTH_TYPE=developer -AUTH_LOGIN=%DEVAUTH_HOST% -AUTH_PASSWORD=User%%i"
	)
	start "FrontendUI instance %%i (%MODE%, %BUILD%)" !LAUNCH! -windowed -ResX=1280 -ResY=720 -WinX=!X! -WinY=60 -log -LOG=Instance%%i.log !EXTRA!
	set /a X+=1300
)

endlocal
