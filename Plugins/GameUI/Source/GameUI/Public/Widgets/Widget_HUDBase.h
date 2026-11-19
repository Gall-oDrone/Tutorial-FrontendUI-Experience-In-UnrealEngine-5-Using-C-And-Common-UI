// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_HUDBase.generated.h"

class UCommonTextBlock;

/**
 * Base for the in-match HUD. Lives on Frontend.WidgetStack.GameHud, never takes focus,
 * and keeps the game in Game input mode so gameplay input is unaffected by it being up.
 * Subclass in Blueprint and add the actual widgets; react to the pause menu through
 * BP_OnPauseMenuVisibilityChanged.
 *
 * Match content the C++ keeps current, each both as an optional text block (name it as
 * below in the Blueprint and it is filled in) and as a BP_* event for anything fancier:
 *
 *   CommonText_SessionName   the session's title
 *   CommonText_PlayerCount   "2 / 4" (or "2" when the capacity is unknown)
 *   CommonText_MatchClock    time since the match started, "--:--" in the lobby
 *   CommonText_Ping          the local player's ping, "42 ms"
 *
 * None of them is required: a HUD without these widgets works as before.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class GAMEUI_API UWidget_HUDBase : public UWidget_ActivatableBase
{
	GENERATED_BODY()

public:
	UWidget_HUDBase(const FObjectInitializer& ObjectInitializer);

	/** Called by the match player controller when the pause menu opens or closes (hide crosshairs, dim, etc.). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|HUD")
	void BP_OnPauseMenuVisibilityChanged(bool bIsPauseMenuOpen);

	/** Session name and occupancy, sent on arrival and whenever a player joins or leaves. MaxPlayers is 0 if unknown. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|HUD")
	void BP_OnMatchInfoChanged(const FText& SessionName, int32 CurrentPlayers, int32 MaxPlayers);

	/**
	 * The match clock moved: once a second while the match is in progress, and once when it
	 * starts or stops. ElapsedSeconds is 0 and ClockText "--:--" while it is not in progress.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|HUD")
	void BP_OnMatchClockChanged(bool bIsMatchInProgress, int32 ElapsedSeconds, const FText& ClockText);

	/** The local player's ping changed (milliseconds, as the server measures it; 0 when not networked). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|HUD")
	void BP_OnPingChanged(int32 PingInMilliseconds);

	/** Called by the match player controller: fills the session texts, then fires BP_OnMatchInfoChanged. */
	void SetMatchInfo(const FText& SessionName, int32 CurrentPlayers, int32 MaxPlayers);

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget Interface

	//~ Begin UCommonActivatableWidget Interface
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	//~ End UCommonActivatableWidget Interface

private:
	/** Match clock and ping, read from the game state and the owning player's state twice a second. */
	void RefreshLiveStats();

	//****** Bound Widgets (all optional) ****** //
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	UCommonTextBlock* CommonText_SessionName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	UCommonTextBlock* CommonText_PlayerCount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	UCommonTextBlock* CommonText_MatchClock;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	UCommonTextBlock* CommonText_Ping;
	//****** Bound Widgets (all optional) ****** //

	FTimerHandle LiveStatsTimerHandle;

	/** What was last shown, so the texts and events only change when the values do. -1 = nothing yet. */
	int32 LastShownElapsedSeconds = -1;
	int32 LastShownPing = -1;
	bool bLastShownMatchInProgress = false;
};
