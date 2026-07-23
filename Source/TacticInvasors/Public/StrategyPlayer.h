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
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY()
	AResource* TargetResource = nullptr;

	UPROPERTY()
	bool bIsAgresive = false;

	UPROPERTY()
	float Fitnees;

	UPROPERTY()
	float ASpeed=0.0f;

	bool bHasTarget = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Sets default values for this actor's properties
	AStrategyPlayer();
	bool InitializeAgent();
	void SetAggresive(bool bAggresive) { bIsAgresive = bAggresive; }
	bool IsAggresive() const { return bIsAgresive; }
	void SetFitness(float InFitness) { Fitnees = InFitness; }
	float GetFitness() const { return Fitnees; }
	void SetSpeed(float InSpeed) { ASpeed = InSpeed; }
	float GetSpeed() const { return ASpeed; }
	void UpdateMeshLocation();

};
