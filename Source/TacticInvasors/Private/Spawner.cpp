// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner.h"
#include "Pasive.h"
#include "Agresive.h"
#include "StrategyPlayer.h"
#include "Resource.h"
#include "Engine/World.h"
#include "TimerManager.h" // <- needed for timers

void ASpawner::PushFreeResource(AResource* Resource)
{
    if (!Resource)
    {
        return;
    }

    // Store resource in the free pool
    SFreeResources.Push(Resource);

    UStaticMeshComponent* Mesh = Resource->FindComponentByClass<UStaticMeshComponent>();
    if (Mesh)
        Mesh->SetSimulatePhysics(true);

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
        UStaticMeshComponent* Mesh = Resource->FindComponentByClass<UStaticMeshComponent>();
        FVector BaseLocation = SpawnerCenterLocation.IsZero() ? GetActorLocation() : SpawnerCenterLocation;
        FVector SpawnLocation = BaseLocation + FVector(
            FMath::FRandRange(-SpawnAreaSize.X, SpawnAreaSize.X),
            FMath::FRandRange(-SpawnAreaSize.Y, SpawnAreaSize.Y),
            110.0f
        );
        if (Mesh)
        {
			Mesh->SetWorldLocation(SpawnLocation);
            Mesh->SetSimulatePhysics(false);
        }
        
		Resource->SetActorLocation(SpawnLocation);
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

    // Schedule a non-blocking delayed call instead of blocking the game thread
    if (UWorld* World = GetWorld())
    {
        // Avoid scheduling multiple timers if one is already active
        if (!World->GetTimerManager().IsTimerActive(SpawnPlayersTimerHandle))
        {
            World->GetTimerManager().SetTimer(SpawnPlayersTimerHandle, this, &ASpawner::AssignFreePlayers, 5.0f, false);
        }
    }
}

void ASpawner::AssignFreePlayers()
{
    if (bSpawningAgents)
    {
        return;
    }

    bSpawningAgents = true;

    // Move the free players into a local array so we can operate without holding onto the original container
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
