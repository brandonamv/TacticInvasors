// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Pasive.h"
#include "Agresive.h"
#include "Resource.h"
#include <stack>
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

    std::stack<AResource*> SFreeResources;
    std::stack<AResource*> SBussyResources;

	std::stack<APasive*> SAlivePasives;
	std::stack<APasive*> SDeadPasives;

	std::stack<AAgresive*> SAliveAgresives;
	std::stack<AAgresive*> SDeadAgresives;


};
