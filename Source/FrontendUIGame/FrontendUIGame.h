// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * The project's primary game module (see FrontendUIGame.Build.cs for why it exists).
 *
 * The one thing it does: pin the network version to the project version. Clients and
 * dedicated servers of this project are routinely built with different engine builds
 * (Launcher UE 5.6 editor on a developer PC, source-built UE 5.6.1 on the Linux build box),
 * and the engine's default network version hashes the engine changelist, so those builds
 * refuse each other with "incompatible version of the game" even when the project code is
 * identical. Hashing ProjectVersion instead makes any client/server pair built from the
 * same project revision compatible. Bump ProjectVersion (Project Settings > Description)
 * whenever replicated classes change in a way old clients cannot handle.
 */
class FFrontendUIGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	static uint32 GetProjectNetworkVersion();
};
