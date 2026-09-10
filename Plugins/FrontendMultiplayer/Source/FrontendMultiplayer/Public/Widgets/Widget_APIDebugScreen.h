// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_APIDebugScreen.generated.h"

class UCommonTextBlock;
class UScrollBox;

/**
 * Scrollable API debug log fed by UFrontendMultiplayerSubsystem::OnAPIDebugLogUpdated.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_APIDebugScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

private:
	UFUNCTION()
	void HandleAPIDebugLogUpdated(const FString& LogLine);

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UScrollBox* ScrollBox_DebugLog;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Text_DebugLog;
	//****** Bound Widgets ****** //
};
