#include "StrategyPlayer.h"
#include "Spawner.h"
#include "Resource.h"
#include "Components/StaticMeshComponent.h"

AStrategyPlayer::AStrategyPlayer()
{
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
		return true;
	}

	return false;
}

void AStrategyPlayer::BeginPlay()
{
	Super::BeginPlay();
	MeshComponent = FindComponentByClass<UStaticMeshComponent>();
}

void AStrategyPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!TargetResource || !MeshComponent)
	{
		return;
	}

	if (!bHasTarget)
	{
		if (MeshComponent->IsSimulatingPhysics())
		{
			MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		return;
	}

	FVector CurrentPhysicalLocation = MeshComponent->GetComponentLocation();
	FVector GoalLocation = TargetResource->GetActorLocation();

	const float Distance = FVector::Dist(CurrentPhysicalLocation, GoalLocation);
	const float StopDistance = 150.0f;

	if (Distance <= StopDistance)
	{
		if (MeshComponent->IsSimulatingPhysics())
		{
			MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		TargetResource->SetTaked(this);
		bHasTarget = false;
		return;
	}

	FVector Direction = (GoalLocation - CurrentPhysicalLocation).GetSafeNormal();
	const float CurrentSpeed = 500.0f * ASpeed;
	FVector TargetVelocity = Direction * CurrentSpeed;
	TargetVelocity.Z = MeshComponent->GetComponentVelocity().Z; // Mantener gravedad

	if (!MeshComponent->IsSimulatingPhysics())
	{
		MeshComponent->SetSimulatePhysics(true);
	}

	MeshComponent->SetPhysicsLinearVelocity(TargetVelocity);
}