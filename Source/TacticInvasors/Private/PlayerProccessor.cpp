// Fill out your copyright notice in the Description page of Project Settings.



#include "PlayerProccessor.h"
#include "Spawner.h"
#include "StrategyPlayer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Char.h" // For FChar::IsDigit, FChar::IsWhitespace
#include "Math/UnrealMathUtility.h" // For FMath::Pow
#include "Containers/UnrealString.h"

void APlayerProccessor::Initialize()
{
        ReadFile();
}



void APlayerProccessor::ProcessPlayers(AStrategyPlayer* Player1, AStrategyPlayer* Player2)
{

    if (!IsValid(Player1) || !IsValid(Player2))
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessPlayers called with invalid player(s)."));
        return;
    }

    // Resolve row/column indices in a clear, safe way
    const int32 Row = Player1->IsAggresive() ? 0 : 1;
    const int32 Col = Player2->IsAggresive() ? 0 : 1;

    float InteractionValue1 = 0.0f;
    if (SInteractionsArray.IsValidIndex(Row) && SInteractionsArray[Row].Values.IsValidIndex(Col))
    {
        InteractionValue1 = SInteractionsArray[Row].Values[Col];
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Missing interaction entry for Row=%d Col=%d; defaulting to 0."), Row, Col);
    }

    float P1Fitness = Player1->GetFitness() + InteractionValue1;
    Player1->SetFitness(P1Fitness);

    // Symmetric for player2
    const int32 Row2 = Player2->IsAggresive() ? 0 : 1;
    const int32 Col2 = Player1->IsAggresive() ? 0 : 1;

    float InteractionValue2 = 0.0f;
    if (SInteractionsArray.IsValidIndex(Row2) && SInteractionsArray[Row2].Values.IsValidIndex(Col2))
    {
        InteractionValue2 = SInteractionsArray[Row2].Values[Col2];
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Missing interaction entry for Row=%d Col=%d; defaulting to 0."), Row2, Col2);
    }

    float P2Fitness = Player2->GetFitness() + InteractionValue2;
    Player2->SetFitness(P2Fitness);
}

void APlayerProccessor::ProcessPlayer(AStrategyPlayer* Player)
{
    if (!IsValid(Player))
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessPlayer called with null or invalid Player"));
        return;
    }

    float Pfitness = Player->GetFitness();
    ASpawner* OwnerSpawner = Cast<ASpawner>(GetOwner());
    if (!IsValid(OwnerSpawner))
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessPlayer: Owner spawner is invalid; skipping processing for %s"), *Player->GetName());
        return;
    }

    if (Pfitness < 0.0)
    {
        Player->SetFitness(this->I);
        OwnerSpawner->KillPlayer(Player);
        return;
    }
	OwnerSpawner->PushWaitingPlayer(Player);

    if (Pfitness >= this->MaxFitness)
    {
        Player->SetFitness(this->I);
		OwnerSpawner->SpawnPlayer(Player);
    }
}

void APlayerProccessor::DailyPenalty(AStrategyPlayer* Player)
{
    float NewFitness = Player->GetFitness() - this->M;
    Player->SetFitness(NewFitness);
}

void APlayerProccessor::ReadFile()
{
    const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    const FString SavedDir = FPaths::Combine(ProjectDir, TEXT("Saved"));
    const FString FilePath = FPaths::Combine(SavedDir, TEXT("matrix.txt"));

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    if (!PlatformFile.DirectoryExists(*SavedDir))
    {
        if (!PlatformFile.CreateDirectoryTree(*SavedDir))
        {
            UE_LOG(LogTemp, Error, TEXT("ReadFile: Failed to create Saved directory: %s"), *SavedDir);
            return;
        }
    }

    // Ensure file exists with safe default content
    if (!PlatformFile.FileExists(*FilePath))
    {
        const FString DefaultContent = TEXT("1 0 0\n0 1 0\n0 0 1");
        if (!FFileHelper::SaveStringToFile(DefaultContent, *FilePath))
        {
            UE_LOG(LogTemp, Error, TEXT("ReadFile: Failed to write default matrix to %s"), *FilePath);
            return;
        }
    }
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("ReadFile: Failed to load file: %s"), *FilePath);
        return;
    }

    // Reset arrays/state before parsing to allow repeated calls
    SInteractionsArray.Empty();
    InitialAgresivePlayers = 0;
    InitialPasivePlayers = 0;
    AInitialFitness = 0.0f;
    PInitialFitness = 0.0f;

    for (const FString& LineRaw : Lines)
    {
        const FString Line = LineRaw.TrimStartAndEnd();
        if (Line.IsEmpty()) continue;

        UE_LOG(LogTemp, Log, TEXT("ReadFile: Line read: %s"), *Line);

        if (Line.StartsWith(TEXT("v=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            V = FCString::Atof(*ValueStr);
            continue;
        }
        if (Line.StartsWith(TEXT("c=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            C = FCString::Atof(*ValueStr);
            continue;
        }
        if (Line.StartsWith(TEXT("m=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            M = FCString::Atof(*ValueStr);
            continue;
        }
        if (Line.StartsWith(TEXT("i=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            I = FCString::Atof(*ValueStr);
            continue;
        }
        if (Line.StartsWith(TEXT("s=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            Speed = FCString::Atof(*ValueStr);
            continue;
        }
        if (Line.StartsWith(TEXT("p=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            MaxIteractions = FCString::Atoi(*ValueStr);
            continue;
        }
        if (Line.StartsWith(TEXT("r=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            bResourceFilling = ValueStr.ToBool();
            continue;
        }
        if (Line.StartsWith(TEXT("u=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            MaxFitness = FCString::Atof(*ValueStr);
            continue;
        }

        // Interaction lines: expected format example:
        // "0=exprA;exprB;...;AInitialFitness;InitialPlayers" or "1=..."
        bool bAgresive = false;
        if (Line.StartsWith(TEXT("0=")))
        {
            bAgresive = true;
        }
        else if (Line.StartsWith(TEXT("1=")))
        {
            bAgresive = false;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ReadFile: Unrecognized line prefix: %s"), *Line);
            continue;
        }

        const FString Content = Line.Mid(2);
        TArray<FString> Parts;
        Content.ParseIntoArray(Parts, TEXT(";"), /*CullEmpty=*/ true);

        // Expect at least two interaction expressions and two trailing numeric values (indices 3 and 4 in original code)
        if (Parts.Num() < 3)
        {
            UE_LOG(LogTemp, Warning, TEXT("ReadFile: Interaction line has insufficient parts (%d): %s"), Parts.Num(), *Line);
            // still attempt to parse whatever interaction expressions are present
        }

        FFloatArray InteractionArray;
        // Parse first two expressions as interaction matrix row values (guard indices)
        for (int32 i = 0; i < 2; ++i)
        {
            if (Parts.IsValidIndex(i))
            {
                const FString Expr = Parts[i].TrimStartAndEnd();
                const float Eval = ShuntingYard(Expr);
                InteractionArray.Values.Add(Eval);
                UE_LOG(LogTemp, Log, TEXT("ReadFile: Interaction expr[%d]='%s' => %f"), i, *Expr, Eval);
            }
            else
            {
                InteractionArray.Values.Add(0.0f);
                UE_LOG(LogTemp, Warning, TEXT("ReadFile: Missing interaction part %d for line: %s"), i, *Line);
            }
        }

        SInteractionsArray.Add(InteractionArray);

        if (Parts.IsValidIndex(2))
        {
            const FString CountStr = Parts[2].TrimStartAndEnd();
            const int32 CountVal = FCString::Atoi(*CountStr);
            if (bAgresive)
            {
                InitialAgresivePlayers = CountVal;
            }
            else
            {
                InitialPasivePlayers = CountVal;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ReadFile: Missing initial players value (index 4) in line: %s"), *Line);
        }
    } // end for lines

    UE_LOG(LogTemp, Log, TEXT("ReadFile: Parsed interactions rows = %d ; InitialAgg=%d InitialPas=%d"), SInteractionsArray.Num(), InitialAgresivePlayers, InitialPasivePlayers);

}

float APlayerProccessor::ApplyOp(char InOp, float B, float A)
{
    switch (InOp) {
    case '+': return A + B;
    case '-': return A - B;
    case '*': return A * B;
    case '/':
        if (B == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Division by zero in APlayerProccessor::ApplyOp for expression with operator '%c'"), InOp);
            return 0.0f; // Return a default value or handle error
        }
        return A / B;
    case '^': return FMath::Pow(A, B);
    }
    return 0.0f; // Should not reach here for valid operators
}

int APlayerProccessor::GetPrecedence(char InOp)
{
    if (InOp == '+' || InOp == '-') return 1;
    if (InOp == '*' || InOp == '/') return 2;
    if (InOp == '^') return 3; // Power operator has higher precedence
    return 0; // For '(' or unrecognized characters
}

void APlayerProccessor::ProcessOperator(TArray<char>& OpsStack, TArray<float>& ValuesStack)
{
    if (ValuesStack.Num() < 2 || OpsStack.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("APlayerProccessor::ProcessOperator - Malformed expression: not enough operands or operators. Values: %d, Ops: %d"), ValuesStack.Num(), OpsStack.Num());
        // In a real application, you might want to return a boolean indicating success/failure
        return;
    }

    float B = ValuesStack.Pop(); // Get second operand
    float A = ValuesStack.Pop(); // Get first operand
    char Op = OpsStack.Pop();    // Get operator
    ValuesStack.Push(ApplyOp(Op, B, A));
}

float APlayerProccessor::ShuntingYard(const FString& Expression)
{
    TArray<float> ResultsStack;
    TArray<char> OpsStack;

    int32 CurrentIndex = 0;
    while (CurrentIndex < Expression.Len())
    {
        TCHAR CurrentChar = Expression[CurrentIndex];

        // 1. Skip Whitespace
        if (FChar::IsWhitespace(CurrentChar))
        {
            CurrentIndex++;
            continue;
        }

        // 2. Handle Numbers
        if (FChar::IsDigit(CurrentChar) || CurrentChar == '.')
        {
            FString NumberString = TEXT("");
            while (CurrentIndex < Expression.Len() && (FChar::IsDigit(Expression[CurrentIndex]) || Expression[CurrentIndex] == '.'))
            {
                NumberString.AppendChar(Expression[CurrentIndex]);
                CurrentIndex++;
            }
            ResultsStack.Push(FCString::Atof(*NumberString));
            continue;
        }

        // 3. Handle Variables ('v', 'c', 'm')
        if (CurrentChar == 'v')
        {
            ResultsStack.Push(this->V);
            CurrentIndex++;
            continue;
        }
        if (CurrentChar == 'c')
        {
            ResultsStack.Push(this->C);
            CurrentIndex++;
            continue;
        }
        if (CurrentChar == 'm') // Assuming 'm' could also be a variable based on ReadFile context
        {
            ResultsStack.Push(this->M);
            CurrentIndex++;
            continue;
        }

        // 4. Handle Operators and Parentheses
        if (CurrentChar == '+' || CurrentChar == '-' || CurrentChar == '*' || CurrentChar == '/' || CurrentChar == '^')
        {
            while (OpsStack.Num() > 0 && GetPrecedence(OpsStack.Top()) >= GetPrecedence(CurrentChar) && OpsStack.Top() != '(')
            {
                ProcessOperator(OpsStack, ResultsStack);
            }
            OpsStack.Push(CurrentChar);
            CurrentIndex++;
            continue;
        }

        if (CurrentChar == '(')
        {
            OpsStack.Push(CurrentChar);
            CurrentIndex++;
            continue;
        }

        if (CurrentChar == ')')
        {
            while (OpsStack.Num() > 0 && OpsStack.Top() != '(')
            {
                ProcessOperator(OpsStack, ResultsStack);
            }

            if (OpsStack.Num() == 0 || OpsStack.Top() != '(')
            {
                UE_LOG(LogTemp, Error, TEXT("APlayerProccessor::ShuntingYard - Mismatched parentheses in expression: %s (Missing opening parenthesis)"), *Expression);
                return 0.0f; // Error: Mismatched parentheses
            }

            OpsStack.Pop(); // Pop the '('
            CurrentIndex++;
            continue;
        }

        // Unrecognized character
        UE_LOG(LogTemp, Warning, TEXT("APlayerProccessor::ShuntingYard - Unrecognized character '%c' in expression: %s at index %d"), CurrentChar, *Expression, CurrentIndex);
        return 0.0f; // Error: Unrecognized character
    }

    // 5. Process remaining operators
    while (OpsStack.Num() > 0)
    {
        if (OpsStack.Top() == '(')
        {
            UE_LOG(LogTemp, Error, TEXT("APlayerProccessor::ShuntingYard - Mismatched parentheses (remaining '(' on stack) in expression: %s"), *Expression);
            return 0.0f; // Error: Mismatched parentheses
        }
        ProcessOperator(OpsStack, ResultsStack);
    }

    // 6. Final Result
    if (ResultsStack.Num() == 1)
    {
        return ResultsStack.Pop();
    }
    else if (ResultsStack.Num() > 1)
    {
        UE_LOG(LogTemp, Error, TEXT("APlayerProccessor::ShuntingYard - Malformed expression: Too many values left on stack for expression: %s"), *Expression);
    }
    else // ResultsStack.Num() == 0
    {
        UE_LOG(LogTemp, Error, TEXT("APlayerProccessor::ShuntingYard - Expression resulted in no value: %s"), *Expression);
    }

    return 0.0f;
}
