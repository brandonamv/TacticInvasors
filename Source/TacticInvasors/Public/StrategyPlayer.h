#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyPlayer.generated.h"

class ASpawner;
class AResource;
class UStaticMeshComponent; 
class UBillboardComponent;

UCLASS()
class TACTICINVASORS_API AStrategyPlayer : public AActor
{
	GENERATED_BODY()

public:
	AStrategyPlayer();
	virtual void Tick(float DeltaTime) override;

	bool InitializeAgent();
	void SetAggresive(bool bAggresive) { bIsAgresive = bAggresive; }
	bool IsAggresive() const { return bIsAgresive; }
	void SetFitness(float InFitness) { Fitnees = InFitness; }
	float GetFitness() const { return Fitnees; }
	void SetSpeed(float InSpeed) { ASpeed = InSpeed; }
	float GetSpeed() const { return ASpeed; }
	UStaticMeshComponent* GetMesh() const { return MeshComponent; }
	void PlayInteraction(bool isAgresive);
	void StopInteraction();
protected:
	virtual void BeginPlay() override;

	// Añadimos el componente en caché para evitar FindComponentByClass
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UBillboardComponent* UwSameInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UBillboardComponent* UwDistinctInteraction;

	UPROPERTY()
	AResource* TargetResource = nullptr;

	UPROPERTY()
	bool bIsAgresive = false;

	UPROPERTY()
	float Fitnees;

	UPROPERTY()
	float ASpeed = 0.0f;

	bool bHasTarget = false;
};