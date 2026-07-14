// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner.h"
#include "Pasive.h"
#include "Agresive.h"
#include "StrategyPlayer.h"
#include "Resource.h"
#include "PlayerProccessor.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Containers/UnrealString.h" // For FString operations, usually in CoreMinimal.h

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
		Resource->SetActorLocation(SpawnLocation);
        if (Mesh)
        {
            Mesh->SetWorldLocation(SpawnLocation);
            Mesh->SetSimulatePhysics(false);
        }
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

    QFreePlayers.Enqueue(Player);
	this->CurrentFreePlayers++;
    UE_LOG(LogTemp, Log, TEXT("Player free [%s]: Players Freed %d"),
        *Player->GetName(), this->CurrentFreePlayers);

    if (bSpawningAgents)
    {
        return;
    }

    const int32 TotalAgents = SAliveAgresives.Num() + SAlivePasives.Num();
    if (TotalAgents <= 0)
    {
        return;
    }

    if (this->CurrentFreePlayers<TotalAgents)
    {
        return;
    }

    // Schedule a non-blocking delayed call instead of blocking the game thread
    if (UWorld* World = GetWorld())
    {
        // Avoid scheduling multiple timers if one is already active
        if (!World->GetTimerManager().IsTimerActive(SpawnPlayersTimerHandle))
        {
            World->GetTimerManager().SetTimer(SpawnPlayersTimerHandle, this, &ASpawner::AssignFreePlayers, 2.0f, false);
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
	TArray <AStrategyPlayer*> PlayersToInitialize;
    while(!QFreePlayers.IsEmpty())
    {
        AStrategyPlayer* FreePlayer = nullptr;
        if (QFreePlayers.Dequeue(FreePlayer))
        {
            if (!IsValid(FreePlayer))
            {
                continue;
            }
            if (!FreePlayer->InitializeAgent(this)) {
				PlayersToInitialize.Add(FreePlayer);
            }
            else {
                this->CurrentFreePlayers--;
            }
        }
	}
    for (AStrategyPlayer* Player : PlayersToInitialize)
    {
        if (IsValid(Player))
        {
            QFreePlayers.Enqueue(Player);
        }
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
    if (!APlayerProcessor)
    {
        APlayerProcessor = GetWorld()->SpawnActor<APlayerProccessor>();
        if (APlayerProcessor)
        {
            APlayerProcessor->Initialize();
            InitialAgresivePlayers = APlayerProcessor->GetInitialAgresivePlayers();
            InitialPasivePlayers = APlayerProcessor->GetInitialPasivePlayers();
		}
    }
    this->SpawnAgents();
}

void ASpawner::SpawnAgents()
{
    UE_LOG(LogTemp, Log, TEXT("initial agresive: %d, initial pasive: %d"), InitialAgresivePlayers, InitialPasivePlayers);
    UWorld* World = GetWorld();
    if (!World) return;

    // Si no has configurado SpawnerCenterLocation, usamos la posición del Actor en el mapa
    FVector BaseLocation = SpawnerCenterLocation.IsZero() ? GetActorLocation() : SpawnerCenterLocation;
    FRotator SpawnRotation = FRotator::ZeroRotator;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    InitialResourcePlayers = FMath::Max((InitialAgresivePlayers + InitialPasivePlayers) / 2, 1); // Aseguramos al menos un recurso

    for (int32 x = 0; x < InitialResourcePlayers; x++)
    {
        // Validamos que tengamos una clase válida asignada antes de spawnear
        if (ResourceClass)
        {
			AResource* NewResource = World->SpawnActor<AResource>(ResourceClass, BaseLocation, SpawnRotation, SpawnParams);
            if (NewResource)
            {
                this->SFreeResources.Push(NewResource);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s"),
                    *GetName(), *NewResource->GetName());
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
    int32 IMaxAgresives = InitialAgresivePlayers;
    int32 IMaxPasives = InitialPasivePlayers;
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
            if (IMaxAgresives == 0)
            {
                bSpawningAgresives = false;
                continue;
            }
            AAgresive* NewAgresive = World->SpawnActor<AAgresive>(AgresiveClass, SpawnLocation, SpawnRotation, SpawnParams);

            if (NewAgresive)
            {
				NewAgresive->SetAggresive(true);
                this->SAliveAgresives.Push(NewAgresive);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s at %s"),
                    *GetName(), *NewAgresive->GetName(), *SpawnLocation.ToString());
                bSpawningAgresives = false;
                IMaxAgresives--;
            }
            else {
                continue;
            }
        }
        else
        {
            if (IMaxPasives == 0)
            {
                bSpawningAgresives = true;
                continue;
            }

            APasive* NewPasive = World->SpawnActor<APasive>(PasiveClass, SpawnLocation, SpawnRotation, SpawnParams);

            if (NewPasive)
            {
				NewPasive->SetAggresive(false);
                this->SAlivePasives.Push(NewPasive);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s at %s"),
                    *GetName(), *NewPasive->GetName(), *SpawnLocation.ToString());
                bSpawningAgresives = true;
                IMaxPasives--;
            }
            else {
                continue;
            }
        }
        ISpawnedAgents++;
    }

    for (APasive* Pasive : SAlivePasives)
    {
		this->PushFreePlayer(Pasive);
	}

    for (AAgresive* Agresive : SAliveAgresives)
    {
		this->PushFreePlayer(Agresive);
    }

}
