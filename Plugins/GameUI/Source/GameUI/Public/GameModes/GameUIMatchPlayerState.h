// Diego Gallo All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameUIMatchPlayerState.generated.h"

/**
 * Player state for gameplay maps: lobby Ready flag and whether this player is the
 * listen host. Both are set on the server and replicated; clients never guess.
 */
UCLASS()
class GAMEUI_API AGameUIMatchPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	bool IsReady() const { return bIsReady; }

	UFUNCTION(BlueprintPure, Category = "Game UI|Lobby")
	bool IsHost() const { return bIsHost; }

	/** Server only. */
	void SetReady(bool bInReady);

	/** Server only. */
	void SetIsHost(bool bInIsHost);

	/** Server-only GameLift PlayerSessionId parsed from ?PlayerSessionId= during login. */
	const FString& GetPlayerSessionId() const { return PlayerSessionId; }
	void SetPlayerSessionId(const FString& InPlayerSessionId) { PlayerSessionId = InPlayerSessionId; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyFlagsChanged, AGameUIMatchPlayerState*);
	/** Ready or host flag changed, on server and clients alike. */
	FOnLobbyFlagsChanged OnLobbyFlagsChanged;

protected:
	//~ Begin AActor Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AActor Interface

	//~ Begin APlayerState Interface
	virtual void CopyProperties(APlayerState* PlayerState) override;
	/** The state can replicate before its name; refresh the lobby row when the name arrives. */
	virtual void OnRep_PlayerName() override;
	//~ End APlayerState Interface

	UFUNCTION()
	void OnRep_LobbyFlags();

private:
	void BroadcastLobbyFlagsChanged();

	UPROPERTY(ReplicatedUsing = OnRep_LobbyFlags)
	bool bIsReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyFlags)
	bool bIsHost = false;

	/** Server-only GameLift PlayerSessionId; not replicated to other clients. */
	FString PlayerSessionId;
};
