// Diego Gallo All Rights Reserved


#include "Subsystems/GameUIFlowSubsystem.h"
#include "GameUILog.h"
#include "Settings/GameUIDeveloperSettings.h"
#include "Subsystems/FrontendUISubsystem.h"
#include "FrontendMultiplayerSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "GameUIFlow"

UGameUIFlowSubsystem* UGameUIFlowSubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
		return UGameInstance::GetSubsystem<UGameUIFlowSubsystem>(World->GetGameInstance());
	}

	return nullptr;
}

void UGameUIFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
	}
}

void UGameUIFlowSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	Super::Deinitialize();
}

void UGameUIFlowSubsystem::ReturnToFrontend()
{
	const UGameUIDeveloperSettings* Settings = UGameUIDeveloperSettings::Get();

	if (Settings->FrontendMap.IsNull())
	{
		UE_LOG(LogGameUI, Error, TEXT("Game UI Settings -> Maps -> Frontend Map is not set; cannot return to the frontend."));
		return;
	}

	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogGameUI, Error, TEXT("no local player controller to travel with."));
		return;
	}

	// A paused world would carry the pause flag into the loading screen.
	UGameplayStatics::SetGamePaused(PC, false);

	// Tear down the online session (host: destroy, client: leave) so the service lets
	// this player host or join again from the frontend.
	if (UFrontendMultiplayerSubsystem* MultiplayerSubsystem = UGameInstance::GetSubsystem<UFrontendMultiplayerSubsystem>(GetGameInstance()))
	{
		MultiplayerSubsystem->LeaveSession();
	}

	const FString FrontendMapPackage = Settings->FrontendMap.ToSoftObjectPath().GetLongPackageName();
	PC->ClientTravel(FrontendMapPackage, ETravelType::TRAVEL_Absolute);
}

void UGameUIFlowSubsystem::ShowPendingDisconnectMessage()
{
	if (!HasPendingDisconnectMessage())
	{
		return;
	}

	const FText Title = PendingDisconnectTitle;
	const FText Message = PendingDisconnectMessage;
	PendingDisconnectTitle = FText::GetEmpty();
	PendingDisconnectMessage = FText::GetEmpty();

	if (UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this))
	{
		FrontendUISubsystem->PushConfirmScreenToModalStackAsync(
			EConfirmScreenType::Ok,
			Title,
			Message,
			[](EConfirmScreenButtonType) {}
		);
	}
}

void UGameUIFlowSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	UE_LOG(LogGameUI, Warning, TEXT("network failure %s - %s. Returning to the frontend."), ENetworkFailure::ToString(FailureType), *ErrorString);

	const FText Message = ErrorString.IsEmpty()
		? FText::FromString(ENetworkFailure::ToString(FailureType))
		: FText::FromString(ErrorString);

	ReturnToFrontendWithMessage(World, LOCTEXT("DisconnectedTitle", "Disconnected"), Message);
}

void UGameUIFlowSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	UE_LOG(LogGameUI, Warning, TEXT("travel failure %s - %s. Returning to the frontend."), ETravelFailure::ToString(FailureType), *ErrorString);

	const FText Message = ErrorString.IsEmpty()
		? FText::FromString(ETravelFailure::ToString(FailureType))
		: FText::FromString(ErrorString);

	ReturnToFrontendWithMessage(World, LOCTEXT("TravelFailedTitle", "Connection Failed"), Message);
}

void UGameUIFlowSubsystem::ReturnToFrontendWithMessage(UWorld* World, const FText& InTitle, const FText& InMessage)
{
	PendingDisconnectTitle = InTitle;
	PendingDisconnectMessage = InMessage;

	ReturnToFrontend();
}

#undef LOCTEXT_NAMESPACE
