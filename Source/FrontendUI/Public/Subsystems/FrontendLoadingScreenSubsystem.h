// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrontendLoadingScreenSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class FRONTENDUI_API UFrontendLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	//~ Begin USubsystem Interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const;
	virtual void Initialize(FSubsystemCollectionBase& Collection);
	virtual void Deinitialize();
	//~ End USubsystem Interface

private:
	void OnMapPreloaded(const FWorldContext& WorldContext, const FString& MapName);

	void OnMapPostLoaded(UWorld* LoadedWorld);
};
