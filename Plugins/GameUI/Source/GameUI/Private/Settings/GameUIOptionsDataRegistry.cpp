// Diego Gallo All Rights Reserved


#include "Settings/GameUIOptionsDataRegistry.h"

void UGameUIOptionsDataRegistry::InitGameplayCollectionTab()
{
	// Keep the framework's Gameplay tab (currently: Difficulty) and extend it here.
	// To replace it entirely, drop the Super call and build the collection yourself,
	// appending it to RegisteredOptionsTabCollections.
	Super::InitGameplayCollectionTab();
}
