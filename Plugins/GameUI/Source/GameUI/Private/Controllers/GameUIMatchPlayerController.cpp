// Diego Gallo All Rights Reserved


#include "Controllers/GameUIMatchPlayerController.h"
#include "GameUIGameplayTags.h"
#include "Settings/GameUIDeveloperSettings.h"
#include "Widgets/Widget_HUDBase.h"
#include "Widgets/Widget_PauseMenu.h"
#include "Widgets/Widget_PrimaryLayout.h"
#include "Subsystems/FrontendUISubsystem.h"
#include "Subsystems/FrontendLoadingScreenSubsystem.h"
#include "FrontendFunctionLibrary.h"
#include "FrontendGameplayTags.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"

namespace
{
	const FName MatchLoadingScreenHoldTag(TEXT("GameUI.Match"));
}

void AGameUIMatchPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// UI exists only for the local player; a server-side proxy of a remote client has no viewport.
	if (!IsLocalPlayerController())
	{
		return;
	}

	AddMatchMappingContext();
	CreatePrimaryLayout();
	PushHUD();
}

void AGameUIMatchPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("GameUI: input component is not UEnhancedInputComponent. Set DefaultInputComponentClass in DefaultInput.ini."));
		return;
	}

	const UGameUIDeveloperSettings* Settings = UGameUIDeveloperSettings::Get();
	if (Settings->PauseAction.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("GameUI: Game UI Settings -> Pause Menu -> Pause Action is not set; the pause menu can only be opened from code or Blueprint."));
		return;
	}

	if (UInputAction* PauseAction = Settings->PauseAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &ThisClass::OnPauseActionTriggered);
	}
}

void AGameUIMatchPlayerController::AddMatchMappingContext()
{
	const UGameUIDeveloperSettings* Settings = UGameUIDeveloperSettings::Get();
	if (Settings->MatchMappingContext.IsNull())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UInputMappingContext* MappingContext = Settings->MatchMappingContext.LoadSynchronous();

	if (InputSubsystem && MappingContext && !InputSubsystem->HasMappingContext(MappingContext))
	{
		InputSubsystem->AddMappingContext(MappingContext, Settings->MatchMappingContextPriority);
	}
}

void AGameUIMatchPlayerController::CreatePrimaryLayout()
{
	const UGameUIDeveloperSettings* Settings = UGameUIDeveloperSettings::Get();
	if (Settings->MatchPrimaryLayoutClass.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("GameUI: Game UI Settings -> Match -> Match Primary Layout Class is not set; no in-match UI will be created."));
		return;
	}

	UClass* LayoutClass = Settings->MatchPrimaryLayoutClass.LoadSynchronous();
	if (!LayoutClass)
	{
		UE_LOG(LogTemp, Error, TEXT("GameUI: Match Primary Layout Class failed to load."));
		return;
	}

	CreatedPrimaryLayout = CreateWidget<UWidget_PrimaryLayout>(this, LayoutClass);
	if (!CreatedPrimaryLayout)
	{
		return;
	}

	CreatedPrimaryLayout->AddToViewport();

	if (UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this))
	{
		FrontendUISubsystem->RegisterCreatedPrimaryLayout(CreatedPrimaryLayout);
	}
}

void AGameUIMatchPlayerController::PushHUD()
{
	if (!CreatedPrimaryLayout)
	{
		return;
	}

	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	UFrontendLoadingScreenSubsystem* LoadingScreenSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFrontendLoadingScreenSubsystem>() : nullptr;
	if (!FrontendUISubsystem)
	{
		return;
	}

	// Same pattern as the frontend: keep the loading screen up until the first widget is on a stack.
	if (LoadingScreenSubsystem)
	{
		LoadingScreenSubsystem->RegisterLoadingScreenHold(MatchLoadingScreenHoldTag, TEXT("Preparing HUD"));
	}

	TWeakObjectPtr<AGameUIMatchPlayerController> WeakThis(this);

	FrontendUISubsystem->PushSoftWidgetToStackAsync(
		FrontendGameplayTags::Frontend_WidgetStack_GameHud,
		UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(GameUIGameplayTags::Frontend_Widget_HUD),
		[WeakThis](EAsyncPushWidgetState InPushState, UWidget_ActivatableBase* PushedWidget)
		{
			if (!WeakThis.IsValid() || !PushedWidget)
			{
				return;
			}

			if (InPushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				PushedWidget->SetOwningPlayer(WeakThis.Get());
				WeakThis->ActiveHUD = Cast<UWidget_HUDBase>(PushedWidget);
			}
			else if (InPushState == EAsyncPushWidgetState::AfterPush)
			{
				if (UFrontendLoadingScreenSubsystem* LoadingScreen = WeakThis->GetGameInstance()->GetSubsystem<UFrontendLoadingScreenSubsystem>())
				{
					LoadingScreen->ClearLoadingScreenHold(MatchLoadingScreenHoldTag);
				}

				// The HUD is not focusable, so nothing else will give the viewport keyboard focus.
				WeakThis->FocusGameViewport();
			}
		}
	);
}

void AGameUIMatchPlayerController::OnPauseActionTriggered()
{
	// Only opens. Closing is CommonUI's Back action on the menu itself, so the same key
	// never races between "open" here and "close" there.
	OpenPauseMenu();
}

void AGameUIMatchPlayerController::OpenPauseMenu()
{
	if (bIsPauseMenuOpen || !IsLocalPlayerController())
	{
		return;
	}

	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	if (!FrontendUISubsystem)
	{
		return;
	}

	bIsPauseMenuOpen = true;
	SetWorldPausedForMenu(true);

	if (ActiveHUD.IsValid())
	{
		ActiveHUD->BP_OnPauseMenuVisibilityChanged(true);
	}

	TWeakObjectPtr<AGameUIMatchPlayerController> WeakThis(this);

	FrontendUISubsystem->PushSoftWidgetToStackAsync(
		FrontendGameplayTags::Frontend_WidgetStack_GameMenu,
		UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(GameUIGameplayTags::Frontend_Widget_PauseMenu),
		[WeakThis](EAsyncPushWidgetState InPushState, UWidget_ActivatableBase* PushedWidget)
		{
			if (!WeakThis.IsValid() || !PushedWidget)
			{
				return;
			}

			if (InPushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				PushedWidget->SetOwningPlayer(WeakThis.Get());

				if (UWidget_PauseMenu* PauseMenu = Cast<UWidget_PauseMenu>(PushedWidget))
				{
					WeakThis->ActivePauseMenu = PauseMenu;
					PauseMenu->OnCloseRequested.AddUObject(WeakThis.Get(), &ThisClass::HandlePauseMenuCloseRequested);
				}
			}
			else if (InPushState == EAsyncPushWidgetState::AfterPush)
			{
				if (UWidget* WidgetToFocus = PushedWidget->GetDesiredFocusTarget())
				{
					WidgetToFocus->SetFocus();
				}
			}
		}
	);
}

void AGameUIMatchPlayerController::ClosePauseMenu()
{
	if (!bIsPauseMenuOpen)
	{
		return;
	}

	if (ActivePauseMenu.IsValid())
	{
		// RequestClose broadcasts OnCloseRequested, which lands in HandlePauseMenuCloseRequested.
		ActivePauseMenu->RequestClose();
	}
	else
	{
		HandlePauseMenuCloseRequested();
	}
}

void AGameUIMatchPlayerController::TogglePauseMenu()
{
	if (bIsPauseMenuOpen)
	{
		ClosePauseMenu();
	}
	else
	{
		OpenPauseMenu();
	}
}

void AGameUIMatchPlayerController::HandlePauseMenuCloseRequested()
{
	bIsPauseMenuOpen = false;
	ActivePauseMenu.Reset();

	SetWorldPausedForMenu(false);

	// Keyboard focus was on a button inside the menu that no longer exists. The HUD is not
	// focusable, so unless we hand focus back to the viewport ourselves, no key reaches the
	// player controller until the player clicks the screen.
	FocusGameViewport();

	// The pause key's release may have arrived while CommonUI had the viewport in Menu mode,
	// leaving PlayerInput thinking it is still held. Forget all pressed keys.
	FlushPressedKeys();

	if (ActiveHUD.IsValid())
	{
		ActiveHUD->BP_OnPauseMenuVisibilityChanged(false);
	}
}

void AGameUIMatchPlayerController::FocusGameViewport()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UGameViewportClient* ViewportClient = LocalPlayer ? LocalPlayer->ViewportClient : nullptr;
	if (!ViewportClient || !FSlateApplication::IsInitialized())
	{
		return;
	}

	TSharedPtr<SViewport> ViewportWidget = ViewportClient->GetGameViewportWidget();
	if (ViewportWidget.IsValid())
	{
		FSlateApplication::Get().SetUserFocus(LocalPlayer->GetControllerId(), ViewportWidget, EFocusCause::SetDirectly);
	}
}

void AGameUIMatchPlayerController::SetWorldPausedForMenu(bool bPaused)
{
	if (!UGameUIDeveloperSettings::Get()->bPauseGameWhileMenuOpen)
	{
		return;
	}

	// No-op for clients in a networked game: only an authoritative controller can pause.
	UGameplayStatics::SetGamePaused(this, bPaused);
}
