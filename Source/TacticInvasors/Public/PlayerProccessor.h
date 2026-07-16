// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Char.h" // For FChar::IsDigit, FChar::IsWhitespace
#include "Math/UnrealMathUtility.h" // For FMath::Pow, often included implicitly but good to be explicit

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
	int32 GetInitialPasivePlayers() const { return InitialPasivePlayers; }
	int32 GetInitialAgresivePlayers() const { return InitialAgresivePlayers; }

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
