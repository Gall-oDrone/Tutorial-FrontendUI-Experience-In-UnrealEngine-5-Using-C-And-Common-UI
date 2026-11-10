// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"

/**
 * The credential chain shared by every backend that signs local user 0 in on an
 * OnlineSubsystem (UFrontendSessionBackend_OSS, UFrontendSessionBackend_GameLiftEOS):
 *
 *   1. -AUTH_TYPE= / -AUTH_LOGIN= / -AUTH_PASSWORD= on the command line (never in Shipping,
 *      and only when UFrontendMultiplayerOSSSettings::bAllowCommandLineCredentials) - how
 *      LaunchInstances.bat gives each window its own Dev Auth Tool user;
 *   2. "persistentauth" (a refresh token cached by an earlier interactive sign-in), falling
 *      back to 3 when it fails;
 *   3. the type / id / token configured in Project Settings > Game > Frontend Multiplayer OSS.
 */
namespace FrontendOSSCredentials
{
	/** Returns the first credentials to try; OutFallback is set when a second attempt should follow a failure. */
	FRONTENDMULTIPLAYEROSS_API FOnlineAccountCredentials Build(bool& bOutHasFallback, FOnlineAccountCredentials& OutFallback);
}
