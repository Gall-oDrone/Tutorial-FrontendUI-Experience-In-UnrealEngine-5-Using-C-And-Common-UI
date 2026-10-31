// Diego Gallo All Rights Reserved


#include "Controllers/GameUIMatchPlayerController.h"
#include "GameUILog.h"
#include "GameUIGameplayTags.h"
#include "Settings/GameUIDeveloperSettings.h"
#include "GameModes/GameUIMatchGameMode.h"
#include "GameModes/GameUIMatchGameState.h"
#include "GameModes/GameUIMatchPlayerState.h"
#include "Subsystems/GameUIFlowSubsystem.h"
#include "Widgets/Widget_HUDBase.h"
#include "Widgets/Widget_PauseMenu.h"
#include "Widgets/Widget_LobbyScreen.h"
#include "Widgets/Widget_PrimaryLayout.h"
#include "Subsystems/FrontendUISubsystem.h"
#include "Subsystems/FrontendLoadingScreenSubsystem.h"
#include "FrontendFunctionLibrary.h"
#include "FrontendGameplayTags.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"

#define LOCTEXT_NAMESPACE "GameUILobby"

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

	// The game state decides whether we are in the lobby or the match. On a client it can
	// replicate after this controller begins play, so bind either now or when it arrives.
	UWorld* World = GetWorld();
	if (AGameStateBase* GameState = World ? World->GetGameState() : nullptr)
	{
		BindToGameState(GameState);
	}
	else if (World)
	{
		GameStateSetHandle = World->GameStateSetEvent.AddUObject(this, &ThisClass::BindToGameState);
	}
}

void AGameUIMatchPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GameStateSetEvent.Remove(GameStateSetHandle);
	}

	if (AGameUIMatchGameState* GameState = BoundGameState.Get())
	{
		GameState->OnMatchPhaseChanged.RemoveAll(this);
		GameState->OnLobbyStateChanged.RemoveAll(this);
	}
	BoundGameState.Reset();

	Super::EndPlay(EndPlayReason);
}

void AGameUIMatchPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogGameUI, Error, TEXT("input component is not UEnhancedInputComponent. Set DefaultInputComponentClass in DefaultInput.ini."));
		return;
	}

	const UGameUIDeveloperSettings* Settings = UGameUIDeveloperSettings::Get();
	if (Settings->PauseAction.IsNull())
	{
		UE_LOG(LogGameUI, Warning, TEXT("Game UI Settings -> Pause Menu -> Pause Action is not set; the pause menu can only be opened from code or Blueprint."));
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
		UE_LOG(LogGameUI, Error, TEXT("Game UI Settings -> Match -> Match Primary Layout Class is not set; no in-match UI will be created."));
		return;
	}

	UClass* LayoutClass = Settings->MatchPrimaryLayoutClass.LoadSynchronous();
	if (!LayoutClass)
	{
		UE_LOG(LogGameUI, Error, TEXT("Match Primary Layout Class failed to load."));
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
				WeakThis->PushMatchInfoToHUD();
			}
		}
	);
}

void AGameUIMatchPlayerController::OnPauseActionTriggered()
{
	UE_LOG(LogGameUI, Verbose, TEXT("pause action triggered (menu open=%d, world paused=%d)"), bIsPauseMenuOpen ? 1 : 0, IsPaused() ? 1 : 0);

	// Only opens. Closing is CommonUI's Back action on the menu itself, so the same key
	// never races between "open" here and "close" there.
	OpenPauseMenu();
}

void AGameUIMatchPlayerController::OpenPauseMenu()
{
	if (bIsPauseMenuOpen || bIsLobbyOpen || !IsLocalPlayerController())
	{
		UE_LOG(LogGameUI, Verbose, TEXT("OpenPauseMenu ignored (already open=%d, lobby open=%d, local=%d)"), bIsPauseMenuOpen ? 1 : 0, bIsLobbyOpen ? 1 : 0, IsLocalPlayerController() ? 1 : 0);
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
				UE_LOG(LogGameUI, Verbose, TEXT("pause menu widget created/reused: %s"), *PushedWidget->GetName());
				PushedWidget->SetOwningPlayer(WeakThis.Get());

				if (UWidget_PauseMenu* PauseMenu = Cast<UWidget_PauseMenu>(PushedWidget))
				{
					WeakThis->ActivePauseMenu = PauseMenu;
					PauseMenu->OnCloseRequested.RemoveAll(WeakThis.Get());
					PauseMenu->OnCloseRequested.AddUObject(WeakThis.Get(), &ThisClass::HandlePauseMenuCloseRequested);
				}
				else
				{
					UE_LOG(LogGameUI, Error, TEXT("widget mapped to Frontend.Widget.PauseMenu is not a Widget_PauseMenu; close events will not reach the controller."));
				}
			}
			else if (InPushState == EAsyncPushWidgetState::AfterPush)
			{
				UE_LOG(LogGameUI, Verbose, TEXT("pause menu pushed (activated=%d)"), PushedWidget->IsActivated() ? 1 : 0);

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
	UE_LOG(LogGameUI, Verbose, TEXT("pause menu close requested"));

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

//~ Begin lobby

void AGameUIMatchPlayerController::BindToGameState(AGameStateBase* GameState)
{
	AGameUIMatchGameState* MatchGameState = Cast<AGameUIMatchGameState>(GameState);
	if (!MatchGameState || BoundGameState == MatchGameState)
	{
		if (GameState && !MatchGameState)
		{
			UE_LOG(LogGameUI, Warning, TEXT("game state %s is not an AGameUIMatchGameState; no lobby will be shown."), *GameState->GetName());
		}
		return;
	}

	BoundGameState = MatchGameState;
	MatchGameState->OnMatchPhaseChanged.AddUObject(this, &ThisClass::HandleMatchPhaseChanged);
	MatchGameState->OnLobbyStateChanged.AddUObject(this, &ThisClass::HandleLobbyStateChanged);

	HandleMatchPhaseChanged();
	HandleLobbyStateChanged();
}

void AGameUIMatchPlayerController::HandleMatchPhaseChanged()
{
	const AGameUIMatchGameState* MatchGameState = BoundGameState.Get();
	if (!MatchGameState)
	{
		return;
	}

	UE_LOG(LogGameUI, Verbose, TEXT("match phase -> %s"), *MatchGameState->GetMatchState().ToString());

	if (MatchGameState->IsInLobby())
	{
		OpenLobby();
	}
	else if (MatchGameState->GetMatchState() != MatchState::EnteringMap)
	{
		CloseLobby();
	}
}

void AGameUIMatchPlayerController::HandleLobbyStateChanged()
{
	PushMatchInfoToHUD();
}

void AGameUIMatchPlayerController::OpenLobby()
{
	if (bIsLobbyOpen || !IsLocalPlayerController())
	{
		return;
	}

	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	if (!FrontendUISubsystem)
	{
		return;
	}

	// The lobby replaces the pause menu as the GameMenu-stack screen for this phase.
	if (bIsPauseMenuOpen)
	{
		ClosePauseMenu();
	}

	bIsLobbyOpen = true;

	TWeakObjectPtr<AGameUIMatchPlayerController> WeakThis(this);

	FrontendUISubsystem->PushSoftWidgetToStackAsync(
		FrontendGameplayTags::Frontend_WidgetStack_GameMenu,
		UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(GameUIGameplayTags::Frontend_Widget_LobbyScreen),
		[WeakThis](EAsyncPushWidgetState InPushState, UWidget_ActivatableBase* PushedWidget)
		{
			if (!WeakThis.IsValid() || !PushedWidget)
			{
				return;
			}

			if (InPushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				PushedWidget->SetOwningPlayer(WeakThis.Get());
				WeakThis->ActiveLobby = Cast<UWidget_LobbyScreen>(PushedWidget);
				if (!WeakThis->ActiveLobby.IsValid())
				{
					UE_LOG(LogGameUI, Error, TEXT("widget mapped to Frontend.Widget.LobbyScreen is not a Widget_LobbyScreen."));
				}
			}
			else if (InPushState == EAsyncPushWidgetState::AfterPush)
			{
				// The match may have started while the widget was streaming in.
				if (!WeakThis->bIsLobbyOpen)
				{
					PushedWidget->DeactivateWidget();
					return;
				}

				if (UWidget* WidgetToFocus = PushedWidget->GetDesiredFocusTarget())
				{
					WidgetToFocus->SetFocus();
				}
			}
		}
	);
}

void AGameUIMatchPlayerController::CloseLobby()
{
	if (!bIsLobbyOpen)
	{
		return;
	}

	bIsLobbyOpen = false;

	if (UWidget_LobbyScreen* Lobby = ActiveLobby.Get())
	{
		// Popping the GameMenu stack re-applies the HUD's Game input config synchronously.
		Lobby->DeactivateWidget();
	}
	ActiveLobby.Reset();

	// Same hygiene as closing the pause menu: give the viewport focus back and forget any
	// key the player was holding while CommonUI was in Menu mode.
	FocusGameViewport();
	FlushPressedKeys();
}

void AGameUIMatchPlayerController::PushMatchInfoToHUD()
{
	UWidget_HUDBase* HUD = ActiveHUD.Get();
	const AGameUIMatchGameState* MatchGameState = BoundGameState.Get();
	if (!HUD || !MatchGameState)
	{
		return;
	}

	HUD->BP_OnMatchInfoChanged(
		FText::FromString(MatchGameState->GetSessionDisplayName()),
		MatchGameState->PlayerArray.Num(),
		MatchGameState->GetMaxPlayers());
}

void AGameUIMatchPlayerController::SetReady(bool bReady)
{
	ServerSetReady(bReady);
}

void AGameUIMatchPlayerController::ToggleReady()
{
	const AGameUIMatchPlayerState* MatchPlayerState = GetPlayerState<AGameUIMatchPlayerState>();
	ServerSetReady(!(MatchPlayerState && MatchPlayerState->IsReady()));
}

void AGameUIMatchPlayerController::RequestStartMatch()
{
	ServerRequestStartMatch();
}

void AGameUIMatchPlayerController::CancelStartMatch()
{
	ServerCancelStartMatch();
}

void AGameUIMatchPlayerController::LeaveMatch()
{
	if (UGameUIFlowSubsystem* FlowSubsystem = UGameInstance::GetSubsystem<UGameUIFlowSubsystem>(GetGameInstance()))
	{
		FlowSubsystem->ReturnToFrontend();
	}
}

bool AGameUIMatchPlayerController::ServerSetReady_Validate(bool bReady)
{
	return true;
}

void AGameUIMatchPlayerController::ServerSetReady_Implementation(bool bReady)
{
	if (AGameUIMatchPlayerState* MatchPlayerState = GetPlayerState<AGameUIMatchPlayerState>())
	{
		MatchPlayerState->SetReady(bReady);
	}
}

bool AGameUIMatchPlayerController::ServerRequestStartMatch_Validate()
{
	return true;
}

void AGameUIMatchPlayerController::ServerRequestStartMatch_Implementation()
{
	AGameUIMatchGameMode* MatchGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGameUIMatchGameMode>() : nullptr;
	if (!MatchGameMode)
	{
		return;
	}

	FText FailReason;
	if (!MatchGameMode->RequestStartMatch(this, FailReason))
	{
		ClientStartMatchRejected(FailReason.ToString());
	}
}

bool AGameUIMatchPlayerController::ServerCancelStartMatch_Validate()
{
	return true;
}

void AGameUIMatchPlayerController::ServerCancelStartMatch_Implementation()
{
	// Only the host may cancel directly; everyone else cancels implicitly by un-readying.
	const AGameUIMatchPlayerState* MatchPlayerState = GetPlayerState<AGameUIMatchPlayerState>();
	if (!MatchPlayerState || !MatchPlayerState->IsHost())
	{
		return;
	}

	if (AGameUIMatchGameMode* MatchGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGameUIMatchGameMode>() : nullptr)
	{
		MatchGameMode->CancelStartCountdown();
	}
}

void AGameUIMatchPlayerController::ClientStartMatchRejected_Implementation(const FString& Reason)
{
	if (UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this))
	{
		FrontendUISubsystem->PushConfirmScreenToModalStackAsync(
			EConfirmScreenType::Ok,
			LOCTEXT("StartRejectedTitle", "Cannot Start"),
			FText::FromString(Reason),
			[](EConfirmScreenButtonType) {});
	}
}

//~ End lobby

#undef LOCTEXT_NAMESPACE
