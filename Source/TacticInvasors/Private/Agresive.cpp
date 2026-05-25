// Fill out your copyright notice in the Description page of Project Settings.


#include "Agresive.h"

// Sets default values
AAgresive::AAgresive()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAgresive::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAgresive::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

