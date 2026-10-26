// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Controllers/FrontendPlayerController.h"
#include "GameUIMatchPlayerController.generated.h"

class UWidget_PrimaryLayout;
class UWidget_HUDBase;
class UWidget_PauseMenu;

/**
 * Player controller for gameplay maps. On BeginPlay it creates the primary layout, pushes
 * the HUD onto the GameHud stack under a loading-screen hold, and binds the pause action.
 * The frontend map keeps its own (Blueprint) controller; the two flows stay separate.
 */
UCLASS()
class GAMEUI_API AGameUIMatchPlayerController : public AFrontendPlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Game UI|Pause Menu")
	void OpenPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Pause Menu")
	void ClosePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Game UI|Pause Menu")
	void TogglePauseMenu();

	UFUNCTION(BlueprintPure, Category = "Game UI|Pause Menu")
	bool IsPauseMenuOpen() const { return bIsPauseMenuOpen; }

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin APlayerController Interface
	virtual void SetupInputComponent() override;
	//~ End APlayerController Interface

private:
	void AddMatchMappingContext();
	void CreatePrimaryLayout();
	void PushHUD();

	void OnPauseActionTriggered();
	void HandlePauseMenuCloseRequested();
	void SetWorldPausedForMenu(bool bPaused);

	UPROPERTY(Transient)
	TObjectPtr<UWidget_PrimaryLayout> CreatedPrimaryLayout;

	TWeakObjectPtr<UWidget_HUDBase> ActiveHUD;
	TWeakObjectPtr<UWidget_PauseMenu> ActivePauseMenu;

	bool bIsPauseMenuOpen = false;
};
