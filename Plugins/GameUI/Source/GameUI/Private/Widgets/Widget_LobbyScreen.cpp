// Diego Gallo All Rights Reserved


#include "Widgets/Widget_LobbyScreen.h"
#include "Widgets/GameUILobbyPlayerEntryData.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "Controllers/GameUIMatchPlayerController.h"
#include "GameModes/GameUIMatchGameState.h"
#include "GameModes/GameUIMatchPlayerState.h"
#include "Subsystems/FrontendUISubsystem.h"
#include "CommonListView.h"
#include "CommonTextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "GameUILobby"

UWidget_LobbyScreen::UWidget_LobbyScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Back means Leave; handled natively so the confirmation cannot be bypassed.
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
}

void UWidget_LobbyScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CommonButton_Ready->OnClicked().AddUObject(this, &ThisClass::OnReadyClicked);
	CommonButton_Start->OnClicked().AddUObject(this, &ThisClass::OnStartClicked);
	CommonButton_Leave->OnClicked().AddUObject(this, &ThisClass::OnLeaveClicked);
}

void UWidget_LobbyScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// Force every BP_* event to fire once on (re)activation, whatever the cached values.
	LastShownCountdownSeconds = -1;
	bLastShownReady = !bLastShownReady;
	bLastShownHost = !bLastShownHost;
	bLastShownCanStart = !bLastShownCanStart;

	BindToGameState();
	HandleLobbyStateChanged();
	HandleStartCountdownChanged();
}

void UWidget_LobbyScreen::NativeOnDeactivated()
{
	UnbindFromGameState();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownDisplayTimerHandle);
	}

	Super::NativeOnDeactivated();
}

bool UWidget_LobbyScreen::NativeOnHandleBackAction()
{
	OnLeaveClicked();
	return true;
}

UWidget* UWidget_LobbyScreen::NativeGetDesiredFocusTarget() const
{
	return CommonButton_Ready;
}

TOptional<FUIInputConfig> UWidget_LobbyScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

//~ Begin button handlers

void UWidget_LobbyScreen::OnReadyClicked()
{
	if (AGameUIMatchPlayerController* MatchController = GetMatchPlayerController())
	{
		MatchController->ToggleReady();
	}
}

void UWidget_LobbyScreen::OnStartClicked()
{
	AGameUIMatchPlayerController* MatchController = GetMatchPlayerController();
	AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (!MatchController || !MatchGameState)
	{
		return;
	}

	// The same button cancels a running countdown; the Blueprint relabels it from BP_OnCountdownChanged.
	if (MatchGameState->IsStartCountdownActive())
	{
		MatchController->CancelStartMatch();
	}
	else
	{
		MatchController->RequestStartMatch();
	}
}

void UWidget_LobbyScreen::OnLeaveClicked()
{
	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(this);
	if (!FrontendUISubsystem)
	{
		return;
	}

	const AGameUIMatchPlayerState* LocalPlayerState = GetLocalMatchPlayerState();
	const bool bIsHost = LocalPlayerState && LocalPlayerState->IsHost();

	const FText Message = bIsHost
		? LOCTEXT("LeaveLobbyHostMessage", "You are the host. Leaving ends the session for everyone. Leave anyway?")
		: LOCTEXT("LeaveLobbyMessage", "Leave the lobby and return to the main menu?");

	TWeakObjectPtr<UWidget_LobbyScreen> WeakThis(this);

	FrontendUISubsystem->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::YesNo,
		LOCTEXT("LeaveLobbyTitle", "Leave Lobby"),
		Message,
		[WeakThis](EConfirmScreenButtonType ClickedButton)
		{
			if (ClickedButton != EConfirmScreenButtonType::Confirmed || !WeakThis.IsValid())
			{
				return;
			}

			if (AGameUIMatchPlayerController* MatchController = WeakThis->GetMatchPlayerController())
			{
				MatchController->LeaveMatch();
			}
		}
	);
}

//~ End button handlers

//~ Begin game state binding

void UWidget_LobbyScreen::BindToGameState()
{
	AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (!MatchGameState || BoundGameState == MatchGameState)
	{
		return;
	}

	UnbindFromGameState();

	BoundGameState = MatchGameState;
	MatchGameState->OnLobbyStateChanged.AddUObject(this, &ThisClass::HandleLobbyStateChanged);
	MatchGameState->OnStartCountdownChanged.AddUObject(this, &ThisClass::HandleStartCountdownChanged);
}

void UWidget_LobbyScreen::UnbindFromGameState()
{
	if (AGameUIMatchGameState* MatchGameState = BoundGameState.Get())
	{
		MatchGameState->OnLobbyStateChanged.RemoveAll(this);
		MatchGameState->OnStartCountdownChanged.RemoveAll(this);
	}
	BoundGameState.Reset();
}

void UWidget_LobbyScreen::HandleLobbyStateChanged()
{
	// The game state may arrive after the screen on a client; latch on as soon as it exists.
	if (!BoundGameState.IsValid())
	{
		BindToGameState();
	}

	RefreshHeader();
	RefreshPlayerList();
	RefreshButtons();
}

void UWidget_LobbyScreen::HandleStartCountdownChanged()
{
	UWorld* World = GetWorld();
	AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (!World)
	{
		return;
	}

	if (MatchGameState && MatchGameState->IsStartCountdownActive())
	{
		// Poll the replicated end time rather than ticking: whole seconds are all the UI shows.
		World->GetTimerManager().SetTimer(CountdownDisplayTimerHandle, this, &ThisClass::TickCountdownDisplay, 0.2f, true);
		TickCountdownDisplay();
	}
	else
	{
		World->GetTimerManager().ClearTimer(CountdownDisplayTimerHandle);
		LastShownCountdownSeconds = -1;
		BP_OnCountdownChanged(0, false);
	}

	RefreshButtons();
}

void UWidget_LobbyScreen::TickCountdownDisplay()
{
	const AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (!MatchGameState || !MatchGameState->IsStartCountdownActive())
	{
		return;
	}

	const int32 SecondsRemaining = FMath::CeilToInt(MatchGameState->GetStartCountdownSecondsRemaining());
	if (SecondsRemaining != LastShownCountdownSeconds)
	{
		LastShownCountdownSeconds = SecondsRemaining;
		BP_OnCountdownChanged(SecondsRemaining, true);
	}
}

//~ End game state binding

//~ Begin refresh

void UWidget_LobbyScreen::RefreshHeader()
{
	const AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	if (!MatchGameState)
	{
		CommonText_SessionName->SetText(LOCTEXT("LobbyConnecting", "Connecting..."));
		CommonText_PlayerCount->SetText(FText::GetEmpty());
		return;
	}

	CommonText_SessionName->SetText(FText::FromString(MatchGameState->GetSessionDisplayName()));

	const int32 CurrentPlayers = MatchGameState->PlayerArray.Num();
	const int32 MaxPlayers = MatchGameState->GetMaxPlayers();
	CommonText_PlayerCount->SetText(MaxPlayers > 0
		? FText::Format(LOCTEXT("PlayerCountWithMax", "{0} / {1}"), CurrentPlayers, MaxPlayers)
		: FText::AsNumber(CurrentPlayers));
}

void UWidget_LobbyScreen::RefreshPlayerList()
{
	const AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	const AGameUIMatchPlayerState* LocalPlayerState = GetLocalMatchPlayerState();

	TArray<UObject*> ListItems;
	if (!MatchGameState)
	{
		CommonListView_Players->SetListItems(ListItems);
		return;
	}

	// Host first, then by arrival; PlayerArray is already in arrival order.
	TArray<AGameUIMatchPlayerState*> Sorted;
	Sorted.Reserve(MatchGameState->PlayerArray.Num());
	for (APlayerState* PlayerState : MatchGameState->PlayerArray)
	{
		if (AGameUIMatchPlayerState* MatchPlayerState = Cast<AGameUIMatchPlayerState>(PlayerState))
		{
			if (!MatchPlayerState->IsInactive())
			{
				Sorted.Add(MatchPlayerState);
			}
		}
	}
	Sorted.StableSort([](const AGameUIMatchPlayerState& A, const AGameUIMatchPlayerState& B)
	{
		return A.IsHost() && !B.IsHost();
	});

	ListItems.Reserve(Sorted.Num());
	for (const AGameUIMatchPlayerState* MatchPlayerState : Sorted)
	{
		UGameUILobbyPlayerEntryData* Entry = NewObject<UGameUILobbyPlayerEntryData>(this);
		Entry->PlayerName = MatchPlayerState->GetPlayerName();
		Entry->PlayerId = MatchPlayerState->GetPlayerId();
		Entry->bIsReady = MatchPlayerState->IsReady();
		Entry->bIsHost = MatchPlayerState->IsHost();
		Entry->bIsLocalPlayer = (MatchPlayerState == LocalPlayerState);
		ListItems.Add(Entry);
	}

	CommonListView_Players->SetListItems(ListItems);
}

void UWidget_LobbyScreen::RefreshButtons()
{
	const AGameUIMatchGameState* MatchGameState = GetMatchGameState();
	const AGameUIMatchPlayerState* LocalPlayerState = GetLocalMatchPlayerState();

	const bool bIsReady = LocalPlayerState && LocalPlayerState->IsReady();
	const bool bIsHost = LocalPlayerState && LocalPlayerState->IsHost();

	FText StartReason;
	bool bCanStart = MatchGameState && MatchGameState->CanStartMatch(StartReason);
	const bool bCountdownActive = MatchGameState && MatchGameState->IsStartCountdownActive();

	// The host may always press the button while a countdown runs: it cancels.
	if (bCountdownActive)
	{
		bCanStart = true;
		StartReason = FText::GetEmpty();
	}

	CommonButton_Ready->SetIsEnabled(MatchGameState != nullptr);
	CommonButton_Start->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	CommonButton_Start->SetIsEnabled(bIsHost && bCanStart);

	if (bIsReady != bLastShownReady)
	{
		bLastShownReady = bIsReady;
		BP_OnReadyStateChanged(bIsReady);
	}

	if (bIsHost != bLastShownHost)
	{
		bLastShownHost = bIsHost;
		BP_OnHostStateChanged(bIsHost);
	}

	// Reason text can change while bCanStart stays false (e.g. 1/3 ready -> 2/3), so always forward it.
	bLastShownCanStart = bCanStart;
	BP_OnStartAvailabilityChanged(bCanStart, StartReason);
}

//~ End refresh

AGameUIMatchGameState* UWidget_LobbyScreen::GetMatchGameState() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetGameState<AGameUIMatchGameState>() : nullptr;
}

AGameUIMatchPlayerController* UWidget_LobbyScreen::GetMatchPlayerController() const
{
	return Cast<AGameUIMatchPlayerController>(GetOwningPlayer());
}

AGameUIMatchPlayerState* UWidget_LobbyScreen::GetLocalMatchPlayerState() const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	return PlayerController ? PlayerController->GetPlayerState<AGameUIMatchPlayerState>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
