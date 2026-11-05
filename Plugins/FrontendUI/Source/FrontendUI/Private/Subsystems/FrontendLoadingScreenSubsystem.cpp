// Diego Gallo All Rights Reserved


#include "Subsystems/FrontendLoadingScreenSubsystem.h"
#include "PreLoadScreenManager.h"
#include "FrontendSettings/FrontendLoadingScreenSettings.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/FrontendLoadingScreenInterface.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FrontendUI"

bool UFrontendLoadingScreenSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!CastChecked<UGameInstance>(Outer)->IsDedicatedServerInstance())
	{
		TArray<UClass*> FoundClasses;
		GetDerivedClasses(GetClass(), FoundClasses);

		return FoundClasses.IsEmpty();
	}

	return false;
}

void UFrontendLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &ThisClass::OnMapPreloaded);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnMapPostLoaded);
}

void UFrontendLoadingScreenSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	// PIE stop / exit while the screen is up: give the viewport back.
	TryRemoveLoadingScreen();
	if (UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
	{
		ViewportClient->bDisableWorldRendering = false;
	}
	ActiveLoadingScreenHolds.Empty();

	Super::Deinitialize();
}

UWorld* UFrontendLoadingScreenSubsystem::GetTickableGameObjectWorld() const
{
	if (UGameInstance* OwningGameInstance = GetGameInstance())
	{
		return OwningGameInstance->GetWorld();
	}

	return nullptr;
}

void UFrontendLoadingScreenSubsystem::Tick(float DeltaTime)
{
	TryUpdateLoadingScreen();
}

ETickableTickType UFrontendLoadingScreenSubsystem::GetTickableTickType() const
{
	if (IsTemplate())
	{
		return ETickableTickType::Never;
	}
	return ETickableTickType::Conditional;
}

bool UFrontendLoadingScreenSubsystem::IsTickable() const
{
	return GetGameInstance() && GetGameInstance()->GetGameViewportClient();
}

TStatId UFrontendLoadingScreenSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFrontendLoadingScreenSubsystem,STATGROUP_Tickables);
}

void UFrontendLoadingScreenSubsystem::OnMapPreloaded(const FWorldContext& WorldContext, const FString& MapName)
{
	if (WorldContext.OwningGameInstance != GetGameInstance())
	{
		return;
	}

	SetTickableTickType(ETickableTickType::Conditional);

	bIsCurrentlyLoadingMap = true;

	TryUpdateLoadingScreen();
}

void UFrontendLoadingScreenSubsystem::OnMapPostLoaded(UWorld* LoadedWorld)
{
	if (LoadedWorld && LoadedWorld->GetGameInstance() == GetGameInstance())
	{
		bIsCurrentlyLoadingMap = false;
	}
}

void UFrontendLoadingScreenSubsystem::RegisterLoadingScreenHold(FName InHoldTag, const FString& InReason)
{
	if (InHoldTag.IsNone())
	{
		return;
	}

	ActiveLoadingScreenHolds.Add(InHoldTag, InReason);

	// A hold placed after the screen was dismissed must bring it back, so make sure we tick.
	SetTickableTickType(ETickableTickType::Conditional);

	TryUpdateLoadingScreen();
}

void UFrontendLoadingScreenSubsystem::ClearLoadingScreenHold(FName InHoldTag)
{
	ActiveLoadingScreenHolds.Remove(InHoldTag);
}

void UFrontendLoadingScreenSubsystem::TryUpdateLoadingScreen()
{
	// Holds and map events can arrive before the viewport exists; the tick picks it up.
	UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
	if (!ViewportClient)
	{
		return;
	}

	//Check if there's any start up loading screen that's currently active
	if (IsPreLoadScreenActive())
	{
		return;
	}

	//Check if we should show the loading screen
	if (ShouldShowLoadingScreen())
	{
		//Try display the loading screen here
		TryDisplayLoadingScreenIfNone();

		OnLoadingReasonUpdated.Broadcast(CurrentLoadingReason);
	}
	else
	{
		//Try remove the current active loading screen
		TryRemoveLoadingScreen();

		HoldLoadingScreenStartUpTime = -1.0;

		//Notify the loading is complete
		NotifyLoadingScreenVisibilityChange(false);

		//Disable the ticking
		SetTickableTickType(ETickableTickType::Never);
	}
}

bool UFrontendLoadingScreenSubsystem::IsPreLoadScreenActive() const
{
	if (FPreLoadScreenManager* PreLoadScreenManager = FPreLoadScreenManager::Get())
	{
		return PreLoadScreenManager->HasValidActivePreLoadScreen();
	}

	return false;
}

bool UFrontendLoadingScreenSubsystem::ShouldShowLoadingScreen()
{
	const UFrontendLoadingScreenSettings* LoadingScreenSettings = GetDefault<UFrontendLoadingScreenSettings>();

	if (GIsEditor && !LoadingScreenSettings->bShouldShowLoadingScreenInEditor)
	{
		return false;
	}

	UGameViewportClient* ViewportClient = GetGameInstance()->GetGameViewportClient();

	//Check if the objects in the world need a loading screen
	if (CheckTheNeedToShowLoadingScreen())
	{
		ViewportClient->bDisableWorldRendering = true;

		return true;
	}

	CurrentLoadingReason = LOCTEXT("LoadingReasonTextureStreaming", "Waiting for texture streaming").ToString();

	//There's no need to show the loading screen. Allow the world to be renderer to our viewport here
	ViewportClient->bDisableWorldRendering = false;

	const double CurrentTime = FPlatformTime::Seconds();

	if (HoldLoadingScreenStartUpTime < 0.0)
	{
		HoldLoadingScreenStartUpTime = CurrentTime;
	}

	const double ElapsedTime = CurrentTime - HoldLoadingScreenStartUpTime;

	// Work the engine cannot observe (frontend building its first screen, session join,
	// save streaming) keeps the screen up through explicit holds. Checked after world
	// rendering is enabled so texture streaming keeps progressing underneath.
	if (!ActiveLoadingScreenHolds.IsEmpty())
	{
		for (const TPair<FName, FString>& ActiveHold : ActiveLoadingScreenHolds)
		{
			CurrentLoadingReason = ActiveHold.Value;
			break;
		}

		return true;
	}

	if (ElapsedTime < LoadingScreenSettings->HoldLoadingScreenExtraSeconds)
	{
		return true;
	}

	return false;
}

bool UFrontendLoadingScreenSubsystem::CheckTheNeedToShowLoadingScreen()
{
	if (bIsCurrentlyLoadingMap)
	{
		CurrentLoadingReason = LOCTEXT("LoadingReasonLevel", "Loading level").ToString();

		return true;
	}

	UWorld* OwningWorld = GetGameInstance()->GetWorld();

	if (!OwningWorld)
	{
		CurrentLoadingReason = LOCTEXT("LoadingReasonWorld", "Initializing world").ToString();

		return true;
	}

	if (!OwningWorld->HasBegunPlay())
	{
		CurrentLoadingReason = LOCTEXT("LoadingReasonBeginPlay", "Starting world").ToString();

		return true;
	}

	if (!OwningWorld->GetFirstLocalPlayerFromController())
	{
		CurrentLoadingReason = LOCTEXT("LoadingReasonPlayer", "Waiting for player").ToString();

		return true;
	}

	return false;
}

void UFrontendLoadingScreenSubsystem::TryDisplayLoadingScreenIfNone()
{
	//If there's already active loading screen, return early if yes

	if (CachedCreatedLoadingScreenWidget)
	{
		return;
	}

	const UFrontendLoadingScreenSettings* LoadingScreenSettings = GetDefault<UFrontendLoadingScreenSettings>();

	TSubclassOf<UUserWidget> LoadedWidgetClass = LoadingScreenSettings->GetLoadingScreenWidgetClassChecked();

	UUserWidget* CreatedWidget = UUserWidget::CreateWidgetInstance(*GetGameInstance(), LoadedWidgetClass, NAME_None);

	check(CreatedWidget);

	CachedCreatedLoadingScreenWidget = CreatedWidget->TakeWidget();

	GetGameInstance()->GetGameViewportClient()->AddViewportWidgetContent(
		CachedCreatedLoadingScreenWidget.ToSharedRef(),
		1000
	);

	NotifyLoadingScreenVisibilityChange(true);
}

void UFrontendLoadingScreenSubsystem::TryRemoveLoadingScreen()
{
	if (!CachedCreatedLoadingScreenWidget)
	{
		return;
	}

	if (UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
	{
		ViewportClient->RemoveViewportWidgetContent(CachedCreatedLoadingScreenWidget.ToSharedRef());
	}

	CachedCreatedLoadingScreenWidget.Reset();
}

void UFrontendLoadingScreenSubsystem::NotifyLoadingScreenVisibilityChange(bool bIsVisible)
{
	for (ULocalPlayer* ExistingLocalPlayer : GetGameInstance()->GetLocalPlayers())
	{
		if (!ExistingLocalPlayer)
		{
			continue;
		}

		if (APlayerController* PC = ExistingLocalPlayer->GetPlayerController(GetGameInstance()->GetWorld()))
		{
			//Query if the player controller implements the interface. Call the function through the interface to notify the loading status if yes.
			if (PC->Implements<UFrontendLoadingScreenInterface>())
			{
				if (bIsVisible)
				{
					IFrontendLoadingScreenInterface::Execute_OnLoadingScreenActivated(PC);
				}
				else
				{
					IFrontendLoadingScreenInterface::Execute_OnLoadingScreenDeactivated(PC);
				}
			}

			if (APawn* OwningPawn = PC->GetPawn())
			{
				if (OwningPawn->Implements<UFrontendLoadingScreenInterface>())
				{
					if (bIsVisible)
					{
						IFrontendLoadingScreenInterface::Execute_OnLoadingScreenActivated(OwningPawn);
					}
					else
					{
						IFrontendLoadingScreenInterface::Execute_OnLoadingScreenDeactivated(OwningPawn);
					}
				}
			}
		}

		//The code for notifying other objects in the world goes here
	}
}

#undef LOCTEXT_NAMESPACE
