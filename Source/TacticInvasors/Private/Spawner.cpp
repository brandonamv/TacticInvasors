// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner.h"
#include "Pasive.h"
#include "Agresive.h"
#include "StrategyPlayer.h"
#include "Resource.h"
#include "PlayerProccessor.h"

#include "Engine/World.h"
#include "TimerManager.h"

// Move ShuffleArray implementation into the .cpp file to avoid parsing issues
void ASpawner::ShuffleArray(TArray<AStrategyPlayer*>& TargetArray)
{
	const int32 Count = TargetArray.Num();
	if (Count <= 1)
	{
		return;
	}

	// Fisher–Yates shuffle
	for (int32 i = 0; i < Count - 1; ++i)
	{
		const int32 j = FMath::RandRange(i, Count - 1);
		if (i != j)
		{
			if (TargetArray.IsValidIndex(i) && TargetArray.IsValidIndex(j))
			{
				// Safely access elements (ensure forward-declared AStrategyPlayer methods are available by including StrategyPlayer.h)
				if (AStrategyPlayer* PlayerAtI = TargetArray[i])
				{
					if (APlayerProcessor)
					{
						APlayerProcessor->DailyPenalty(PlayerAtI);
					}
				}
				TargetArray.Swap(i, j);
			}
		}
	}
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
			MaxIterations = APlayerProcessor->GetMaxIteractions();
			Speed = APlayerProcessor->GetSpeed();
		}
	}
	this->SpawnAgents();
}

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
}

AResource* ASpawner::PopFreeResource()
{
	// Pop from active list (use Last element semantics) but keep consistent types
	if (SActiveResources.Num() == 0)
	{
		return nullptr;
	}

	// Pop the last active resource
	AResource* Resource = SActiveResources.Pop();
	if (!IsValid(Resource))
	{
		UE_LOG(LogTemp, Warning, TEXT("PopFreeResource: popped invalid resource."));
		return nullptr;
	}

	UStaticMeshComponent* Mesh = Resource->GetMesh();
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

	return Resource;
}

void ASpawner::PushFreePlayer(AStrategyPlayer* Player)
{
	if (!IsValid(Player))
	{
		return;
	}

	SActivePlayers.Pop();

	if (!this->SActivePlayers.IsEmpty())
	{
		return;
	}

	// Schedule a non-blocking delayed call instead of blocking the game thread
	if (UWorld* World = GetWorld())
	{
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
	this->SFreePlayers.Add(Player);
}

void ASpawner::AssignFreePlayers()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (CurrentIteration >= MaxIterations)
	{
		return;
	}
	else
	{
		CurrentIteration++;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	FVector BaseLocation = SpawnerCenterLocation.IsZero() ? GetActorLocation() : SpawnerCenterLocation;
	FRotator SpawnRotation = FRotator::ZeroRotator;


	int32 ResourcesNeed = 0;

	if (APlayerProcessor)
	{
		if (APlayerProcessor->GetResourceFillingType() != 0) {
			ResourcesNeed = CurrentIteration == 0 ? InitialResourcePlayers : APlayerProcessor->GetResourceIncrement(CurrentIteration, SFreePlayers.Num());
		}
		else {
			ResourcesNeed = InitialResourcePlayers;
		}
	}

	// Ensure we have at least ResourcesNeed resources: spawn missing ones if class available
	if (ResourcesNeed > SFreeResources.Num())
	{
		if (!ResourceClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: ResourceClass is not set, cannot spawn resources."), *GetName());
		}
		else
		{
			const int32 ToSpawn = ResourcesNeed - SFreeResources.Num();
			SFreeResources.Reserve(SFreeResources.Num() + ToSpawn);
			for (int32 i = 0; i < ToSpawn; ++i)
			{
				AResource* NewResource = World->SpawnActor<AResource>(ResourceClass, BaseLocation, SpawnRotation, SpawnParams);
				if (NewResource)
				{
					NewResource->SetActorHiddenInGame(true);
					SFreeResources.Push(NewResource);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: Failed to spawn Resource (attempt %d of %d)"), *GetName(), i + 1, ToSpawn);
				}
			}
		}
	}

	// Activate exactly the number of resources we need (or as many as available)
	const int32 ActivateCount = FMath::Min(ResourcesNeed, SFreeResources.Num());
	for (int32 i = 0; i < ActivateCount; ++i)
	{
		AResource* Resource = SFreeResources.Pop();
		if (IsValid(Resource))
		{
			Resource->SetActorHiddenInGame(false);
			SActiveResources.Push(Resource);
		}
	}

	// Ensure remaining free resources are hidden
	for (AResource* Remaining : SFreeResources)
	{
		if (IsValid(Remaining))
		{
			Remaining->SetActorHiddenInGame(true);
		}
	}

	// Hide dead players (reuse list elements safely)
	for (AAgresive* DeadA : SDeadAgresives)
	{
		if (IsValid(DeadA)) DeadA->SetActorHiddenInGame(true);
	}
	for (APasive* DeadP : SDeadPasives)
	{
		if (IsValid(DeadP)) DeadP->SetActorHiddenInGame(true);
	}


	// Shuffle free players and initialize them: move initialized ones to active, keep others waiting
	ShuffleArray(SFreePlayers);
	TArray<AStrategyPlayer*> PlayersToWait;
	PlayersToWait.Reserve(SFreePlayers.Num());
	while (!SFreePlayers.IsEmpty())
	{
		AStrategyPlayer* Player1 = SFreePlayers.Pop();
		AStrategyPlayer* Player2 = SFreePlayers.IsEmpty() ? nullptr : SFreePlayers.Pop();
		Player1->StopInteraction();
		if (Player2) Player2->StopInteraction();

		AResource* TargetResource = PopFreeResource();
		if (!IsValid(TargetResource))
		{
			// No available resource, push players back to wait
			if (Player1->GetFitness() > 0)
			{
				PlayersToWait.Push(Player1);
			}
			else
			{
				Player1->SetFitness(APlayerProcessor->GetI());
				this->KillPlayer(Player1);
			}
			if (Player2 == nullptr)
			{
				continue;
			}
			if (Player2->GetFitness() > 0)
			{
				PlayersToWait.Push(Player2);
			}
			else
			{
				Player2->SetFitness(APlayerProcessor->GetI());
				this->KillPlayer(Player2);
			}
			continue;
		}

		if (Player2 == nullptr)
		{
			continue;
		}

		if (IsValid(TargetResource))
		{
			TargetResource->SetAsigned(Player1, Player2);
			SActivePlayers.Push(Player1);
			SActivePlayers.Push(Player2);
			ProcessPlayers(Player1, Player2);
		}


	}


	// Return the waiting players to the free list (preserve order not required)
	if (!PlayersToWait.IsEmpty())
	{
		SFreePlayers.Append(PlayersToWait);
	}
}

// Replacement: SpawnAgents method refactor for clearer, safer spawning and less duplication.
void ASpawner::SpawnAgents()
{
	// Plan (pseudocode):
	// 1. Validate World and compute BaseLocation & SpawnRotation.
	// 2. Prepare SpawnParams and compute InitialResourcePlayers (at least 1).
	// 3. Reserve space in SFreeResources to avoid reallocations.
	// 4. Provide a small helper to compute randomized spawn locations.
	// 5. Spawn resources in one block if ResourceClass is valid; log once if missing.
	// 6. Spawn aggressive players in one loop, logging failures but avoiding per-iteration class checks.
	// 7. Spawn passive players in one loop, same pattern.
	// 8. Call AssignFreePlayers() at the end.
	// The goal: reduce duplication, check classes once, keep behavior identical.

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector BaseLocation = SpawnerCenterLocation.IsZero() ? GetActorLocation() : SpawnerCenterLocation;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Ensure at least one resource is available
	InitialResourcePlayers = APlayerProcessor ? APlayerProcessor->GetInitialResources() : 1;
	// Reserve to reduce reallocations
	SFreeResources.Reserve(SFreeResources.Num() + InitialResourcePlayers);

	// Helper: random location generator
	auto RandomPlayerLocation = [&](float AreaMultiplier = 2.0f)->FVector
		{
			return BaseLocation + FVector(
				FMath::FRandRange(-SpawnAreaSize.X * AreaMultiplier, SpawnAreaSize.X * AreaMultiplier),
				FMath::FRandRange(-SpawnAreaSize.Y * AreaMultiplier, SpawnAreaSize.Y * AreaMultiplier),
				FMath::FRandRange(50.0f, 50.0f + SpawnAreaSize.Z)
			);
		};

	// Spawn resources if class provided, log once if not
	if (!ResourceClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: ResourceClass is not set, cannot spawn resources."), *GetName());
	}
	else
	{
		for (int32 i = 0; i < InitialResourcePlayers; ++i)
		{
			AResource* NewResource = World->SpawnActor<AResource>(ResourceClass, BaseLocation, SpawnRotation, SpawnParams);
			if (NewResource)
			{
				SFreeResources.Push(NewResource);
				NewResource->SetActorHiddenInGame(true);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: Failed to spawn Resource (index %d)"), *GetName(), i);
			}
		}
	}

	// Spawn aggressive players
	if (!AgresiveClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: AgresiveClass is not set, skipping aggressive player spawn."), *GetName());
	}
	else
	{
		SFreePlayers.Reserve(SFreePlayers.Num() + InitialAgresivePlayers);
		for (int32 i = 0; i < InitialAgresivePlayers; ++i)
		{
			FVector SpawnLocation = RandomPlayerLocation(2.0f);
			AAgresive* NewAgresive = World->SpawnActor<AAgresive>(AgresiveClass, SpawnLocation, SpawnRotation, SpawnParams);
			if (NewAgresive)
			{
				NewAgresive->SetAggresive(true);
				NewAgresive->SetSpeed(Speed);
				SFreePlayers.Add(NewAgresive);
				NewAgresive->SetFitness(APlayerProcessor->GetI());
				++CurrentAgresivePlayers;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: Failed to spawn AAgresive (index %d)"), *GetName(), i);
			}
		}
	}

	// Spawn passive players
	if (!PasiveClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: PasiveClass is not set, skipping passive player spawn."), *GetName());
	}
	else
	{
		SFreePlayers.Reserve(SFreePlayers.Num() + InitialPasivePlayers);
		for (int32 i = 0; i < InitialPasivePlayers; ++i)
		{
			FVector SpawnLocation = RandomPlayerLocation(2.0f);
			APasive* NewPasive = World->SpawnActor<APasive>(PasiveClass, SpawnLocation, SpawnRotation, SpawnParams);
			if (NewPasive)
			{
				NewPasive->SetAggresive(false);
				NewPasive->SetSpeed(Speed);
				SFreePlayers.Add(NewPasive);
				NewPasive->SetFitness(APlayerProcessor->GetI());
				++CurrentPasivePlayers;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: Failed to spawn APasive (index %d)"), *GetName(), i);
			}
		}
	}

	// Finalize initial population
	this->AssignFreePlayers();
}

void ASpawner::ProcessPlayers(AStrategyPlayer* Player1, AStrategyPlayer* Player2)
{
	if (APlayerProcessor)
	{
		APlayerProcessor->ProcessPlayers(Player1, Player2);
	}
}

void ASpawner::ProcessPlayer(AStrategyPlayer* Player) {
	if (APlayerProcessor)
	{
		APlayerProcessor->ProcessPlayer(Player);
	}
}

void ASpawner::SpawnPlayer(AStrategyPlayer* Player)
{
	if (!IsValid(Player))
	{
		return;
	}

	// Base spawn info
	FVector PlayerLocation = Player->GetMesh()->GetComponentLocation();

	FVector SpawnLocation = PlayerLocation;
	SpawnLocation.Z += 150.0f;
	SpawnLocation.X += 100.0f;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UWorld* World = GetWorld();
	if (!World) return;

	// Helper to handle reuse or spawn for either aggressive or passive players
	auto SpawnOrReuse = [&](bool bAggressive)
		{
			if (bAggressive)
			{
				++CurrentAgresivePlayers;
				// Try reuse dead aggressive
				if (!SDeadAgresives.IsEmpty())
				{
					AAgresive* ReusedAgresive = nullptr;
					for (AAgresive* Candidate : SDeadAgresives)
					{
						if (IsValid(Candidate))
						{
							ReusedAgresive = Candidate;
							break;
						}
					}

					if (ReusedAgresive)
					{
						ReusedAgresive->StopInteraction();
						ReusedAgresive->SetActorHiddenInGame(false);
						ReusedAgresive->GetMesh()->SetWorldLocation(SpawnLocation);
						ReusedAgresive->SetFitness(APlayerProcessor->GetI());
						this->PushWaitingPlayer(ReusedAgresive);
						SDeadAgresives.Remove(ReusedAgresive);
					}

					return;
				}

				// Spawn new aggressive if possible
				if (!AgresiveClass)
				{
					UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: AgresiveClass is not set, cannot spawn aggressive player."), *GetName());
					return;
				}

				AStrategyPlayer* NewPlayer = World->SpawnActor<AStrategyPlayer>(AgresiveClass, SpawnLocation, SpawnRotation, SpawnParams);
				if (NewPlayer)
				{
					NewPlayer->SetAggresive(true);
					NewPlayer->SetSpeed(Speed);
					NewPlayer->SetFitness(APlayerProcessor->GetI());
					this->PushWaitingPlayer(NewPlayer);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: Failed to spawn aggressive player."), *GetName());
				}
			}
			else
			{
				++CurrentPasivePlayers;
				// Try reuse dead passive
				if (!SDeadPasives.IsEmpty())
				{
					APasive* ReusedPasive = nullptr;
					for (APasive* Candidate : SDeadPasives)
					{
						if (IsValid(Candidate))
						{
							ReusedPasive = Candidate;
							break;
						}
					}
					if (ReusedPasive)
					{
						ReusedPasive->StopInteraction();
						ReusedPasive->SetActorHiddenInGame(false);
						ReusedPasive->GetMesh()->SetWorldLocation(SpawnLocation);
						ReusedPasive->SetFitness(APlayerProcessor->GetI());
						this->PushWaitingPlayer(ReusedPasive);
						SDeadPasives.Remove(ReusedPasive);
					}
					return;
				}

				// Spawn new passive if possible
				if (!PasiveClass)
				{
					UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: PasiveClass is not set, cannot spawn passive player."), *GetName());
					return;
				}

				AStrategyPlayer* NewPlayer = World->SpawnActor<AStrategyPlayer>(PasiveClass, SpawnLocation, SpawnRotation, SpawnParams);
				if (NewPlayer)
				{
					NewPlayer->SetAggresive(false);
					NewPlayer->SetSpeed(Speed);
					NewPlayer->SetFitness(APlayerProcessor->GetI());
					this->PushWaitingPlayer(NewPlayer);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: Failed to spawn passive player."), *GetName());
				}
			}
		};

	// Execute helper based on the desired aggression
	SpawnOrReuse(Player->IsAggresive());
}

void ASpawner::KillPlayer(AStrategyPlayer* Player)
{
	if (!IsValid(Player))
	{
		return;
	}

	// Use a weak pointer to avoid dangling references if Player is destroyed before the timer executes.
	TWeakObjectPtr<AStrategyPlayer> WeakPlayer = Player;

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(KillPlayersTimerHandle))
		{
			World->GetTimerManager().SetTimer(KillPlayersTimerHandle, [this, WeakPlayer]()
				{
					// Resolve the weak pointer safely
					if (AStrategyPlayer* ResolvedPlayer = WeakPlayer.Get())
					{
						ResolvedPlayer->StopInteraction();
						ResolvedPlayer->KillPlayer();
					}
				}, 1.0f, false);
		}
	}

	if (Player->IsAggresive())
	{
		// Safely decrement, never below zero
		CurrentAgresivePlayers = FMath::Max(0, CurrentAgresivePlayers - 1);

		// Ensure we only add valid AAgresive instances 
		AAgresive* AsAgg = Cast<AAgresive>(Player);
		if (AsAgg)
		{
			SDeadAgresives.Add(AsAgg);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("KillPlayer: expected AAgresive but cast failed for %s"), *Player->GetName());
		}
	}
	else
	{
		// Safely decrement, never below zero
		CurrentPasivePlayers = FMath::Max(0, CurrentPasivePlayers - 1);

		// Ensure we only add valid APasive instances
		APasive* AsPas = Cast<APasive>(Player);
		if (AsPas)
		{
			SDeadPasives.Add(AsPas);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("KillPlayer: expected APasive but cast failed for %s"), *Player->GetName());
		}
	}
}
