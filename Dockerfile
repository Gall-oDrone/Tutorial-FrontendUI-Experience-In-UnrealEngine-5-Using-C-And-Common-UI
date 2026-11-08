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

# Run the engine binary directly (what FrontendUIServer.sh would exec) so it is PID 1 and
# receives SIGTERM from `docker stop` / fleet scale-in: the engine then shuts down cleanly and
# the GameLift backend sends TerminateServerProcess. Through the .sh wrapper, /bin/sh is PID 1,
# does not forward the signal, and the server is SIGKILLed after the grace period instead.
ENTRYPOINT ["/home/ue/server/FrontendUI/Binaries/Linux/FrontendUIServer", "FrontendUI"]
# -stdout -FullStdOutLogOutput: mirror the engine log to stdout so `docker logs` and the
# fleet's log collection see it (otherwise it only lands in FrontendUI/Saved/Logs inside the container).
CMD ["/Game/Maps/L_MatchTest", "-log", "-stdout", "-FullStdOutLogOutput", "-unattended", "-NullRHI", "-port=7777"]
