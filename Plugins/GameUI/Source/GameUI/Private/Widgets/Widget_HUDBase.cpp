// Diego Gallo All Rights Reserved


#include "Widgets/Widget_HUDBase.h"
#include "GameModes/GameUIMatchGameState.h"
#include "CommonTextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "GameUIHUD"

namespace
{
	/** Often enough that the clock never visibly skips a second; the texts only change when a value does. */
	constexpr float LiveStatsRefreshSeconds = 0.5f;
}

UWidget_HUDBase::UWidget_HUDBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Not focusable: a HUD is drawn over the game, never interacted with. It must still
	// *support* activation focus, because CommonUI only treats such widgets as an active
	// root - and only an active root's input config (Game mode, here) is applied. With this
	// false, closing a menu above the HUD left CommonUI with no root and stuck in Menu mode.
	SetIsFocusable(false);
	bIsBackHandler = false;
}

void UWidget_HUDBase::NativeConstruct()
{
	Super::NativeConstruct();

	// Construct, not OnInitialized: a pooled widget is destructed and constructed again every
	// time it leaves and re-enters a stack, and the timer must follow that.
	LastShownElapsedSeconds = -1;
	LastShownPing = -1;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(LiveStatsTimerHandle, this, &ThisClass::RefreshLiveStats, LiveStatsRefreshSeconds, true);
	}

	RefreshLiveStats();
}

void UWidget_HUDBase::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LiveStatsTimerHandle);
	}

	Super::NativeDestruct();
}

TOptional<FUIInputConfig> UWidget_HUDBase::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
}

void UWidget_HUDBase::SetMatchInfo(const FText& SessionName, int32 CurrentPlayers, int32 MaxPlayers)
{
	if (CommonText_SessionName)
	{
		CommonText_SessionName->SetText(SessionName);
	}

	if (CommonText_PlayerCount)
	{
		CommonText_PlayerCount->SetText(MaxPlayers > 0
			? FText::Format(LOCTEXT("PlayerCountFmt", "{0} / {1}"), FText::AsNumber(CurrentPlayers), FText::AsNumber(MaxPlayers))
			: FText::AsNumber(CurrentPlayers));
	}

	BP_OnMatchInfoChanged(SessionName, CurrentPlayers, MaxPlayers);
}

void UWidget_HUDBase::RefreshLiveStats()
{
	const UWorld* World = GetWorld();

	// Match clock. AGameState counts ElapsedTime once a second while the match is in progress,
	// on the server and on every client; a client that joins late starts from the server's
	// value (the property replicates once, on arrival). Nothing of ours needs replicating.
	const AGameUIMatchGameState* MatchGameState = World ? World->GetGameState<AGameUIMatchGameState>() : nullptr;
	const bool bMatchInProgress = MatchGameState && MatchGameState->IsMatchInProgress();
	const int32 ElapsedSeconds = bMatchInProgress ? FMath::Max(0, MatchGameState->ElapsedTime) : 0;

	if (bMatchInProgress != bLastShownMatchInProgress || ElapsedSeconds != LastShownElapsedSeconds)
	{
		bLastShownMatchInProgress = bMatchInProgress;
		LastShownElapsedSeconds = ElapsedSeconds;

		const FText ClockText = bMatchInProgress
			? FText::AsTimespan(FTimespan::FromSeconds(static_cast<double>(ElapsedSeconds)))
			: LOCTEXT("MatchClockIdle", "--:--");

		if (CommonText_MatchClock)
		{
			CommonText_MatchClock->SetText(ClockText);
		}

		BP_OnMatchClockChanged(bMatchInProgress, ElapsedSeconds, ClockText);
	}

	// Ping of the player this HUD belongs to, as the server measures and replicates it.
	const APlayerState* OwningPlayerState = GetOwningPlayerState<APlayerState>();
	const int32 PingInMilliseconds = OwningPlayerState ? FMath::Max(0, FMath::RoundToInt(OwningPlayerState->GetPingInMilliseconds())) : 0;

	if (PingInMilliseconds != LastShownPing)
	{
		LastShownPing = PingInMilliseconds;

		if (CommonText_Ping)
		{
			CommonText_Ping->SetText(FText::Format(LOCTEXT("PingFmt", "{0} ms"), FText::AsNumber(PingInMilliseconds)));
		}

		BP_OnPingChanged(PingInMilliseconds);
	}
}

#undef LOCTEXT_NAMESPACE
