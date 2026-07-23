// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "PlayerProccessor.generated.h"

USTRUCT()
struct FFloatArray
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<float> Values;
};


class AStrategyPlayer;

/**
 * 
 */
UCLASS()
class TACTICINVASORS_API APlayerProccessor : public AInfo
{
	GENERATED_BODY()
	
public:
	void Initialize();
	void ProcessPlayers(AStrategyPlayer* Player1, AStrategyPlayer* Player2);
    void ProcessPlayer(AStrategyPlayer* Player);
	int32 GetInitialPasivePlayers() const { return InitialPasivePlayers; }
	int32 GetInitialAgresivePlayers() const { return InitialAgresivePlayers; }
    int32 GetMaxIteractions() const { return MaxIteractions; }
    float GetSpeed() const { return Speed; }
    void DailyPenalty(AStrategyPlayer* Player);

private:
    void ReadFile();
    float ApplyOp(char Op, float B, float A);
    int GetPrecedence(char Op);
    void ProcessOperator(TArray<char>& OpsStack, TArray<float>& ValuesStack);
    float ShuntingYard(const FString& Expression);

    int32 InitialPasivePlayers;

    int32 InitialAgresivePlayers;

    UPROPERTY()
    float V = 0.0f;

    UPROPERTY()
    float C = 0.0f;

    UPROPERTY()
    float M = 0.0f;

    UPROPERTY()
	float I = 0.0f;

    UPROPERTY()
    float Speed = 0.0f;

    UPROPERTY()
    int32 MaxIteractions = 0;

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
};
