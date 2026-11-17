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
#   GAMELIFT_ENDPOINT_URL       the proxy; with GAMELIFT_SERVER_KEY_PARAM the server submits leaderboard scores
#   GAMELIFT_SERVER_KEY_PARAM   SSM SecureString holding the servers' key, read here at start (see below)
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

# ---- Leaderboard server key (optional) ----------------------------------------------------
# A server that reports finished matches must present a key to the proxy (Docs/GameLift.md
# section 6). The key is in neither the image nor the fleet's container definition, where
# anybody who can read the GameLift configuration would see it: it is read here, at start,
# from SSM Parameter Store with the credentials the container already has - the fleet role on
# a container fleet (AWS_CONTAINER_CREDENTIALS_*), or AWS_ACCESS_KEY_ID / AWS_SECRET_ACCESS_KEY
# when the image is run by hand - and reaches the engine through its environment only.
# Whatever goes wrong, the server starts anyway, without score submission, and says why.
# Nothing here prints the key or the credentials.
json_field() { # json_field <name>   stdin: a JSON object -> the (string) value of that field
    sed -n "s/.*\"$1\"[[:space:]]*:[[:space:]]*\"\([^\"]*\)\".*/\1/p" | head -n 1
}

load_server_key() {
    [ -z "${GAMELIFT_SERVER_KEY:-}" ] || return 0
    [ -n "${GAMELIFT_SERVER_KEY_PARAM:-}" ] || return 0
    if [ -z "${GAMELIFT_ENDPOINT_URL:-}" ]; then
        echo "[entrypoint] leaderboard: GAMELIFT_ENDPOINT_URL is not set; scores will not be submitted"
        return 0
    fi
    if ! command -v curl >/dev/null 2>&1; then
        echo "[entrypoint] leaderboard: no curl in this image; scores will not be submitted"
        return 0
    fi

    key_region="${GAMELIFT_SERVER_KEY_REGION:-${AWS_REGION:-${AWS_DEFAULT_REGION:-}}}"
    if [ -z "$key_region" ]; then
        # wss://us-east-1.api.amazongamelift.com -> us-east-1
        key_region="$(printf '%s' "${GAMELIFT_SDK_WEBSOCKET_URL:-}" | sed -n 's#^wss://\([a-z0-9-]*\)\.api\.amazongamelift\.com.*#\1#p')"
    fi
    [ -n "$key_region" ] || key_region="us-east-1"

    aws_id="${AWS_ACCESS_KEY_ID:-}"; aws_secret="${AWS_SECRET_ACCESS_KEY:-}"; aws_token="${AWS_SESSION_TOKEN:-}"
    if [ -z "$aws_id" ]; then
        creds_url=""
        if [ -n "${AWS_CONTAINER_CREDENTIALS_FULL_URI:-}" ]; then
            creds_url="$AWS_CONTAINER_CREDENTIALS_FULL_URI"
        elif [ -n "${AWS_CONTAINER_CREDENTIALS_RELATIVE_URI:-}" ]; then
            creds_url="http://169.254.170.2${AWS_CONTAINER_CREDENTIALS_RELATIVE_URI}"
        fi
        if [ -n "$creds_url" ]; then
            creds="$(curl -sS --max-time 5 "$creds_url" 2>/dev/null || true)"
            aws_id="$(printf '%s' "$creds" | json_field AccessKeyId)"
            aws_secret="$(printf '%s' "$creds" | json_field SecretAccessKey)"
            aws_token="$(printf '%s' "$creds" | json_field Token)"
            creds=""
        fi
    fi
    if [ -z "$aws_id" ] || [ -z "$aws_secret" ]; then
        echo "[entrypoint] leaderboard: no AWS credentials in this container; scores will not be submitted"
        return 0
    fi

    # The credentials reach curl as a config on stdin, not on its command line.
    response="$( {
            printf 'user = "%s:%s"\n' "$aws_id" "$aws_secret"
            [ -z "$aws_token" ] || printf 'header = "x-amz-security-token: %s"\n' "$aws_token"
        } | curl -sS --max-time 10 --config - \
            --aws-sigv4 "aws:amz:${key_region}:ssm" \
            -H "Content-Type: application/x-amz-json-1.1" \
            -H "X-Amz-Target: AmazonSSM.GetParameter" \
            -d "{\"Name\":\"${GAMELIFT_SERVER_KEY_PARAM}\",\"WithDecryption\":true}" \
            "https://ssm.${key_region}.amazonaws.com/" 2>/dev/null || true )"
    aws_id=""; aws_secret=""; aws_token=""

    server_key="$(printf '%s' "$response" | json_field Value)"
    if [ -n "$server_key" ]; then
        export GAMELIFT_SERVER_KEY="$server_key"
        echo "[entrypoint] leaderboard: server key loaded from $GAMELIFT_SERVER_KEY_PARAM (not shown)"
    else
        why="$(printf '%s' "$response" | json_field __type)"
        echo "[entrypoint] leaderboard: could not read $GAMELIFT_SERVER_KEY_PARAM in $key_region (${why:-no answer}); scores will not be submitted"
    fi
    server_key=""; response=""
}
load_server_key

echo "[entrypoint] map=$MAP port=$PORT backend=$BACKEND"
# shellcheck disable=SC2086  # SERVER_EXTRA_ARGS is intentionally word-split
exec "$BIN" FrontendUI "$MAP" \
    -log -stdout -FullStdOutLogOutput -unattended -NullRHI "-port=$PORT" \
    "-ini:Game:[/Script/FrontendMultiplayer.FrontendMultiplayerSettings]:SessionBackendClass=$BACKEND" \
    "-ini:Game:[/Script/GameUI.GameUIDeveloperSettings]:bAutoStartWhenAllReady=True" \
    "-ini:Engine:[/Script/OnlineSubsystemUtils.OnlineEngineInterfaceImpl]:CompatibleUniqueNetIdTypes=EOS" \
    ${SERVER_EXTRA_ARGS:-} "$@"
