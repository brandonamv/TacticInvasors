#include "StrategyPlayer.h"
#include "Spawner.h"
#include "Resource.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h" // Required for UGameplayStatics::GetPlayerCameraManager

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
		UE_LOG(LogTemp, Error, TEXT("[%s]: PlayInteraction: Uno de los widgets de interacción es nulo. Verifique la inicialización en BeginPlay."), *GetName());
		return;
	}

	SetActorLocation(MeshComponent->GetComponentLocation());

	if (bIsAgresive == isAgresive)
	{
		UwSameInteraction->SetVisibility(ESlateVisibility::Visible);
	}
	else {
		UwDistinctInteraction->SetVisibility(ESlateVisibility::Visible);
	}
}

void AStrategyPlayer::StopInteraction()
{
	if (!UwDistinctInteraction || !UwSameInteraction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: StopInteraction: Uno de los widgets de interacción es nulo. No se puede ocultar."), *GetName());
		return;
	}
	UwDistinctInteraction->SetVisibility(ESlateVisibility::Hidden);
	UwSameInteraction->SetVisibility(ESlateVisibility::Hidden);
}

void AStrategyPlayer::BeginPlay()
{
	Super::BeginPlay();
	MeshComponent = FindComponentByClass<UStaticMeshComponent>();
	
	UWidgetComponent* DistinctInteractionWidgetComponent = FindComponentByTag<UWidgetComponent>(FName("DistinctInteraction"));
	if (DistinctInteractionWidgetComponent)
	{
		UwDistinctInteraction = Cast<UUserWidget>(DistinctInteractionWidgetComponent->GetUserWidgetObject());
		if (UwDistinctInteraction)
		{
			UwDistinctInteraction->SetVisibility(ESlateVisibility::Hidden);
		}
		UE_LOG(LogTemp, Log, TEXT("[%s]: DistinctInteractionWidgetComponent encontrado. UwDistinctInteraction = %s"), *GetName(), UwDistinctInteraction ? *UwDistinctInteraction->GetName() : TEXT("nullptr"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[%s]: DistinctInteractionWidgetComponent con la etiqueta 'DistinctInteraction' NO ENCONTRADO."), *GetName());
	}

	// Buscar y asignar UwSameInteraction
	UWidgetComponent* SameInteractionWidgetComponent = FindComponentByTag<UWidgetComponent>(FName("SameInteraction"));
	if (SameInteractionWidgetComponent)
	{
		UwSameInteraction = Cast<UUserWidget>(SameInteractionWidgetComponent->GetUserWidgetObject());
		if (UwSameInteraction)
		{
			UwSameInteraction->SetVisibility(ESlateVisibility::Hidden);
		}
		UE_LOG(LogTemp, Log, TEXT("[%s]: SameInteractionWidgetComponent encontrado. UwSameInteraction = %s"), *GetName(), UwSameInteraction ? *UwSameInteraction->GetName() : TEXT("nullptr"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[%s]: SameInteractionWidgetComponent con la etiqueta 'SameInteraction' NO ENCONTRADO."), *GetName());
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