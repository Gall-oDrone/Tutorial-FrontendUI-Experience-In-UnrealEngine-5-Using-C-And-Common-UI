// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "Widget_OptionsScreen.generated.h"

class UOptionsDataRegistry;
class UFrontendTabListWidgetBase;
class UFrontendCommonListView;
class UWidget_OptionsDetailsView;
class UListDataObject_Base;

/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_OptionsScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()
	
protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	//~ Begin UCommonActivatableWidget Interface
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	//~ End UCommonActivatableWidget Interface

private:
	UOptionsDataRegistry* GetOrCreateDataRegistry();

	void OnResetBoundActionTriggered();
	void OnBackBoundActionTriggered();

	UFUNCTION()
	void OnOptionsTabSelected(FName TabId);

	void OnListViewItemHovered(UObject* InHoveredItem, bool bWasHovered);
	void OnListViewItemSelected(UObject* InSelectedItem);

	FString TryGetEntryWidgetClassName(UObject* InOwningListItem) const;
	 
	void OnListViewListDataModified(UListDataObject_Base* ModifiedData, EOptionsListDataModifyReason ModifiedReason);

	//****** Bound Widgets ****** //
	UPROPERTY(meta = (BindWidget))
	UFrontendTabListWidgetBase* TabListWidget_OptionsTabs;

	UPROPERTY(meta = (BindWidget))
	UFrontendCommonListView* CommonListView_OptionsList;

	UPROPERTY(meta = (BindWidget))
	UWidget_OptionsDetailsView* DetailsView_ListEntryInfo;
	//****** Bound Widgets ****** //
	

	//Handle the creation of data in the options sreen. Direct access to this variable is forbidden.
	UPROPERTY(Transient)
	UOptionsDataRegistry* CreatedOwningDataRegistry;

	UPROPERTY(EditDefaultsOnly, Category = "Frontend Options Screen", meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle ResetAction;

	FUIActionBindingHandle ResetActionHandle;

	UPROPERTY(Transient)
	TArray<UListDataObject_Base*> ResettableDataArray;
};
