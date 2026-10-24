// Diego Gallo All Rights Reserved


#include "FrontendFunctionLibrary.h"
#include "FrontendSettings/FrontendDeveloperSettings.h"

TSoftClassPtr<UWidget_ActivatableBase> UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(UPARAM(meta = (Categories = "Frontend.Widget")) FGameplayTag InWidgetTag)
{
	const UFrontendDeveloperSettings* FrontendDeveloperSettings = GetDefault<UFrontendDeveloperSettings>();

	checkf(FrontendDeveloperSettings->FrontendWidgetMap.Contains(InWidgetTag), TEXT("Could not find the corresponding widget under the tag %s"),*InWidgetTag.ToString());

	return FrontendDeveloperSettings->FrontendWidgetMap.FindRef(InWidgetTag);
	
}

TSoftObjectPtr<UTexture2D> UFrontendFunctionLibrary::GetOptionsSoftImageByTag(FGameplayTag InImageTag)
{
	const UFrontendDeveloperSettings* FrontendDeveloperSettings = GetDefault<UFrontendDeveloperSettings>();

	// Unlike widget classes, a missing image is cosmetic: UWidget_OptionsDetailsView already
	// skips null images, so degrade to "no image" and say so rather than asserting.
	if (const TSoftObjectPtr<UTexture2D>* FoundImage = FrontendDeveloperSettings->OptionsScreenSoftImageMap.Find(InImageTag))
	{
		return *FoundImage;
	}

	UE_LOG(LogTemp, Warning, TEXT("FrontendUI: no image mapped to tag %s in Frontend UI Settings -> Options Image Reference."), *InImageTag.ToString());
	return TSoftObjectPtr<UTexture2D>();
}

bool UFrontendFunctionLibrary::IsLogoScreenEnabled()
{
#if UE_BUILD_SHIPPING
	// The studio splash is a brand / legal requirement in shipped builds; the toggle is
	// an iteration convenience and must not be able to turn it off there.
	return true;
#else
	return GetDefault<UFrontendDeveloperSettings>()->bShowLogoScreen;
#endif
}