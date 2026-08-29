// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyPopulationChartWidget.generated.h"

USTRUCT(BlueprintType)
struct FPopulationRecord
{
    GENERATED_BODY()
    float TypeA; // Ej. Halcones
    float TypeB; // Ej. Palomas
};

/**
 * 
 */
UCLASS()
class TACTICINVASORS_API UMyPopulationChartWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    // Lista con el registro de poblaciones a lo largo del tiempo
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Data")
    TArray<FPopulationRecord> PopulationHistory;
    UFUNCTION(BlueprintCallable, Category = "Chart Data")
    void AddPopulationRecord(float AgresiveCount, float PasiveCount);

protected:
    // Sobrescribir la función nativa de dibujo de Slate
    virtual int32 NativePaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;
};
