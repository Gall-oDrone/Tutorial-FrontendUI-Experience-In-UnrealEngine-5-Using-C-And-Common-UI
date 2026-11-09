#!/bin/sh
# Container entrypoint for the FrontendUI Linux dedicated server.
#
# Container fleets cannot override the container command (only its environment), so the
# image decides how to run from environment variables:
#
#   GAMELIFT_SDK_WEBSOCKET_URL  set by GameLift (container/Anywhere) -> GameLift session backend
#   SERVER_SESSION_BACKEND      explicit backend class path, overrides the rule above
#   SERVER_MAP                  default /Game/Maps/L_MatchTest
#   SERVER_PORT                 default 7777 (must match the container port in the fleet definition)
#   SERVER_EXTRA_ARGS           appended verbatim; any arguments to the container are appended too
#
# `exec` keeps the engine as PID 1 so SIGTERM from the orchestrator reaches it and the
# GameLift backend can send TerminateServerProcess on the way out.
set -eu

ROOT="$(cd "$(dirname "$0")" && pwd)"
BIN="$ROOT/FrontendUI/Binaries/Linux/FrontendUIServer"
MAP="${SERVER_MAP:-/Game/Maps/L_MatchTest}"
PORT="${SERVER_PORT:-7777}"

if [ -n "${SERVER_SESSION_BACKEND:-}" ]; then
    BACKEND="$SERVER_SESSION_BACKEND"
elif [ -n "${GAMELIFT_SDK_WEBSOCKET_URL:-}" ]; then
    BACKEND="/Script/FrontendMultiplayer.FrontendSessionBackend_GameLift"
else
    BACKEND="/Script/FrontendMultiplayer.FrontendSessionBackend_FixedAddress"
fi

echo "[entrypoint] map=$MAP port=$PORT backend=$BACKEND"
# shellcheck disable=SC2086  # SERVER_EXTRA_ARGS is intentionally word-split
exec "$BIN" FrontendUI "$MAP" \
    -log -stdout -FullStdOutLogOutput -unattended -NullRHI "-port=$PORT" \
    "-ini:Game:[/Script/FrontendMultiplayer.FrontendMultiplayerSettings]:SessionBackendClass=$BACKEND" \
    "-ini:Game:[/Script/GameUI.GameUIDeveloperSettings]:bAutoStartWhenAllReady=True" \
    ${SERVER_EXTRA_ARGS:-} "$@"
