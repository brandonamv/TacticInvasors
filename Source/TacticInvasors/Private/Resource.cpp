// Fill out your copyright notice in the Description page of Project Settings.


#include "Resource.h"
#include "Spawner.h"
// Sets default values
AResource::AResource()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AResource::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AResource::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AResource::Aviable()
{
	return !bPlayer1 || !bPlayer2;
}

void AResource::SetPlayer()
{
	if (!bPlayer1)
	{
		bPlayer1 = true;
		return;
	}
	if (!bPlayer2)
	{
		bPlayer2 = true;
	}
}

void AResource::SetTaked()
{
	if (!bTaked1)
	{
		bTaked1 = true;
		return;
	}
	if (!bTaked2)
	{
		bTaked1 = false;
		bPlayer1 = false;
		bPlayer2 = false;
		this->GetOwner<ASpawner>()->PushFreeResource(this);
	}

}
