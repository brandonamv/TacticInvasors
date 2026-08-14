#include "StrategyPlayer.h"
#include "Spawner.h"
#include "Resource.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BillboardComponent.h" // Required for UBillboardComponent

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

void AStrategyPlayer::PlayInteraction(bool isAgresive)
{
	if (!UwSameInteraction || !UwDistinctInteraction)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s]: PlayInteraction: One of the interaction billboards is null. Verify initialization in BeginPlay."), *GetName());
		return;
	}

	SetActorLocation(MeshComponent->GetComponentLocation());
	// Changing the actor's location here is not directly related to billboard visibility and could cause unintended movement.

	if (bIsAgresive == isAgresive)
	{
		UwSameInteraction->SetHiddenInGame(false); // Show the 'same' interaction billboard
		UwDistinctInteraction->SetHiddenInGame(true); // Hide the 'distinct' interaction billboard
	}
	else {
		UwDistinctInteraction->SetHiddenInGame(false); // Show the 'distinct' interaction billboard
		UwSameInteraction->SetHiddenInGame(true); // Hide the 'same' interaction billboard
	}
}

void AStrategyPlayer::StopInteraction()
{
	if (!UwDistinctInteraction || !UwSameInteraction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: StopInteraction: One of the interaction billboards is null. Cannot hide."), *GetName());
		return;
	}
	UwDistinctInteraction->SetHiddenInGame(true); // Hide the distinct interaction billboard
	UwSameInteraction->SetHiddenInGame(true); // Hide the same interaction billboard
}

void AStrategyPlayer::BeginPlay()
{
	Super::BeginPlay();
	MeshComponent = FindComponentByClass<UStaticMeshComponent>();
	
	// Buscar y asignar UwDistinctInteraction como UBillboardComponent
	UBillboardComponent* DistinctInteractionBillboardComponent = FindComponentByTag<UBillboardComponent>(FName("DistinctInteraction"));
	if (DistinctInteractionBillboardComponent)
	{
		UwDistinctInteraction = DistinctInteractionBillboardComponent;
		if (UwDistinctInteraction)
		{
			UwDistinctInteraction->SetHiddenInGame(true); // Hide initially
		}
		UE_LOG(LogTemp, Log, TEXT("[%s]: DistinctInteractionBillboardComponent found. UwDistinctInteraction = %s"), *GetName(), UwDistinctInteraction ? *UwDistinctInteraction->GetName() : TEXT("nullptr"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[%s]: DistinctInteractionBillboardComponent with tag 'DistinctInteraction' NOT FOUND."), *GetName());
	}

	// Buscar y asignar UwSameInteraction como UBillboardComponent
	UBillboardComponent* SameInteractionBillboardComponent = FindComponentByTag<UBillboardComponent>(FName("SameInteraction"));
	if (SameInteractionBillboardComponent)
	{
		UwSameInteraction = SameInteractionBillboardComponent;
		if (UwSameInteraction)
		{
			UwSameInteraction->SetHiddenInGame(true); // Hide initially
		}
		UE_LOG(LogTemp, Log, TEXT("[%s]: SameInteractionBillboardComponent found. UwSameInteraction = %s"), *GetName(), UwSameInteraction ? *UwSameInteraction->GetName() : TEXT("nullptr"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[%s]: SameInteractionBillboardComponent with tag 'SameInteraction' NOT FOUND."), *GetName());
	}
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
	const float StopDistance = 250.0f;

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