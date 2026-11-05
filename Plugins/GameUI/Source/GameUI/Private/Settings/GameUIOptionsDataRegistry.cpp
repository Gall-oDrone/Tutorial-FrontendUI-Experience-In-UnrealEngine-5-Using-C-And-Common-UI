// Diego Gallo All Rights Reserved


#include "Settings/GameUIOptionsDataRegistry.h"
#include "Widgets/Options/DataObjects/ListDataObject_Collection.h"
#include "Widgets/Options/DataObjects/ListDataObject_String.h"
#include "FrontendFunctionLibrary.h"
#include "FrontendGameplayTags.h"

#define LOCTEXT_NAMESPACE "GameUI"

void UGameUIOptionsDataRegistry::InitGameplayCollectionTab()
{
	// Framework rows first (Difficulty), then this game's additions.
	Super::InitGameplayCollectionTab();

	UListDataObject_Collection* GameplayTabCollection = FindTabCollection(FName("GameplayTabCollection"));
	if (!GameplayTabCollection)
	{
		return;
	}

	// Demonstrates a details-panel image driven from Project Settings
	// (Frontend UI Settings -> Options Image Reference -> Frontend.Image.TestImage).
	// Development-only: it is a showcase row, not a game option.
#if !UE_BUILD_SHIPPING
	{
		UListDataObject_String* TestItem = NewObject<UListDataObject_String>();
		TestItem->SetDataID(FName("TestItem"));
		TestItem->SetDataDisplayName(LOCTEXT("TestImageItem", "Test Image Item"));
		TestItem->SetSoftDescriptionImage(UFrontendFunctionLibrary::GetOptionsSoftImageByTag(FrontendGameplayTags::Frontend_Image_TestImage));
		TestItem->SetDescriptionRichText(LOCTEXT("TestImageItemDesc", "The image shown here comes from Project Settings (Frontend UI Settings, Options Image Reference). Any texture the developer assigns there appears in this panel."));

		GameplayTabCollection->AddChildListData(TestItem);
	}
#endif
}

UListDataObject_Collection* UGameUIOptionsDataRegistry::FindTabCollection(const FName& InTabID) const
{
	for (UListDataObject_Collection* TabCollection : RegisteredOptionsTabCollections)
	{
		if (TabCollection && TabCollection->GetDataID() == InTabID)
		{
			return TabCollection;
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
