// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "FrontendGameUserSettings.generated.h"

/**
 * 
 */
UCLASS()
class FRONTENDUI_API UFrontendGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	UFrontendGameUserSettings();

	static UFrontendGameUserSettings* Get();

	//***** Gameplay Collection Tab ***** //
	UFUNCTION()
	FString GetCurrentGameDifficulty() const { return CurrentGameDifficulty; }

	UFUNCTION()
	void SetCurrentGameDifficulty(const FString& InNewDifficulty) { CurrentGameDifficulty = InNewDifficulty; }
	//***** Gameplay Collection Tab ***** //

	//***** Audio Collection Tab ***** //
	UFUNCTION()
	float GetOverallVolume() const { return OverallVolume; }

	UFUNCTION()
	void SetOverallVolume(float InVolume);
	//***** Audio Collection Tab ***** //

private:
	//***** Gameplay Collection Tab ***** //
	UPROPERTY(Config)
	FString CurrentGameDifficulty;
	//***** Gameplay Collection Tab ***** //

	//***** Audio Collection Tab ***** //
	UPROPERTY(Config)
	float OverallVolume;
	//***** Audio Collection Tab ***** //
};