#include "StrategyPlayer.h"
#include "Spawner.h"
#include "Resource.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BillboardComponent.h" // Required for UBillboardComponent

AStrategyPlayer::AStrategyPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AStrategyPlayer::InitializeAgent(AResource* Target)
{

	TargetResource = Target;
	if (TargetResource)
	{
		bHasTarget = true;
	}
}

void AStrategyPlayer::PlayInteraction(bool isAgresive)
{
	if (!UwSameInteraction || !UwDistinctInteraction)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s]: PlayInteraction: One of the interaction billboards is null. Verify initialization in BeginPlay."), *GetName());
		return;
	}

	SetActorLocation(MeshComponent->GetComponentLocation());

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
			{
				const FVector CameraRight = CameraManager->GetActorRightVector();
				const FVector CameraUp = CameraManager->GetActorUpVector();

				// Ajusta estas magnitudes en unidades Unreal
				const float OffsetRight = 45.0f;
				const float OffsetUp = 120.0f;

				const FVector TargetLocation = MeshComponent->GetComponentLocation()
					+ (CameraRight * OffsetRight)
					+ (CameraUp * OffsetUp);

				this->UwDistinctInteraction->SetWorldLocation(TargetLocation);
				this->UwSameInteraction->SetWorldLocation(TargetLocation);
			}
		}
	}
	// Changing the actor's location here is not directly related to billboard visibility and could cause unintended movement.
	this->UwDeathInteraction->SetHiddenInGame(true); // Hide the death interaction billboard
	if (bIsAgresive == isAgresive)
	{
		this->UwSameInteraction->SetHiddenInGame(false); // Show the 'same' interaction billboard
	}
	else {
		this->UwDistinctInteraction->SetHiddenInGame(false); // Show the 'distinct' interaction billboard
	}
}

void AStrategyPlayer::StopInteraction()
{
	if (!UwDistinctInteraction || !UwSameInteraction || !UwDeathInteraction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: StopInteraction: One of the interaction billboards is null. Cannot hide."), *GetName());
		return;
	}
	this->UwDistinctInteraction->SetHiddenInGame(true); // Hide the distinct interaction billboard
	this->UwSameInteraction->SetHiddenInGame(true); // Hide the same interaction billboard
	this->UwDeathInteraction->SetHiddenInGame(true); // Hide the death interaction billboard
}

void AStrategyPlayer::KillPlayer()
{
	if (!UwDeathInteraction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: KillPlayer: Death interaction billboard is null. Cannot show."), *GetName());
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
			{
				const FVector CameraRight = CameraManager->GetActorRightVector();
				const FVector CameraUp = CameraManager->GetActorUpVector();

				// Ajusta estas magnitudes en unidades Unreal
				const float OffsetRight = 45.0f;
				const float OffsetUp = 120.0f;

				const FVector TargetLocation = MeshComponent->GetComponentLocation()
					+ (CameraRight * OffsetRight)
					+ (CameraUp * OffsetUp);

				this->UwDeathInteraction->SetWorldLocation(TargetLocation);
				
			}
		}
	}
	SetActorLocation(MeshComponent->GetComponentLocation());
	this->UwDeathInteraction->SetHiddenInGame(false); // Show the death interaction billboard
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

	UBillboardComponent* DeathInteractionBillboardComponent = FindComponentByTag<UBillboardComponent>(FName("DeathInteraction"));
	if (DeathInteractionBillboardComponent)
	{
		UwDeathInteraction = DeathInteractionBillboardComponent;
		if (UwDeathInteraction)
		{
			UwDeathInteraction->SetHiddenInGame(true); // Hide initially
		}
		UE_LOG(LogTemp, Log, TEXT("[%s]: DeathInteractionBillboardComponent found. UwDeathInteraction = %s"), *GetName(), UwDeathInteraction ? *UwDeathInteraction->GetName() : TEXT("nullptr"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[%s]: DeathInteractionBillboardComponent with tag 'DeathInteraction' NOT FOUND."), *GetName());
	}
}

void AStrategyPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	if (!TargetResource || !MeshComponent || !bHasTarget) 
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