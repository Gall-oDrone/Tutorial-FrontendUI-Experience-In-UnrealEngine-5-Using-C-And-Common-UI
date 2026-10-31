// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Widget_ListEntry_LobbyPlayer.generated.h"

class UCommonTextBlock;

/**
 * One row of the lobby player list. Sets the name natively; the Blueprint styles the
 * ready / host / local markers from BP_OnPlayerEntryUpdated.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class GAMEUI_API UWidget_ListEntry_LobbyPlayer : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	/** Show a tick for ready, a crown/"Host" tag for the host, and highlight the local player's row. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game UI|Lobby")
	void BP_OnPlayerEntryUpdated(bool bIsReady, bool bIsHost, bool bIsLocalPlayer);

	//~ Begin IUserObjectListEntry Interface
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	//~ End IUserObjectListEntry Interface

private:
	//***** Bound Widgets ***** //
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	UCommonTextBlock* CommonText_PlayerName;
	//***** Bound Widgets ***** //
};
