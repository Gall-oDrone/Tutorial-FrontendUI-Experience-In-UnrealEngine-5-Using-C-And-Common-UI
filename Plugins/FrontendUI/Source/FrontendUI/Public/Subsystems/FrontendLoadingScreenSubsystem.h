// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrontendLoadingScreenSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class FRONTENDUI_API UFrontendLoadingScreenSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadingReasonUpdatedDelegate, const FString&, CurrentLoadingReason);

	UPROPERTY(BlueprintAssignable)
	FOnLoadingReasonUpdatedDelegate OnLoadingReasonUpdated;

	/**
	 * Keep the loading screen up (or bring it back) until ClearLoadingScreenHold is called
	 * with the same tag. Use it for work the engine cannot see: the frontend pushing its
	 * first screen, joining a session, streaming a save. InReason is shown on the screen.
	 */
	UFUNCTION(BlueprintCallable, Category = "Frontend Loading Screen")
	void RegisterLoadingScreenHold(FName InHoldTag, const FString& InReason);

	UFUNCTION(BlueprintCallable, Category = "Frontend Loading Screen")
	void ClearLoadingScreenHold(FName InHoldTag);

	UFUNCTION(BlueprintPure, Category = "Frontend Loading Screen")
	bool HasActiveLoadingScreenHolds() const { return !ActiveLoadingScreenHolds.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "Frontend Loading Screen")
	bool IsLoadingScreenVisible() const { return CachedCreatedLoadingScreenWidget.IsValid(); }

	//~ Begin USubsystem Interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const;
	virtual void Initialize(FSubsystemCollectionBase& Collection);
	virtual void Deinitialize();
	//~ End USubsystem Interface

	//~ Begin FTickableGameObject Interface
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject Interface

private:
	void OnMapPreloaded(const FWorldContext& WorldContext, const FString& MapName);

	void OnMapPostLoaded(UWorld* LoadedWorld);

	void TryUpdateLoadingScreen();

	bool IsPreLoadScreenActive() const;

	bool ShouldShowLoadingScreen();

	bool CheckTheNeedToShowLoadingScreen();

	void TryDisplayLoadingScreenIfNone();

	void TryRemoveLoadingScreen();

	void NotifyLoadingScreenVisibilityChange(bool bIsVisible);

	bool bIsCurrentlyLoadingMap = false;

	float HoldLoadingScreenStartUpTime = -1.f;

	FString CurrentLoadingReason;

	/** Holds registered through RegisterLoadingScreenHold, keyed by tag. */
	TMap<FName, FString> ActiveLoadingScreenHolds;

	TSharedPtr<SWidget> CachedCreatedLoadingScreenWidget;
};
