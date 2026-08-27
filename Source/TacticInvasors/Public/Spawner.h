// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



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
    AResource* PopFreeResource();

	void PushFreePlayer(AStrategyPlayer* Player);
	void PushWaitingPlayer(AStrategyPlayer* Player);

	void ProcessPlayers(AStrategyPlayer* Player1, AStrategyPlayer* Player2);
    void ProcessPlayer(AStrategyPlayer* Player);

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Data")
    int32 CurrentAgresivePlayers = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Data")
    int32 CurrentPasivePlayers = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Data")
    int32 MaxIterations = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Data")
    int32 CurrentIteration = 0;

    // Tamaño del área tridimensional de spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Config")
    FVector SpawnAreaSize;

    // Ubicación central del spawner en el mapa (ya que AInfo no tiene componente visual)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Config", meta = (MakeEditWidget = true))
    FVector SpawnerCenterLocation;

private:
    FTimerHandle SpawnPlayersTimerHandle;
    FTimerHandle DeathPlayersTimerHandle;
	FTimerHandle KillPlayersTimerHandle;

    void SpawnAgents();
    void AssignFreePlayers();

	// Declaration only: implementation moved to .cpp to avoid header-level parsing issues
	void ShuffleArray(TArray<AStrategyPlayer*>& TargetArray);

    UPROPERTY()
    float Speed = 0.0f;

    UPROPERTY()
    TArray<AResource*> SFreeResources;

    UPROPERTY()
    TArray<AResource*> SActiveResources;

    UPROPERTY()
    TArray<AStrategyPlayer*> SFreePlayers;

    UPROPERTY()
    TArray<AStrategyPlayer*> SActivePlayers;
	

    UPROPERTY()
	TSet<APasive*> SDeadPasives;

    UPROPERTY()
    TSet<AAgresive*> SDeadAgresives;

    UPROPERTY()
    APlayerProccessor* APlayerProcessor = nullptr;
};
