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
AResource* ASpawner::PopFreeResource(AStrategyPlayer* Player)
{
    if (SActiveResources.Num() == 0)
    {
        return nullptr;
    }

    TWeakObjectPtr<AResource> WeakRes = SActiveResources.Pop();
    AResource* Resource = WeakRes.Get();
    if (!IsValid(Resource))
    {
        return nullptr;
    }

    // Claim one player slot on the resource
    Resource->SetAsigned(Player);
    // If resource still has free slots, put it back into pool
    if (Resource->Aviable())
    {
        UStaticMeshComponent* Mesh = Resource->FindComponentByClass<UStaticMeshComponent>();
        FVector BaseLocation = SpawnerCenterLocation.IsZero() ? GetActorLocation() : SpawnerCenterLocation;
        FVector SpawnLocation = BaseLocation + FVector(
            FMath::FRandRange(-SpawnAreaSize.X, SpawnAreaSize.X),
            FMath::FRandRange(-SpawnAreaSize.Y, SpawnAreaSize.Y),
            150.0f
        );        
		Resource->SetActorLocation(SpawnLocation);
        if (Mesh)
        {
            Mesh->SetWorldLocation(SpawnLocation);
            Mesh->SetSimulatePhysics(false);
        }
        SActiveResources.Add(Resource);
    }
    else {
		this->ProcessPlayers(Player, Resource->APlayer1);
    }

    return Resource;
}

void ASpawner::PushFreePlayer(AStrategyPlayer* Player)
{
    if (!IsValid(Player))
    {
        return;
    }

    this->SActivePlayers.Pop();
	APlayerProcessor->ProcessPlayer(Player);

    UE_LOG(LogTemp, Log, TEXT("Player free [%s]: Active Players %d"),
        *Player->GetName(), SActivePlayers.Num());

    if (!this->SActivePlayers.IsEmpty())
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

void ASpawner::PushWaitingPlayer(AStrategyPlayer* Player)
{
    if (!IsValid(Player))
    {
        return;
    }
    this->QWaitingPlayers.Enqueue(Player);
	this->CurrentWaitingPlayers++;
}

void ASpawner::AssignFreePlayers()
{
    if (bSpawningAgents)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    bSpawningAgents = true;
	UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Assigning free players. Current waiting players: %d"), *GetName(), CurrentWaitingPlayers);
	UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Free resources available: %d\n\n"), *GetName(), SFreeResources.Num());
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    FVector BaseLocation = SpawnerCenterLocation.IsZero() ? GetActorLocation() : SpawnerCenterLocation;
    FRotator SpawnRotation = FRotator::ZeroRotator;

    int32 ResourcesNeed = this->CurrentWaitingPlayers / 2;
    if (ResourcesNeed > SFreeResources.Num())
    {
		UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Not enough free resources. Need: %d, Available: %d"), *GetName(), ResourcesNeed, SFreeResources.Num());
        if (!ResourceClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: ResourceClass is not set, cannot spawn resources."), *GetName());
        }
        else
        {
            int32 ToSpawn = ResourcesNeed - SFreeResources.Num();
            for (int32 i = 0; i < ToSpawn; ++i)
            {
                AResource* NewResource = World->SpawnActor<AResource>(ResourceClass, BaseLocation, SpawnRotation, SpawnParams);
                if (NewResource)
                {
                    this->SFreeResources.Push(NewResource);
					NewResource->SetActorHiddenInGame(true);
                    UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s"), *GetName(), *NewResource->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: Failed to spawn Resource (attempt %d of %d)"), *GetName(), i + 1, ToSpawn);
                }
            }
        }
    }
	int32 TotalResources = SFreeResources.Num();
    for (int32 i = 0; i < TotalResources; i++)
    {
        AResource* Resource = this->SFreeResources.Pop();
        if (i < ResourcesNeed)
        {
            this->SActiveResources.Push(Resource);
            Resource->SetActorHiddenInGame(false);
        }
        else {
            this->SFreeResources.Push(Resource);
            Resource->SetActorHiddenInGame(true);
        }
        
    }


    for (AAgresive* Agresive : SDeadAgresives)
    {
        Agresive->SetActorHiddenInGame(true);
    }
    for (APasive* Pasive : SDeadPasives)
    {
        Pasive->SetActorHiddenInGame(true);
    }

	TArray<AStrategyPlayer*> PlayersToWait;
	AStrategyPlayer* Player;
    while (this->QWaitingPlayers.Dequeue(Player))
    {
		Player->SetActorHiddenInGame(false);
        this->CurrentWaitingPlayers--;
        if (Player->InitializeAgent())
        {
			this->SActivePlayers.Add(Player);
        }
        else {
			PlayersToWait.Add(Player);
        }
    }

    for (AStrategyPlayer* PlayerToWait : PlayersToWait)
    {
        this->QWaitingPlayers.Enqueue(PlayerToWait);
        this->CurrentWaitingPlayers++;
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
			APlayerProcessor->SetOwner(this);
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
                NewResource->SetActorHiddenInGame(true);
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
				this->QWaitingPlayers.Enqueue(NewAgresive);
                this->CurrentWaitingPlayers++;
				this->CurrentAgresivePlayers++;
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
				this->QWaitingPlayers.Enqueue(NewPasive);
				this->CurrentWaitingPlayers++;
				this->CurrentPasivePlayers++;
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
	this->AssignFreePlayers();

}

void ASpawner::ProcessPlayers(AStrategyPlayer* Player1, AStrategyPlayer* Player2)
{
    if (APlayerProcessor)
    {
        APlayerProcessor->ProcessPlayers(Player1, Player2);
    }
}

void ASpawner::SpawnPlayer(AStrategyPlayer* Player)
{
    if (!IsValid(Player))
    {
        return;
    }

    // Get player base location and bounds
    FVector PlayerLocation = Player->GetActorLocation();
    FVector Origin = FVector::ZeroVector;
    FVector Extent = FVector::ZeroVector;
    Player->GetActorBounds(true, Origin, Extent); // true = only colliding components

    // Determine radius to spawn near the player (use extents as a base)
    const float Buffer = 100.0f;
    float Radius = FMath::Max(Extent.X, Extent.Y) + Buffer;
    if (Radius <= 0.0f)
    {
        Radius = 20.0f; // safe fallback
    }

    // Random offset within radius on X/Y, keep Z slightly above player
    FVector SpawnLocation = PlayerLocation + FVector(
        FMath::FRandRange(-Radius, Radius),
        FMath::FRandRange(-Radius, Radius),
        110.0f
    );

    FRotator SpawnRotation = FRotator::ZeroRotator;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    UWorld* World = GetWorld();
    if (!World) return;

    if (Player->IsAggresive())
    {
        this->CurrentAgresivePlayers++;
        if (SDeadAgresives.Num() > 0)
        {
			AAgresive* ReusedAgresive = SDeadAgresives.Pop();
			ReusedAgresive->SetActorHiddenInGame(false);
			this->PushWaitingPlayer(ReusedAgresive);
        }
        else
        {
            AAgresive* NewAgresive = World->SpawnActor<AAgresive>(AgresiveClass, SpawnLocation, SpawnRotation, SpawnParams);
            if (NewAgresive)
            {
                NewAgresive->SetAggresive(true);
				this->PushWaitingPlayer(NewAgresive);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s at %s"),
                    *GetName(), *NewAgresive->GetName(), *SpawnLocation.ToString());
            }
        }
		
    }
    else
    {
        this->CurrentPasivePlayers++;
        // Non-aggressive spawn logic: reuse dead pasives if available, otherwise spawn new Pasive.
        if (SDeadPasives.Num() > 0)
        {
			APasive* ReusedPasive = SDeadPasives.Pop();
			ReusedPasive->SetActorHiddenInGame(false);
			this->PushWaitingPlayer(ReusedPasive);
        }
        else 
        {
            APasive* NewPasive = World->SpawnActor<APasive>(PasiveClass, SpawnLocation, SpawnRotation, SpawnParams);
            if (NewPasive)
            {
                NewPasive->SetAggresive(false);
				this->PushWaitingPlayer(NewPasive);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s at %s"),
                    *GetName(), *NewPasive->GetName(), *SpawnLocation.ToString());
            }
        }
		
    }
}

void ASpawner::KillPlayer(AStrategyPlayer* Player)
{
	UE_LOG(LogTemp, Error, TEXT("Spawner [%s]: Killing player %s"), *GetName(), *Player->GetName());
	
    if (Player->IsAggresive())
    {
		this->CurrentAgresivePlayers--;
		this->SDeadAgresives.Add(Cast<AAgresive>(Player));
    }
    else
    {
		this->CurrentPasivePlayers--;
		this->SDeadPasives.Add(Cast<APasive>(Player));
    }
}
