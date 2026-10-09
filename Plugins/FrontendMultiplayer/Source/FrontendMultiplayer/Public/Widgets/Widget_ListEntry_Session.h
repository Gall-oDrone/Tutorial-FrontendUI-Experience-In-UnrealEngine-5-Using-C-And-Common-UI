// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Widget_ListEntry_Session.generated.h"

class UCommonTextBlock;
class UFrontendCommonButtonBase;
class UFrontendSessionListEntryData;

/**
 * List row for one found session. Independent of Widget_ListEntry_Base so it
 * can bind UFrontendSessionListEntryData without CastChecked to ListDataObject_Base.
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDMULTIPLAYER_API UWidget_ListEntry_Session : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	void NativeOnListEntryWidgetHovered(bool bWasHovered);

protected:
	//The child widget blueprint should override it to handle the highlight state when this entry widget is hovered or selected
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Toggle Entry Widget Highlight State"))
	void BP_OnToggleEntryWidgetHighlightState(bool bShouldHighlight) const;

	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	//~ Begin IUserObjectListEntry Interface
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
	virtual void NativeOnEntryReleased() override;
	//~ End IUserObjectListEntry Interface

private:
	void OnJoinButtonClicked();

	//***** Bound Widgets ***** //
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	UCommonTextBlock* CommonText_SessionInfo;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonButtonBase* CommonButton_Join;
	//***** Bound Widgets ***** //

	UPROPERTY(Transient)
	UFrontendSessionListEntryData* CachedSessionData;
};
