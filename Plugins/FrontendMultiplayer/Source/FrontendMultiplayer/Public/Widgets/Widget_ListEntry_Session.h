// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Widget_ListEntry_Session.generated.h"

class UCommonButtonBase;
class UCommonTextBlock;
class UFrontendSessionListEntryData;

/**
 * List row for one found session. Independent of Widget_ListEntry_Base so it
 * can bind UFrontendSessionListEntryData without CastChecked to ListDataObject_Base.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_ListEntry_Session : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	//~ Begin IUserObjectListEntry Interface
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	//~ End IUserObjectListEntry Interface

private:
	void OnJoinButtonClicked();

	//***** Bound Widgets ***** //
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonText_SessionInfo;

	UPROPERTY(meta = (BindWidget))
	UCommonButtonBase* Button_Join;
	//***** Bound Widgets ***** //

	UPROPERTY(Transient)
	UFrontendSessionListEntryData* CachedSessionData;
};
