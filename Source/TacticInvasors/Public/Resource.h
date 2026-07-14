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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	bool bPlayer1 = false;
	bool bPlayer2 = false;
	bool bTaked1 = false;
	bool bTaked2 = false;
	

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool Aviable();
	void SetPlayer();
	void SetTaked(AStrategyPlayer* ATaker);
	void FreeResource();

private:
	AStrategyPlayer* APlayer1 = nullptr;
};
