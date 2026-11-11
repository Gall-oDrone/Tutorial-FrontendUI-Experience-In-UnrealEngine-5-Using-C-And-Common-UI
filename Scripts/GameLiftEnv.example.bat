@echo off
REM Diego Gallo All Rights Reserved
REM
REM GameLift environment for LaunchDedicatedServer.bat / LaunchInstances.bat (GAMELIFT mode).
REM
REM   copy GameLiftEnv.example.bat GameLiftEnv.bat     (GameLiftEnv.bat is git-ignored)
REM
REM then fill in the values from your Anywhere fleet (aws gamelift create-fleet /
REM register-compute; see Docs/GameLift.md section 3). Both scripts `call` GameLiftEnv.bat
REM when it exists, so a fresh console window needs nothing else. Values already present in
REM the environment win over this file. AWS credentials are NOT set here: the scripts take
REM them from the AWS CLI profile (aws configure).

if not defined AWS_REGION                 set "AWS_REGION=us-east-1"
if not defined GAMELIFT_SDK_WEBSOCKET_URL set "GAMELIFT_SDK_WEBSOCKET_URL=wss://us-east-1.api.amazongamelift.com"
if not defined GAMELIFT_SDK_FLEET_ID      set "GAMELIFT_SDK_FLEET_ID=fleet-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
if not defined GAMELIFT_SDK_HOST_ID       set "GAMELIFT_SDK_HOST_ID=DevPC"
if not defined GAMELIFT_LOCATION          set "GAMELIFT_LOCATION=custom-location-1"

REM Optional: clients target an alias instead of the fleet id (the server still needs the fleet
REM id above for GetComputeAuthToken). Swap fleets with `aws gamelift update-alias`; no client change.
REM if not defined GAMELIFT_ALIAS_ID      set "GAMELIFT_ALIAS_ID=alias-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"

REM Optional: route the client's GameLift calls through the API Gateway + Lambda proxy
REM (Docs/GameLift.md section 6) instead of signing them with your own AWS keys. This is
REM what Shipping builds use; setting it here lets you exercise the same path in Development.
REM if not defined GAMELIFT_ENDPOINT_URL  set "GAMELIFT_ENDPOINT_URL=https://xxxxxxxxxx.execute-api.us-east-1.amazonaws.com/"
REM Optional: sign each client window in with EOS (Dev Auth Tool, like EOS mode) and use the
REM GameLiftEOS backend so the proxy can authenticate players (Docs/GameLift.md section 6).
REM if not defined GAMELIFT_AUTH          set "GAMELIFT_AUTH=EOS"

REM Optional: Host through a GameLift game session queue (StartGameSessionPlacement) instead of
REM CreateGameSession on the fleet/alias - GameLift waits for capacity (auto-scaling, several
REM fleets) and reserves the host slot in the same call. Find/Join keep using the alias/fleet.
REM if not defined GAMELIFT_QUEUE_NAME    set "GAMELIFT_QUEUE_NAME=frontendui-dev"
