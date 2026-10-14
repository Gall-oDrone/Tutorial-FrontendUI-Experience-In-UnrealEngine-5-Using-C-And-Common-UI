// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_APIDebugScreen.generated.h"

class UCommonTextBlock;
class UScrollBox;

/**
 * Scrollable API debug log. Shows UFrontendMultiplayerSubsystem's full log history
 * on open and follows new lines while visible.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_APIDebugScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	//~ Begin UCommonActivatableWidget Interface
	virtual void NativeOnActivated() override;
	//~ End UCommonActivatableWidget Interface

private:
	void OnBackBoundActionTriggered();

	UFUNCTION()
	void HandleAPIDebugLogUpdated(const FString& LogLine);

	UFUNCTION()
	void HandleAPIDebugLogCleared();

	void RebuildLogFromHistory();

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UScrollBox* ScrollBox_DebugLog;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonTextBlock_DebugLog;
	//****** Bound Widgets ****** //
};
