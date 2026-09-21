// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Widget_ListEntry_Leaderboard.generated.h"

class UCommonTextBlock;

/**
 * List row for one leaderboard entry. Independent of Widget_ListEntry_Base for the
 * same reason Widget_ListEntry_Session is: that base CastChecks every item to
 * UListDataObject_Base, which a read-only ranking row is not.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_ListEntry_Leaderboard : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	//~ Begin IUserObjectListEntry Interface
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	//~ End IUserObjectListEntry Interface

	/**
	 * Called after the row text is populated so the Blueprint can style the local
	 * player's row. Kept as an event rather than a hard-coded colour here so the
	 * highlight stays with the rest of the visual authoring.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Frontend Leaderboard Entry")
	void BP_OnLocalPlayerStateChanged(bool bIsLocalPlayer);

private:
	//***** Bound Widgets ***** //
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonText_Rank;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonText_PlayerName;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonText_Score;
	//***** Bound Widgets ***** //
};
