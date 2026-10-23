// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OptionsDataRegistry.generated.h"

class UListDataObject_Base;
class UListDataObject_Collection;

/**
 * Builds the tab collections shown by UWidget_OptionsScreen.
 *
 * The framework provides the Audio, Video and Control tabs. A game extends the
 * options screen by subclassing this registry, overriding the Init* functions it
 * wants to change (typically InitGameplayCollectionTab), and pointing
 * UFrontendDeveloperSettings::OptionsDataRegistryClass at the subclass. No edits
 * to the FrontendUI plugin are required.
 */
UCLASS()
class FRONTENDUI_API UOptionsDataRegistry : public UObject
{
	GENERATED_BODY()
	
public:
	//Gets called by options screen right after the object of type UOptionsDataRegistry is created
	virtual void InitOptionsDataRegistry(ULocalPlayer* InOwningLocalPlayer);

	const TArray< UListDataObject_Collection*>& GetRegisteredOptionsTabCollections() const { return RegisteredOptionsTabCollections; }

	TArray<UListDataObject_Base*> GetListSourceItemsBySelectedTabID(const FName& InSelectedTabID) const;

protected:
	/** Override to add, replace or remove game-specific options. The base implementation registers a Difficulty rotator. */
	virtual void InitGameplayCollectionTab();
	virtual void InitAudioCollectionTab();
	virtual void InitVideoCollectionTab();
	virtual void InitControlCollectionTab(ULocalPlayer* InOwningLocalPlayer);

	/** Tabs in registration order; subclasses append to this from their Init* overrides. */
	UPROPERTY(Transient)
	TArray< UListDataObject_Collection*> RegisteredOptionsTabCollections;

private:
	void FindChildListDataRecursively(UListDataObject_Base* InParentData, TArray<UListDataObject_Base*>& OutFoundChildListData) const;
};
