// Fill out your copyright notice in the Description page of Project Settings.


#include "StrategyPlayer.h"
#include "Spawner.h"
#include "Resource.h"

// Sets default values
AStrategyPlayer::AStrategyPlayer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

bool AStrategyPlayer::InitializeAgent()
{
	ASpawner* InSpawner = Cast<ASpawner>(GetOwner());
	if (!IsValid(InSpawner))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: InitializeAgent: Owner is not a valid ASpawner."), *GetName());
		return false;
	}

	TargetResource = InSpawner->PopFreeResource(this);
	if (TargetResource)
	{
		bHasTarget = true;
		UE_LOG(LogTemp, Log, TEXT("[%s]: Assigned target resource [%s]."), *GetName(), *TargetResource->GetName());
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: No available resources from spawner."), *GetName());
		return false;
	}
}

void AStrategyPlayer::UpdateMeshLocation()
{
	UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>();
	if (Mesh)
	{
		FVector ActorLocation = GetActorLocation();
		Mesh->SetWorldLocation(ActorLocation);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: UpdateMeshLocation: No static mesh component found."), *GetName());
	}
}

// Called when the game starts or when spawned
void AStrategyPlayer::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AStrategyPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!TargetResource)
	{
		
		return;
	}

	if (!bHasTarget)
	{
		UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>();
		if (Mesh->IsSimulatingPhysics())
		{
			Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		return;
	}

	UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>();
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: No static mesh component found."), *GetName());
		return;
	}

	FVector CurrentPhysicalLocation = Mesh->GetComponentLocation();
	this->SetActorLocation(CurrentPhysicalLocation); // Sync actor location with physics component
	FVector GoalLocation = TargetResource->GetActorLocation();
	const float Distance = FVector::Dist(CurrentPhysicalLocation, GoalLocation);
	const float StopDistance = 150.0f;
	if (Distance <= StopDistance)
	{
		if (Mesh->IsSimulatingPhysics())
		{
			Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		TargetResource->SetTaked(this);
		bHasTarget = false;
		return;
	}

	FVector Direction = (GoalLocation - CurrentPhysicalLocation).GetSafeNormal();
	const float Speed = 500.0f * ASpeed; // consider UPROPERTY(EditAnywhere) float MoveSpeed;
	FVector TargetVelocity = Direction * Speed;
	TargetVelocity.Z = Mesh->GetComponentVelocity().Z;

	if (!Mesh->IsSimulatingPhysics())
	{
		Mesh->SetSimulatePhysics(true);
	}

	// Prefer AddForce/AddImpulse for natural movement, otherwise SetPhysicsLinearVelocity
	Mesh->SetPhysicsLinearVelocity(TargetVelocity);
	// Do NOT call SetActorLocation; let physics drive the actor (or make Mesh the root component)
}

