// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pasive.generated.h"

UCLASS()
class TACTICINVASORS_API APasive : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APasive();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
