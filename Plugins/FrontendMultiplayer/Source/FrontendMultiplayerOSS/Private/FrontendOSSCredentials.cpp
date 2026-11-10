// Diego Gallo All Rights Reserved

#include "FrontendOSSCredentials.h"
#include "FrontendMultiplayerOSSSettings.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

FOnlineAccountCredentials FrontendOSSCredentials::Build(bool& bOutHasFallback, FOnlineAccountCredentials& OutFallback)
{
	const UFrontendMultiplayerOSSSettings* Settings = UFrontendMultiplayerOSSSettings::Get();
	bOutHasFallback = false;

	// 1. Command line. This is how LaunchInstances.bat gives every window its own
	//    Dev Auth Tool user; the same account cannot be signed in twice on EOS.
	//    Never in Shipping: players must not be able to inject credentials.
#if !UE_BUILD_SHIPPING
	if (Settings->bAllowCommandLineCredentials)
	{
		FString Type;
		if (FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), Type) && !Type.IsEmpty())
		{
			FString Id;
			FString Token;
			FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), Id);
			FParse::Value(FCommandLine::Get(), TEXT("AUTH_PASSWORD="), Token);
			return FOnlineAccountCredentials(Type, Id, Token);
		}
	}
#endif

	const FOnlineAccountCredentials Configured(Settings->CredentialType, Settings->LoginId, Settings->LoginToken);

	// 2. A refresh token cached by an earlier interactive sign-in on this machine, so
	//    the player is not sent to the browser every launch. Falls back to 3 on failure.
	if (Settings->bTryPersistentAuthFirst && !Settings->CredentialType.Equals(TEXT("persistentauth"), ESearchCase::IgnoreCase))
	{
		bOutHasFallback = true;
		OutFallback = Configured;
		return FOnlineAccountCredentials(TEXT("persistentauth"), FString(), FString());
	}

	// 3. Project Settings > Game > Frontend Multiplayer OSS.
	return Configured;
}
