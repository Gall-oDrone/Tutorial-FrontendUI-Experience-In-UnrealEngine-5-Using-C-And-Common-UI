// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "FrontendSessionTypes.generated.h"

/**
 * The asynchronous session request currently owned by the multiplayer subsystem.
 * Only one runs at a time; screens read it to disable buttons and show progress.
 */
UENUM(BlueprintType)
enum class EFrontendSessionOperation : uint8
{
	None,
	Creating,
	Finding,
	Joining,
	Leaving
};
