// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "PlayerProccessor.generated.h"

class AStrategyPlayer;
class UOpTree;

USTRUCT()
struct FTreeArray
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<UOpTree*> Values;
};

UENUM()
enum class EResourceFilling : uint8
{
    NONE UMETA(DisplayName = "None"),
    STATIC UMETA(DisplayName = "Static"),
    EXPONENTIAL UMETA(DisplayName = "Exponential"),
};

/**
 * 
 */
UCLASS()
class TACTICINVASORS_API APlayerProccessor : public AInfo
{
    GENERATED_BODY()
    
public:
    // Constructor that uses the ObjectInitializer (required to create default subobjects safely)
    APlayerProccessor(const FObjectInitializer& ObjectInitializer);

    void Initialize();
    void ProcessPlayers(AStrategyPlayer* Player1, AStrategyPlayer* Player2);
    void ProcessPlayer(AStrategyPlayer* Player);
    int32 GetInitialPasivePlayers() const { return InitialPasivePlayers; }
    int32 GetInitialAgresivePlayers() const { return InitialAgresivePlayers; }
    int32 GetMaxIteractions() const { return MaxIteractions; }
    float GetSpeed() const { return Speed; }
    void DailyPenalty(AStrategyPlayer* Player);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Players|Data")
    FString Player1Name = "";

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Players|Data")
    FString Player2Name = "";

    int32 GetResourceFillingType() const { return static_cast<int32>(ResourceFillingType); }
    float GetV() const { return V; }
    float GetC() const { return C; }
    float GetM() const { return M; }
    float GetI() const { return I; }
    int32 GetInitialResources() const { return InitialResources; }
    int32 GetResourceIncrement(int T, int P);

private:
    void ReadFile();
    int32 InitialPasivePlayers;

    int32 InitialAgresivePlayers;

    UPROPERTY()
    //Value of resource
    float V = 0.0f; 

    UPROPERTY()
    //Cost for resource
    float C = 0.0f; 

    UPROPERTY()
	//Fitness penalty for each day
    float M = 0.0f; 

    UPROPERTY()
    //Initial fitness
    float I = 0.0f; 

    UPROPERTY()
    //Tipo de crecimiento de recursos
    EResourceFilling ResourceFillingType = EResourceFilling::STATIC;

    // Do NOT call NewObject<>() in-header or in the plain constructor initializer.
    // Create the subobject in the ctor implementation using the ObjectInitializer.
    UPROPERTY()
    UOpTree* ResourceFillingFormula = nullptr;

    UPROPERTY()
    //Game speed
    float Speed = 0.0f; 

    UPROPERTY()
    int32 MaxIteractions = 0;

    UPROPERTY()
    int32 InitialResources = 0;

    UPROPERTY()
    //Fitness need to growt
    float MaxFitness = 0.0f;


    UPROPERTY()
    TArray<float> SInteractions;

    UPROPERTY()
    TArray<FTreeArray> SInteractionsArray;
};
