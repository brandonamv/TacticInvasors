#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyPlayer.generated.h"

class AResource;
class UStaticMeshComponent; 
class UBillboardComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class EPlayerState : uint8 {
	IDLE = 0 UMETA(DisplayName = "IDLE"),
	INTERACTING = 1  UMETA(DisplayName = "INTERACTING"),
	DEATH = 2     UMETA(DisplayName = "DEATH"),
	SAME = 3     UMETA(DisplayName = "SAME"),
	DISTINCT = 4     UMETA(DisplayName = "DISTINCT"),
};

UCLASS()
class TACTICINVASORS_API AStrategyPlayer : public AActor
{
	GENERATED_BODY()

public:
	AStrategyPlayer();
	virtual void Tick(float DeltaTime) override;

	void InitializeAgent(AResource* Target);
	void SetAggresive(bool bAggresive) { bIsAgresive = bAggresive; }
	bool IsAggresive() const { return bIsAgresive; }
	void SetFitness(float InFitness) { Fitnees = InFitness; }
	float GetFitness() const { return Fitnees; }
	void SetSpeed(float InSpeed) { ASpeed = InSpeed; }
	float GetSpeed() const { return ASpeed; }
	USphereComponent* GetInteractionSphere() const { return InteractionSphere; }
	void PlayInteraction(bool isAgresive);
	void StopInteraction();
	void KillPlayer();
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UBillboardComponent* UwSameInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UBillboardComponent* UwDistinctInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UBillboardComponent* UwDeathInteraction;

	UPROPERTY()
	AResource* TargetResource = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerData")
	bool bIsAgresive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerData")
	float Fitnees;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerData")
	EPlayerState PlayerState = EPlayerState::IDLE;

	UPROPERTY()
	float ASpeed = 0.0f;

	bool bHasTarget = false;
};