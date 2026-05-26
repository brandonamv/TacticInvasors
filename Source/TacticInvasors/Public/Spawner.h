// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Pasive.h"
#include "Agresive.h"
#include "Resource.h"
#include "Spawner.generated.h"

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

    UPROPERTY()
    TArray<AResource*> SFreeResources;

    UPROPERTY()
    TArray<APasive*> SAlivePasives;

    UPROPERTY()
    TArray<APasive*> SDeadPasives;

    UPROPERTY()
    TArray<AAgresive*> SAliveAgresives;

    UPROPERTY()
    TArray<AAgresive*> SDeadAgresives;


};
