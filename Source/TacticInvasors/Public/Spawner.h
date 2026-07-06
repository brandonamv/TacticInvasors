// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Spawner.generated.h"

USTRUCT()
struct FFloatArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<float> Values;
};

class APasive;
class AAgresive;
class AResource;
class AStrategyPlayer;
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

    void ReadFile();
    float ApplyOp(char Op, float B, float A);
	int GetPrecedence(char Op);
	void ProcessOperator(TArray<char>& OpsStack, TArray<float>& ValuesStack);
	float ShuntingYard(const FString& Expression);
    void SpawnAgents();
    void AssignFreePlayers();
    UPROPERTY()
	bool bSpawningAgents = false;

    UPROPERTY()
	float V = 0.0f;

    UPROPERTY()
    float C = 0.0f;

    UPROPERTY()
	float M = 0.0f;

    UPROPERTY()
	bool bResourceFilling = false;

    UPROPERTY()
	float MaxFitness = 0.0f;

    UPROPERTY()
	float AInitialFitness = 0.0f;

    UPROPERTY()
	float PInitialFitness = 0.0f;

    UPROPERTY()
    float AFinalFitness = 0.0f;

    UPROPERTY()
    float PFinalFitness = 0.0f;

    UPROPERTY()
    TArray<float> SInteractions;

    UPROPERTY()
	TArray<FFloatArray> SInteractionsArray;

    UPROPERTY()
    TArray<AResource*> SFreeResources;

    UPROPERTY()
	TArray<AStrategyPlayer*> SFreePlayers;

    UPROPERTY()
    TArray<APasive*> SAlivePasives;

    UPROPERTY()
	TArray<APasive*> SDeadPasives;

    UPROPERTY()
    TArray<AAgresive*> SAliveAgresives;

    UPROPERTY()
    TArray<AAgresive*> SDeadAgresives;


};
