// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner.h"
#include "Pasive.h"
#include "Agresive.h"
#include "StrategyPlayer.h"
#include "Resource.h"
#include "Engine/World.h"

void ASpawner::PushFreeResource(AResource* Resource)
{
    if (!Resource)
    {
        return;
    }

    // Use actor location as fallback if SpawnerCenterLocation wasn't configured
    const FVector BaseLocation = SpawnerCenterLocation.IsZero() ? GetActorLocation() : SpawnerCenterLocation;

    // Store resource in the free pool
    SFreeResources.Push(Resource);

    // Place resource within spawn area
    Resource->SetActorLocation(BaseLocation + FVector(
        FMath::FRandRange(-SpawnAreaSize.X, SpawnAreaSize.X),
        FMath::FRandRange(-SpawnAreaSize.Y, SpawnAreaSize.Y),
        110.0f
    ));

    UE_LOG(LogTemp, Log, TEXT("Resource free [%s]: Resources Freed %d"),
        *Resource->GetName(), SFreeResources.Num());
}
AResource* ASpawner::PopFreeResource()
{
    if (SFreeResources.Num() == 0)
    {
        return nullptr;
    }

    TWeakObjectPtr<AResource> WeakRes = SFreeResources.Pop();
    AResource* Resource = WeakRes.Get();
    if (!IsValid(Resource))
    {
        return nullptr;
    }

    // Claim one player slot on the resource
    Resource->SetPlayer();

    // If resource still has free slots, put it back into pool
    if (Resource->Aviable())
    {
        SFreeResources.Add(Resource);
    }

    return Resource;
}

void ASpawner::PushFreePlayer(AStrategyPlayer* Player)
{
    if (!IsValid(Player))
    {
        return;
    }

    SFreePlayers.Add(Player);
    UE_LOG(LogTemp, Log, TEXT("Player free [%s]: Players Freed %d"),
        *Player->GetName(), SFreePlayers.Num());

    if (bSpawningAgents)
    {
        return;
    }

    const int32 TotalAgents = SAliveAgresives.Num() + SAlivePasives.Num();
    if (TotalAgents <= 0)
    {
        return;
    }

    if (SFreePlayers.Num() < TotalAgents)
    {
        return;
    }

    _sleep(1.0f);

    bSpawningAgents = true;

    TArray<AStrategyPlayer*> PlayersToAssign = SFreePlayers;
    SFreePlayers.Empty();

    for (AStrategyPlayer* FreePlayer : PlayersToAssign)
    {
        if (!IsValid(FreePlayer))
        {
            continue;
        }

        FreePlayer->InitializeAgent(this);
    }

    bSpawningAgents = false;
}

ASpawner::ASpawner()
{
    // Inicializamos variables con valores por defecto seguros
    InitialAgresivePlayers = 5;
    InitialPasivePlayers = 5;
    InitialResourcePlayers = 5;
    SpawnAreaSize = FVector(500.0f, 500.0f, 100.0f);
    SpawnerCenterLocation = FVector::ZeroVector;

}

void ASpawner::BeginPlay()
{
    Super::BeginPlay();

    this->SpawnAgents();
}

void ASpawner::SpawnAgents()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // Si no has configurado SpawnerCenterLocation, usamos la posición del Actor en el mapa
    FVector BaseLocation = SpawnerCenterLocation.IsZero() ? GetActorLocation() : SpawnerCenterLocation;
    FRotator SpawnRotation = FRotator::ZeroRotator;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (int32 x = 0; x < InitialResourcePlayers; x++)
    {
        // Validamos que tengamos una clase válida asignada antes de spawnear
        if (ResourceClass)
        {
            FVector SpawnLocation = BaseLocation + FVector(
                FMath::FRandRange(-SpawnAreaSize.X, SpawnAreaSize.X),
                FMath::FRandRange(-SpawnAreaSize.Y, SpawnAreaSize.Y),
                110.0f
            );

            AResource* NewResource = World->SpawnActor<AResource>(ResourceClass, SpawnLocation, SpawnRotation, SpawnParams);

            if (NewResource)
            {
                this->SFreeResources.Push(NewResource);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s at %s"),
                    *GetName(), *NewResource->GetName(), *SpawnLocation.ToString());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: No se ha asignado una PasiveClass en el Blueprint."), *GetName());
            break;
        }
    }
    int32 ITotalAgents = InitialAgresivePlayers + InitialPasivePlayers;
    int32 ISpawnedAgents = 0;
    int32 ICurrentAgresives = 0;
    int32 ICurrentPasives = 0;
    bool bSpawningAgresives = true;
    while (ISpawnedAgents < ITotalAgents)
    {
        FVector SpawnLocation = BaseLocation + FVector(
            FMath::FRandRange(-SpawnAreaSize.X * 2, SpawnAreaSize.X * 2),
            FMath::FRandRange(-SpawnAreaSize.Y * 2, SpawnAreaSize.Y * 2),
            FMath::FRandRange(50.0f, 50.0f + SpawnAreaSize.Z)
        );
        if (bSpawningAgresives)
        {
            AAgresive* NewAgresive = World->SpawnActor<AAgresive>(AgresiveClass, SpawnLocation, SpawnRotation, SpawnParams);

            if (NewAgresive)
            {
                NewAgresive->InitializeAgent(this);
                this->SAliveAgresives.Push(NewAgresive);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s at %s"),
                    *GetName(), *NewAgresive->GetName(), *SpawnLocation.ToString());
                bSpawningAgresives = false;
                ICurrentAgresives++;
            }
            else {
                continue;
            }
        }
        else
        {
            APasive* NewPasive = World->SpawnActor<APasive>(PasiveClass, SpawnLocation, SpawnRotation, SpawnParams);

            if (NewPasive)
            {
                NewPasive->InitializeAgent(this);
                this->SAlivePasives.Push(NewPasive);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s at %s"),
                    *GetName(), *NewPasive->GetName(), *SpawnLocation.ToString());
                bSpawningAgresives = true;
                ICurrentPasives++;
            }
            else {
                continue;
            }
        }
        ISpawnedAgents++;
    }
}
