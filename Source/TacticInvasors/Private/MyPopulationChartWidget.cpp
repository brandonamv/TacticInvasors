// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPopulationChartWidget.h"
#include "Rendering/DrawElements.h"

int32 UMyPopulationChartWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    if (PopulationHistory.Num() == 0) return LayerId;

    FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
    float BarWidth = FMath::Floor(WidgetSize.X / PopulationHistory.Num());
    float HalfHeight = WidgetSize.Y / 2.0f;

    // Dejar un pequeño espacio entre barras si hay pocas iteraciones
    float Spacing = FMath::Max(1.0f, 50.0f / PopulationHistory.Num());
    float ActualBarWidth = FMath::Floor(FMath::Max(1.0f, BarWidth - Spacing));

    float MaxPop = 1.0f;
    for (const FPopulationRecord& Rec : PopulationHistory)
    {
        if (Rec.TypeA > MaxPop) MaxPop = Rec.TypeA;
        if (Rec.TypeB > MaxPop) MaxPop = Rec.TypeB;
    }

    const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

    for (int32 i = 0; i < PopulationHistory.Num(); i++)
    {
        float XOffset = FMath::Floor(i * BarWidth);

        // Barra Agresivos (Roja) - Crece desde la mitad hacia arriba
        float HeightA = (PopulationHistory[i].TypeA / MaxPop) * HalfHeight;
        FGeometry GeomA = AllottedGeometry.MakeChild(
            FVector2D(BarWidth, HeightA),
            FSlateLayoutTransform(FVector2D(XOffset, HalfHeight - HeightA))
        );
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId, GeomA.ToPaintGeometry(), WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.679543f, 0.021219f, 0.028426f));

        // Barra Pasivos (Azul) - Crece desde la mitad hacia abajo
        float HeightB = (PopulationHistory[i].TypeB / MaxPop) * HalfHeight;
        FGeometry GeomB = AllottedGeometry.MakeChild(
            FVector2D(BarWidth, HeightB),
            FSlateLayoutTransform(FVector2D(XOffset, HalfHeight))
        );
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId, GeomB.ToPaintGeometry(), WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.105f, 0.527115f, 0.730461f));
    }

    return LayerId;
}

void UMyPopulationChartWidget::AddPopulationRecord(float AgresiveCount, float PasiveCount)
{
    FPopulationRecord NewRecord;
    NewRecord.TypeA = AgresiveCount;
    NewRecord.TypeB = PasiveCount;
    PopulationHistory.Add(NewRecord);
}