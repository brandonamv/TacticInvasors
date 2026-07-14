// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyPlayer.generated.h"

class ASpawner;
class AResource;

UCLASS() 
class TACTICINVASORS_API AStrategyPlayer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AStrategyPlayer();
	bool InitializeAgent(ASpawner* InSpawner);
	void SetAggresive(bool bAggresive) { bIsAgresive = bAggresive; }
	bool IsAggresive() const { return bIsAgresive; }
	void SetFitness(int32 InFitness) { Fitnees = InFitness; }
	int32 GetFitness() const { return Fitnees; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY()
	AResource* TargetResource = nullptr;

	UPROPERTY()
	bool bIsAgresive = false;

	UPROPERTY()
	int32 Fitnees;

	bool bHasTarget = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
