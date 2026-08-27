// Fill out your copyright notice in the Description page of Project Settings.

#include "Resource.h"
#include "Spawner.h"
#include "StrategyPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Containers/UnrealString.h"
#include "Components/StaticMeshComponent.h"

AResource::AResource()
{
 	PrimaryActorTick.bCanEverTick = false;
}

void AResource::BeginPlay()
{
	Super::BeginPlay();

	MeshComponent = FindComponentByClass<UStaticMeshComponent>();
}

bool AResource::Aviable()
{
	// Available if either slot is free
	return (!bPlayer1) || (!bPlayer2);
}

void AResource::SetAsigned(AStrategyPlayer* Player1, AStrategyPlayer* Player2)
{
	if (!IsValid(Player1) || !IsValid(Player2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: SetAsigned called with null Player."), *GetName());
		return;
	}

	bPlayer1 = true;
	bTaked1 = false;
	APlayer1 = Player1;
	Player1->InitializeAgent(this);
	UE_LOG(LogTemp, Log, TEXT("[%s]: Assigned Player1 = %s"), *GetName(), *Player1->GetName());

	bPlayer2 = true;
	bTaked2 = false;
	APlayer2 = Player2;
	Player2->InitializeAgent(this);
	UE_LOG(LogTemp, Log, TEXT("[%s]: Assigned Player2 = %s"), *GetName(), *Player2->GetName());
	return;

}

void AResource::SetTaked(AStrategyPlayer* Player)
{
	// Notify owner spawner that player is free (if available)
	ASpawner* OwnerSpawner = Cast<ASpawner>(GetOwner());
	if (IsValid(OwnerSpawner))
	{
		OwnerSpawner->PushFreePlayer(Player);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: SetTaked: Owner spawner missing."), *GetName());
	}

	// Mark correct taked flag and free if both taked
	if (Player == APlayer1)
	{
		if (!bTaked1)
		{
			bTaked1 = true;
			UE_LOG(LogTemp, Log, TEXT("[%s]: Player1 (%s) taked."), *GetName(), *Player->GetName());
		}
	}
	else if (Player == APlayer2)
	{
		if (!bTaked2)
		{
			bTaked2 = true;
			UE_LOG(LogTemp, Log, TEXT("[%s]: Player2 (%s) taked."), *GetName(), *Player->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: SetTaked called by unknown player %s"), *GetName(), *Player->GetName());
	}

	// If both taked, free the resource
	if (bTaked1 && bTaked2)
	{
		APlayer1->PlayInteraction(APlayer2->IsAggresive());
		APlayer2->PlayInteraction(APlayer1->IsAggresive());
		
		OwnerSpawner->ProcessPlayer(APlayer1);
		OwnerSpawner->ProcessPlayer(APlayer2);
		
		this->FreeResource();
	}
}

void AResource::FreeResource()
{
    bPlayer1 = false;
    bPlayer2 = false;
    bTaked1 = false;
    bTaked2 = false;
    APlayer1 = nullptr;
    APlayer2 = nullptr;

    ASpawner* OwnerSpawner = Cast<ASpawner>(GetOwner());
    if (IsValid(OwnerSpawner))
    {
        OwnerSpawner->PushFreeResource(this);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s]: FreeResource: Owner spawner missing."), *GetName());
    }
}
