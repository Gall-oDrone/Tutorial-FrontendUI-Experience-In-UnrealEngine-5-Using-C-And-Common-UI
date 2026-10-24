// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/OptionsDataRegistry.h"
#include "GameUIOptionsDataRegistry.generated.h"

/**
 * This game's options. The framework (FrontendUI) owns Audio, Video and Controls;
 * anything game-specific is added here by overriding the Init* functions.
 *
 * Activate it in Project Settings → Game → Frontend UI Settings →
 * Options Screen → Options Data Registry Class.
 */
UCLASS()
class GAMEUI_API UGameUIOptionsDataRegistry : public UOptionsDataRegistry
{
	GENERATED_BODY()

protected:
	//~ Begin UOptionsDataRegistry Interface
	virtual void InitGameplayCollectionTab() override;
	//~ End UOptionsDataRegistry Interface

private:
	/** Finds a tab the base class already registered, so game rows can be appended to it. */
	UListDataObject_Collection* FindTabCollection(const FName& InTabID) const;
};
