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

void AStrategyPlayer::InitializeAgent(ASpawner* InSpawner)
{
	if (!InSpawner)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: Spawner is null. Cannot initialize agent."), *GetName());
		return;
	}
	TargetResource = InSpawner->PopFreeResource();
	if (TargetResource)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s]: Assigned target resource [%s]."), *GetName(), *TargetResource->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: No available resources from spawner."), *GetName());
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
		TargetResource = this->GetOwner<ASpawner>()->PopFreeResource();
		return;
	}

	// 1. OBTENER EL MESH PRIMERO: Necesitamos su ubicación física real
	UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>();
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s]: No static mesh component found."), *GetName());
		return;
	}

	// 2. Usamos GetComponentLocation() en vez de GetActorLocation() 
	FVector CurrentPhysicalLocation = Mesh->GetComponentLocation();

	// El objetivo también debería evaluarse idealmente desde su componente visual o su origen
	FVector GoalLocation = TargetResource->GetActorLocation();

	// 3. Calculamos la distancia real entre la esfera física y el cubo objetivo
	const float Distance = FVector::Dist(CurrentPhysicalLocation, GoalLocation);

	// 4. CONDICIÓN DE PARADA: 150.0f o 200.0f es una distancia prudente para el radio de colisión
	if (Distance <= 150.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s]: Ha alcanzado el objetivo [%s] de forma física! Deteniendo."), *GetName(), *TargetResource->GetName());

		if (Mesh->IsSimulatingPhysics())
		{
			// Frenamos por completo fuerzas lineales y rotacionales
			Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}

		TargetResource->SetTaked();

		// Limpiamos el target para no volver a entrar aquí
		TargetResource = nullptr;
		return;
	}

	// 5. LÓGICA DE MOVIMIENTO DIRECCIONAL
	FVector Direction = (GoalLocation - CurrentPhysicalLocation).GetSafeNormal();

	const float Speed = 300.0f;
	FVector TargetVelocity = Direction * Speed;

	if (!Mesh->IsSimulatingPhysics())
	{
		Mesh->SetSimulatePhysics(true);
	}

	// Mantener la gravedad
	TargetVelocity.Z = Mesh->GetComponentVelocity().Z;

	// Aplicamos la velocidad directamente al Mesh físico
	Mesh->SetPhysicsLinearVelocity(TargetVelocity);
	this->SetActorLocation(CurrentPhysicalLocation);

}

