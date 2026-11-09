// Diego Gallo All Rights Reserved

#include "FrontendSessionBackend_GameLiftEOS.h"
#include "FrontendMultiplayerOSSSettings.h"
#include "FrontendOSSCredentials.h"
#include "FrontendMultiplayerSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FrontendMultiplayerOSS"

//~ Begin UFrontendSessionBackend Interface

void UFrontendSessionBackend_GameLiftEOS::Shutdown()
{
	FTSTicker::GetCoreTicker().RemoveTicker(LoginTimeoutHandle);
	LoginTimeoutHandle.Reset();
	if (IOnlineIdentityPtr Identity = GetIdentityInterface())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteHandle);
		Identity->ClearOnLoginStatusChangedDelegate_Handle(0, LoginStatusChangedHandle);
	}
	LoginCompleteHandle.Reset();
	LoginStatusChangedHandle.Reset();
	bLoginInFlight = false;
	DeferredRequest = EDeferred::None;

	Super::Shutdown();
}

void UFrontendSessionBackend_GameLiftEOS::CreateSession(FName SessionName, int32 MaxPlayers)
{
	if (IsLoggedIn())
	{
		Super::CreateSession(SessionName, MaxPlayers);
		return;
	}
	ReportOperationStarted(EFrontendSessionOperation::Creating);
	DeferredSessionName = SessionName;
	DeferredMaxPlayers = MaxPlayers;
	EnsureLoggedInThen(EDeferred::Create);
}

void UFrontendSessionBackend_GameLiftEOS::FindSessions()
{
	if (IsLoggedIn())
	{
		Super::FindSessions();
		return;
	}
	ReportOperationStarted(EFrontendSessionOperation::Finding);
	EnsureLoggedInThen(EDeferred::Find);
}

void UFrontendSessionBackend_GameLiftEOS::JoinSession(int32 SessionIndex)
{
	if (IsLoggedIn())
	{
		Super::JoinSession(SessionIndex);
		return;
	}
	ReportOperationStarted(EFrontendSessionOperation::Joining);
	DeferredJoinIndex = SessionIndex;
	EnsureLoggedInThen(EDeferred::Join);
}

FString UFrontendSessionBackend_GameLiftEOS::GetBackendDisplayName() const
{
	const IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetOwningSubsystem() && GetOwningSubsystem()->GetGameInstance() ? GetOwningSubsystem()->GetGameInstance()->GetWorld() : nullptr);
	return FString::Printf(TEXT("GameLift + %s identity"), OnlineSubsystem ? *OnlineSubsystem->GetSubsystemName().ToString() : TEXT("no OnlineSubsystem"));
}

bool UFrontendSessionBackend_GameLiftEOS::IsLoggedIn() const
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	return Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

void UFrontendSessionBackend_GameLiftEOS::Login()
{
	ReportOperationStarted(EFrontendSessionOperation::LoggingIn);

	if (IsLoggedIn())
	{
		LogDebug(TEXT("[gamelift-eos] Login: already signed in"));
		ReportLoginChanged(true);
		return;
	}

	EnsureLoggedInThen(EDeferred::None);
}

FString UFrontendSessionBackend_GameLiftEOS::GetLocalPlayerDisplayName() const
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid() || Identity->GetLoginStatus(0) != ELoginStatus::LoggedIn)
	{
		return FString();
	}
	return Identity->GetPlayerNickname(0);
}

//~ End UFrontendSessionBackend Interface

FString UFrontendSessionBackend_GameLiftEOS::GetClientAuthToken() const
{
	// The Epic Account Services access token of local user 0. The proxy introspects it with
	// Epic (tokenInfo) and rejects tokens that are expired or issued for another product.
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid() || Identity->GetLoginStatus(0) != ELoginStatus::LoggedIn)
	{
		return FString();
	}
	return Identity->GetAuthToken(0);
}

IOnlineIdentityPtr UFrontendSessionBackend_GameLiftEOS::GetIdentityInterface() const
{
	// Resolved per call: in PIE each instance has its own online subsystem keyed by world.
	const UFrontendMultiplayerSubsystem* Subsystem = GetOwningSubsystem();
	const UGameInstance* GameInstance = Subsystem ? Subsystem->GetGameInstance() : nullptr;
	return Online::GetIdentityInterface(GameInstance ? GameInstance->GetWorld() : nullptr);
}

//~ Begin sign-in (same shape as UFrontendSessionBackend_OSS; kept separate because that
//~ class owns an IOnlineSession state machine this backend does not have)

void UFrontendSessionBackend_GameLiftEOS::EnsureLoggedInThen(EDeferred Request)
{
	if (IsLoggedIn())
	{
		ContinueAfterLogin(Request);
		return;
	}

	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid())
	{
		const FText ErrorText = LOCTEXT("NoIdentity", "No online identity service is active. Launch with DefaultPlatformService=EOS (LaunchInstances.bat ... GAMELIFT with GAMELIFT_AUTH=EOS).");
		ReportLoginChanged(false, ErrorText);
		FailDeferredRequest(Request, ErrorText);
		return;
	}

	DeferredRequest = Request;

	if (bLoginInFlight)
	{
		LogDebug(TEXT("[gamelift-eos] Sign-in already in progress; the new request will run when it completes"));
		return;
	}

	if (!LoginStatusChangedHandle.IsValid())
	{
		LoginStatusChangedHandle = Identity->AddOnLoginStatusChangedDelegate_Handle(0,
			FOnLoginStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleLoginStatusChanged));
	}

	bHasFallbackCredentials = false;
	const FOnlineAccountCredentials FirstCredentials = FrontendOSSCredentials::Build(bHasFallbackCredentials, FallbackCredentials);
	StartLogin(FirstCredentials);
}

void UFrontendSessionBackend_GameLiftEOS::StartLogin(const FOnlineAccountCredentials& Credentials)
{
	IOnlineIdentityPtr Identity = GetIdentityInterface();
	if (!Identity.IsValid())
	{
		bLoginInFlight = true;
		FinishLogin(false, LOCTEXT("LoginNoIdentity", "The online service has no identity interface").ToString());
		return;
	}

	CurrentCredentialType = Credentials.Type;
	bLoginInFlight = true;
	const int32 AttemptSerial = ++LoginAttemptSerial;

	LoginCompleteHandle = Identity->AddOnLoginCompleteDelegate_Handle(0,
		FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleLoginComplete));

	// A browser sign-in the player never finishes would otherwise park Host / Find forever.
	const float TimeoutSeconds = UFrontendMultiplayerOSSSettings::Get()->LoginTimeoutSeconds;
	FTSTicker::GetCoreTicker().RemoveTicker(LoginTimeoutHandle);
	if (TimeoutSeconds > 0.f)
	{
		LoginTimeoutHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, AttemptSerial](float)
		{
			if (bLoginInFlight && LoginAttemptSerial == AttemptSerial)
			{
				FinishLogin(false, LOCTEXT("LoginTimedOut", "Timed out waiting for the sign-in to complete").ToString());
			}
			return false;
		}), TimeoutSeconds);
	}

	LogDebug(FString::Printf(TEXT("[gamelift-eos] Login(%s)%s"), *CurrentCredentialType,
		bHasFallbackCredentials ? *FString::Printf(TEXT(", then %s if that fails"), *FallbackCredentials.Type) : TEXT("")));

	// Some subsystems fire OnLoginComplete synchronously before returning false; only
	// report a rejection that produced no callback for *this* attempt.
	if (!Identity->Login(0, Credentials) && bLoginInFlight && LoginAttemptSerial == AttemptSerial)
	{
		FinishLogin(false, LOCTEXT("LoginRejected", "The login request was rejected").ToString());
	}
}

void UFrontendSessionBackend_GameLiftEOS::HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (LocalUserNum == 0)
	{
		FinishLogin(bWasSuccessful, Error);
	}
}

void UFrontendSessionBackend_GameLiftEOS::FinishLogin(bool bWasSuccessful, const FString& Error)
{
	if (!bLoginInFlight)
	{
		return;
	}
	bLoginInFlight = false;
	FTSTicker::GetCoreTicker().RemoveTicker(LoginTimeoutHandle);
	LoginTimeoutHandle.Reset();

	if (IOnlineIdentityPtr Identity = GetIdentityInterface())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteHandle);
	}
	LoginCompleteHandle.Reset();

	LogDebug(FString::Printf(TEXT("[gamelift-eos] OnLoginComplete (%s) -> %s%s%s"),
		*CurrentCredentialType, bWasSuccessful ? TEXT("OK") : TEXT("FAILED"),
		Error.IsEmpty() ? TEXT("") : TEXT(": "), *Error));

	if (!bWasSuccessful && bHasFallbackCredentials)
	{
		bHasFallbackCredentials = false;
		StartLogin(FallbackCredentials);
		return;
	}

	const EDeferred Request = DeferredRequest;
	DeferredRequest = EDeferred::None;

	if (bWasSuccessful)
	{
		LogDebug(FString::Printf(TEXT("[gamelift-eos] Signed in as '%s'"), *GetLocalPlayerDisplayName()));
		ReportLoginChanged(true);
		ContinueAfterLogin(Request);
		return;
	}

	const FText ErrorText = FText::Format(
		LOCTEXT("LoginFailed", "Could not sign in to Epic Online Services ({0}): {1}"),
		FText::FromString(CurrentCredentialType),
		FText::FromString(Error.IsEmpty() ? FString(TEXT("unknown error")) : Error));

	ReportLoginChanged(false, ErrorText);
	FailDeferredRequest(Request, ErrorText);
}

void UFrontendSessionBackend_GameLiftEOS::ContinueAfterLogin(EDeferred Request)
{
	// The deferred request already reported its operation as started; the GameLift base
	// reports it again, which the subsystem treats as a no-op for the same operation.
	switch (Request)
	{
	case EDeferred::Create:
		Super::CreateSession(DeferredSessionName, DeferredMaxPlayers);
		break;
	case EDeferred::Find:
		Super::FindSessions();
		break;
	case EDeferred::Join:
		Super::JoinSession(DeferredJoinIndex);
		break;
	default:
		// Explicit Login(): ReportLoginChanged has already closed the operation.
		break;
	}
}

void UFrontendSessionBackend_GameLiftEOS::FailDeferredRequest(EDeferred Request, const FText& ErrorText)
{
	switch (Request)
	{
	case EDeferred::Create:
		ReportSessionCreated(false, ErrorText);
		break;
	case EDeferred::Find:
		ReportFindSessionsFailed(ErrorText);
		break;
	case EDeferred::Join:
		ReportSessionJoined(false, FString(), ErrorText);
		break;
	default:
		break;
	}
}

void UFrontendSessionBackend_GameLiftEOS::HandleLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId)
{
	if (LocalUserNum != 0)
	{
		return;
	}
	LogDebug(FString::Printf(TEXT("[gamelift-eos] OnLoginStatusChanged %s -> %s"), ELoginStatus::ToString(OldStatus), ELoginStatus::ToString(NewStatus)));
	if (OldStatus == ELoginStatus::LoggedIn && NewStatus != ELoginStatus::LoggedIn && !bLoginInFlight)
	{
		ReportLoginChanged(false);
	}
}

//~ End sign-in

#undef LOCTEXT_NAMESPACE
