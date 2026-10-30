// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_LobbyScreen.generated.h"

class UCommonListView;
class UCommonTextBlock;
class UFrontendCommonButtonBase;
class AGameUIMatchGameState;
class AGameUIMatchPlayerState;
class AGameUIMatchPlayerController;

/**
 * Pre-match lobby on Frontend.WidgetStack.GameMenu, pushed by the match player
 * controller while the game state is WaitingToStart and deactivated when the match starts.
 *
 * Everything it shows comes from AGameUIMatchGameState / AGameUIMatchPlayerState; every
 * action goes through the match player controller's server RPCs. Back = Leave (with
 * confirmation). The Blueprint handles styling through the BP_* events.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class GAMEUI_API UWidget_LobbyScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()

public:
	UWidget_LobbyScreen(const FObjectInitializer& ObjectInitializer);

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	//~ Begin UCommonActivatableWidget Interface
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual bool NativeOnHandleBackAction() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	//~ End UCommonActivatableWidget Interface

	/** The local player's Ready flag changed; relabel the Ready button. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Lobby")
	void BP_OnReadyStateChanged(bool bIsLocalPlayerReady);

	/** Host only. Whether Start may be pressed, with a human-readable reason when it may not. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Lobby")
	void BP_OnStartAvailabilityChanged(bool bCanStart, const FText& Reason);

	/** Fired once per second while a countdown runs, and once with bIsActive = false when it stops. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Lobby")
	void BP_OnCountdownChanged(int32 SecondsRemaining, bool bIsActive);

	/** Called on activation and whenever host status could have changed. Show/hide host-only chrome here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Lobby")
	void BP_OnHostStateChanged(bool bIsLocalPlayerHost);

private:
	void OnReadyClicked();
	void OnStartClicked();
	void OnLeaveClicked();

	void BindToGameState();
	void UnbindFromGameState();
	void HandleLobbyStateChanged();
	void HandleStartCountdownChanged();
	void TickCountdownDisplay();

	void RefreshHeader();
	void RefreshPlayerList();
	void RefreshButtons();

	AGameUIMatchGameState* GetMatchGameState() const;
	AGameUIMatchPlayerController* GetMatchPlayerController() const;
	AGameUIMatchPlayerState* GetLocalMatchPlayerState() const;

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UCommonListView* CommonListView_Players;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonText_SessionName;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonText_PlayerCount;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_Ready;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_Start;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_Leave;
	//****** Bound Widgets ****** //

	TWeakObjectPtr<AGameUIMatchGameState> BoundGameState;
	FTimerHandle CountdownDisplayTimerHandle;
	int32 LastShownCountdownSeconds = -1;
	bool bLastShownReady = false;
	bool bLastShownHost = false;
	bool bLastShownCanStart = false;
};
