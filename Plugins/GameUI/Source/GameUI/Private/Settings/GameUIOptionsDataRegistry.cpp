// Diego Gallo All Rights Reserved


#include "Settings/GameUIOptionsDataRegistry.h"
#include "Widgets/Options/DataObjects/ListDataObject_Collection.h"
#include "Widgets/Options/DataObjects/ListDataObject_String.h"
#include "FrontendFunctionLibrary.h"
#include "FrontendGameplayTags.h"

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
	{
		UListDataObject_String* TestItem = NewObject<UListDataObject_String>();
		TestItem->SetDataID(FName("TestItem"));
		TestItem->SetDataDisplayName(FText::FromString(TEXT("Test Image Item")));
		TestItem->SetSoftDescriptionImage(UFrontendFunctionLibrary::GetOptionsSoftImageByTag(FrontendGameplayTags::Frontend_Image_TestImage));
		TestItem->SetDescriptionRichText(FText::FromString(TEXT("The image to display can be specified in the project settings. It can be anything the developer assigned in there")));

		GameplayTabCollection->AddChildListData(TestItem);
	}
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
