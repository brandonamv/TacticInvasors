// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Resource.generated.h"

class AStrategyPlayer;

UCLASS()
class TACTICINVASORS_API AResource : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AResource();

	// Exposed pointers to the assigned players (two slots)
	UPROPERTY()
	AStrategyPlayer* APlayer1 = nullptr;

	UPROPERTY()
	AStrategyPlayer* APlayer2 = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Internal occupancy flags
	UPROPERTY()
	bool bPlayer1 = false;

	UPROPERTY()
	bool bPlayer2 = false;

	// Taked flags (indicate the player reached the resource)
	UPROPERTY()
	bool bTaked1 = false;

	UPROPERTY()
	bool bTaked2 = false;
	

public:	

	// Returns true if at least one player slot is free
	bool Aviable();

	// Assign a player to an available slot (fills APlayer1 then APlayer2)
	void SetAsigned(AStrategyPlayer* Player);

	// Called by the player when it has taken the resource
	void SetTaked(AStrategyPlayer* Player);

	// Release/reset the resource
	void FreeResource();
};
