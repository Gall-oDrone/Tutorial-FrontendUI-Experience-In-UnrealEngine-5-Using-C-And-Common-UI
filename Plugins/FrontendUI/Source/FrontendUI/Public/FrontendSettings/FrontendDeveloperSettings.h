// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "FrontendDeveloperSettings.generated.h"

class UWidget_ActivatableBase;
/**
 * 
 */
UCLASS(Config = Game,defaultconfig, meta = (DisplayName = "Frontend UI Settings"))
class FRONTENDUI_API UFrontendDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Config,EditAnywhere, Category = "Widget Reference", meta = (ForceInlineRow,Categories = "Frontend.Widget"))
	TMap<FGameplayTag, TSoftClassPtr<UWidget_ActivatableBase>> FrontendWidgetMap;

	UPROPERTY(Config, EditAnywhere, Category = "Options Image Reference", meta = (ForceInlineRow, Categories = "Frontend.Image"))
	TMap< FGameplayTag, TSoftObjectPtr<UTexture2D>> OptionsScreenSoftImageMap;

	/** Show the studio logo screen before Press Any Key. When off, the boot flow skips straight to Press Any Key. */
	UPROPERTY(Config, EditAnywhere, Category = "Boot Flow")
	bool bShowLogoScreen = true;
};
