// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FrontendMultiplayerOSSSettings.generated.h"

/**
 * How UFrontendSessionBackend_OSS signs the local player in when the active online
 * subsystem needs an identity (EOS, Steam). The Null subsystem never signs in.
 *
 * Credentials are tried in this order, first hit wins:
 *   1. Command line  -AUTH_TYPE=... -AUTH_LOGIN=... -AUTH_PASSWORD=...  (if allowed)
 *   2. persistentauth                                                  (if enabled)
 *   3. Credential Type / Login Id / Login Token below
 *
 * Nothing here is a secret: product keys live in the git-ignored Config/UserEngine.ini
 * under [/Script/OnlineSubsystemEOS.EOSSettings] (see Docs/EOSSetup.md).
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Frontend Multiplayer OSS"))
class FRONTENDMULTIPLAYEROSS_API UFrontendMultiplayerOSSSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UFrontendMultiplayerOSSSettings* Get() { return GetDefault<UFrontendMultiplayerOSSSettings>(); }

	/**
	 * IOnlineIdentity credential type used when neither the command line nor persistent
	 * auth signed the player in. EOS accepts accountportal (browser sign-in), developer
	 * (Dev Auth Tool), exchangecode (Epic Games Launcher), password.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Login")
	FString CredentialType = TEXT("accountportal");

	/** Credential id for the type above. developer: host:port of the Dev Auth Tool. */
	UPROPERTY(Config, EditAnywhere, Category = "Login")
	FString LoginId;

	/** Credential token for the type above. developer: the credential name in the Dev Auth Tool. */
	UPROPERTY(Config, EditAnywhere, Category = "Login")
	FString LoginToken;

	/**
	 * Try a persistentauth sign-in before the configured credentials, so a player who
	 * already signed in on this machine is not asked again. Falls back silently.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Login")
	bool bTryPersistentAuthFirst = true;

	/**
	 * Let -AUTH_TYPE / -AUTH_LOGIN / -AUTH_PASSWORD on the command line override everything.
	 * This is how Scripts/LaunchInstances.bat gives each window its own Dev Auth Tool user.
	 * Ignored in Shipping builds regardless of this value.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Login")
	bool bAllowCommandLineCredentials = true;

	/**
	 * Give up on a sign-in after this many seconds and release Host / Find with an error.
	 * An interactive sign-in (browser) that the player never finishes would otherwise block
	 * the menu indefinitely. 0 disables the timeout.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Login", meta = (ClampMin = "0", UIMin = "0", UIMax = "600"))
	float LoginTimeoutSeconds = 120.f;

	/**
	 * Value of the FRONTEND_SESSION_BUCKET attribute every online (non-LAN) session
	 * advertises and every search filters on. EOS rejects a lobby search with no
	 * parameters, and this also keeps builds with different values from seeing each
	 * other's sessions (e.g. "FrontendUI-dev" vs "FrontendUI").
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Sessions")
	FString SessionBucket = TEXT("FrontendUI");
};
