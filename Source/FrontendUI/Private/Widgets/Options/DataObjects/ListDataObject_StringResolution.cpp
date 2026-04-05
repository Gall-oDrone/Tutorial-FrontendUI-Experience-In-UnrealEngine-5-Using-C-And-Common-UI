// Diego Gallo All Rights Reserved


#include "Widgets/Options/DataObjects/ListDataObject_StringResolution.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Options/OptionsDataInteractionHelper.h"

#include "FrontendDebugHelper.h"

void UListDataObject_StringResolution::InitResolutionValues()
{
	TArray<FIntPoint> AvailableResolutions;

	UKismetSystemLibrary::GetSuppoertedFullscreenResolutions();

	for (const FIntPoint& Resolution : AvailableResolutions)
	{
		Debug::Print(TEXT("Available Resolution: ") + Resolution.ToString());
	}
}

void UListDataObject_StringResolution::OnDataObjectInitialized()
{
	Super::OnDataObjectInitialized();

	if (DataDynamicGetter)
	{
		Debug::Print(TEXT("Resolution Value from Dynamic Getter: ") + DataDynamicGetter->GetValueAsString());
		DataDynamicGetter->GetValueAsString();
	}
}
