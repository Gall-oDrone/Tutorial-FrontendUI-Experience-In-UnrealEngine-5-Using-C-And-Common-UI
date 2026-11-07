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

RUN chmod +x /home/ue/server/FrontendUIServer.sh \
    && find /home/ue/server/FrontendUI/Binaries/Linux -maxdepth 1 -type f -name "FrontendUIServer*" -exec chmod +x {} +

USER ue

EXPOSE 7777/udp

ENTRYPOINT ["/home/ue/server/FrontendUIServer.sh"]
CMD ["/Game/Maps/L_MatchTest", "-log", "-unattended", "-NullRHI", "-port=7777"]
