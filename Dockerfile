# syntax=docker/dockerfile:1
# ==============================================================================
# Packaged Unreal Engine 5.6 Linux Dedicated Server Image (FrontendUIServer)
#
# Prerequisite: cook and stage the LinuxServer target from a source-built UE 5.6
# engine (e.g. on the AWS EC2 build machine restored from snap-0777feef308067690):
#
#   RunUAT.bat BuildCookRun ^
#     -project="%CD%\FrontendUI.uproject" ^
#     -noP4 -server -noclient -serverplatform=Linux ^
#     -serverconfig=Development -cook -allmaps -build -stage -pak -archive ^
#     -archivedirectory="%CD%\Saved\StagedBuilds"
#
# Then build & run locally or push to Amazon ECR for GameLift Container Fleets:
#   docker compose up --build
# ==============================================================================

FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG SERVER_STAGE_DIR=Saved/StagedBuilds/LinuxServer

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libatomic1 \
    && rm -rf /var/lib/apt/lists/*

# Unreal's generated wrapper script refuses to run as root.
RUN useradd --create-home --shell /bin/bash --uid 1000 ue

WORKDIR /home/ue/server

COPY --chown=ue:ue ${SERVER_STAGE_DIR}/ /home/ue/server/
COPY --chown=ue:ue docker/entrypoint.sh /home/ue/server/entrypoint.sh

RUN chmod +x /home/ue/server/FrontendUIServer.sh /home/ue/server/entrypoint.sh \
    && find /home/ue/server/FrontendUI/Binaries/Linux -maxdepth 1 -type f -name "FrontendUIServer*" -exec chmod +x {} +

USER ue

EXPOSE 7777/udp

# entrypoint.sh picks the session backend from the environment (GameLift when
# GAMELIFT_SDK_WEBSOCKET_URL is set, FixedAddress otherwise; SERVER_SESSION_BACKEND overrides)
# and execs the engine binary so it is PID 1 and receives SIGTERM from `docker stop` / fleet
# scale-in -> clean engine shutdown -> TerminateServerProcess. Container fleets cannot
# override the command, only the environment, which is why the choice lives in the image.
# Any arguments given to the container are appended to the engine command line.
ENTRYPOINT ["/home/ue/server/entrypoint.sh"]
CMD []
