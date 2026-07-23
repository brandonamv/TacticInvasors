// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Containers/Queue.h"

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Spawner.generated.h"

class APasive;
class AAgresive;
class AResource;
class AStrategyPlayer;
class APlayerProccessor;
/**
 * 
 */
UCLASS()
class TACTICINVASORS_API ASpawner : public AInfo
{
	GENERATED_BODY()

public:
    ASpawner();
    void PushFreeResource(AResource* Resource);
    AResource* PopFreeResource(AStrategyPlayer* Player);

	void PushFreePlayer(AStrategyPlayer* Player);
	void PushWaitingPlayer(AStrategyPlayer* Player);

	void ProcessPlayers(AStrategyPlayer* Player1, AStrategyPlayer* Player2);

    void SpawnPlayer(AStrategyPlayer* Player);
	void KillPlayer(AStrategyPlayer* Player);

protected:
    virtual void BeginPlay() override;

public:
    // Tipo de actor a spawnear (configurable desde Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Config")
    TSubclassOf<APasive> PasiveClass;
    // Tipo de actor a spawnear (configurable desde Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Config")
    TSubclassOf<AAgresive> AgresiveClass;
    // Tipo de actor a spawnear (configurable desde Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Config")
    TSubclassOf<AResource> ResourceClass;

    // Cantidad inicial de actores pasivos/agresivos a generar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Config")
    int32 InitialAgresivePlayers;
    // Cantidad inicial de actores pasivos/agresivos a generar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Config")
    int32 InitialPasivePlayers;
    // Cantidad inicial de actores pasivos/agresivos a generar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Config")
    int32 InitialResourcePlayers;

    // Tamaño del área tridimensional de spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Config")
    FVector SpawnAreaSize;

    // Ubicación central del spawner en el mapa (ya que AInfo no tiene componente visual)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Config", meta = (MakeEditWidget = true))
    FVector SpawnerCenterLocation;

private:
    FTimerHandle SpawnPlayersTimerHandle;

    void SpawnAgents();
    void AssignFreePlayers();

    template<typename T>
    void ShuffleArray(TArray<T>& TargetArray);

    UPROPERTY()
    int32 MaxIterations = 0;
    UPROPERTY()
    int32 CurrentIteration = 0;
    UPROPERTY()
    float Speed = 0.0f;

    UPROPERTY()
    TArray<AResource*> SFreeResources;
	TArray<AResource*> SActiveResources;

    UPROPERTY()
    TArray<AStrategyPlayer*> SFreePlayers;
    TArray<AStrategyPlayer*> SActivePlayers;
	
    UPROPERTY()
    int32 CurrentAgresivePlayers = 0;

    UPROPERTY()
    int32 CurrentPasivePlayers = 0;

    UPROPERTY()
	TArray<APasive*> SDeadPasives;

    UPROPERTY()
    TArray<AAgresive*> SDeadAgresives;

    UPROPERTY()
    APlayerProccessor* APlayerProcessor = nullptr;
};

template<typename T>
inline void ASpawner::ShuffleArray(TArray<T>& TargetArray)
{
    const int32 LastIndex = TargetArray.Num() - 1;
    for (int32 i = 0; i <= LastIndex; ++i)
    {
        int32 Index = FMath::RandRange(i, LastIndex);
        if (i != Index)
        {
            TargetArray.Swap(i, Index);
        }
    }

}
