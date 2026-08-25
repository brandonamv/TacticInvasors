// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerProccessor.h"
#include "Spawner.h"
#include "StrategyPlayer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Char.h"
#include "Math/UnrealMathUtility.h"
#include "Containers/UnrealString.h"
#include "OpTree.h"

// Constructor: create the default subobject via the ObjectInitializer (safe inside UObject-derived ctor)
APlayerProccessor::APlayerProccessor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Create a named default subobject instead of calling NewObject() here
    ResourceFillingFormula = ObjectInitializer.CreateDefaultSubobject<UOpTree>(this, TEXT("ResourceFillingFormula"));
    if (ResourceFillingFormula)
    {
        // initialize formula variables to current defaults (will be overwritten when ReadFile() runs)
        ResourceFillingFormula->SetV(V);
        ResourceFillingFormula->SetC(C);
    }
}

// Add this helper above ReadFile() (or at top of file). This implements a simple
// shunting-yard evaluation for basic arithmetic so `ShuntingYard` is defined.
// It will evaluate numbers and + - * / and parentheses. On parse failure it
// falls back to `FCString::Atof` of the whole string.
static int32 GetOpPrecedence(TCHAR Op)
{
    switch (Op)
    {
    case '+':
    case '-':
        return 1;
    case '*':
    case '/':
        return 2;
    default:
        return 0;
    }
}

static void ApplyOperator(TArray<float>& Values, TCHAR Op)
{
    if (Values.Num() < 2) return;
    const float Right = Values.Pop();
    const float Left = Values.Pop();
    float Res = 0.0f;
    switch (Op)
    {
    case '+': Res = Left + Right; break;
    case '-': Res = Left - Right; break;
    case '*': Res = Left * Right; break;
    case '/': Res = (Right == 0.0f) ? 0.0f : (Left / Right); break;
    default: break;
    }
    Values.Add(Res);
}

static float ShuntingYard(const FString& Expr)
{
    FString S = Expr;
    S.TrimStartAndEndInline();

    if (S.IsEmpty())
    {
        return 0.0f;
    }

    TArray<float> Values;
    TArray<TCHAR> Ops;

    const int32 Len = S.Len();
    int32 i = 0;
    while (i < Len)
    {
        const TCHAR Ch = S[i];

        if (FChar::IsWhitespace(Ch))
        {
            ++i;
            continue;
        }

        // Number (supports decimal point and scientific notation pieces; we rely on FCString::Atof)
        if (FChar::IsDigit(Ch) || Ch == '.' || ((Ch == '+' || Ch == '-') && i + 1 < Len && (FChar::IsDigit(S[i + 1]) || S[i + 1] == '.')))
        {
            int32 Start = i;
            // allow leading sign if at start or after '(' or an operator
            if ((S[Start] == '+' || S[Start] == '-') && Start + 1 < Len && (FChar::IsDigit(S[Start + 1]) || S[Start + 1] == '.'))
            {
                ++i;
            }
            while (i < Len && (FChar::IsDigit(S[i]) || S[i] == '.' || S[i] == 'e' || S[i] == 'E' || S[i] == '+' || S[i] == '-'))
            {
                // break on a +/- that is clearly an operator (followed by whitespace or digit context handling above prevents common issues)
                // To keep this lightweight we stop number on encountering an operator char with surrounding spaces (common cases).
                if ((S[i] == '+' || S[i] == '-') && i > Start && !FChar::IsDigit(S[i - 1]) && S[i - 1] != 'e' && S[i - 1] != 'E')
                {
                    break;
                }
                ++i;
            }
            const FString NumStr = S.Mid(Start, i - Start);
            const float Val = FCString::Atof(*NumStr);
            Values.Add(Val);
            continue;
        }

        if (Ch == '(')
        {
            Ops.Add(Ch);
            ++i;
            continue;
        }

        if (Ch == ')')
        {
            while (Ops.Num() && Ops.Last() != '(')
            {
                ApplyOperator(Values, Ops.Pop());
            }
            if (Ops.Num() && Ops.Last() == '(')
            {
                Ops.Pop();
            }
            ++i;
            continue;
        }

        // Operator
        if (Ch == '+' || Ch == '-' || Ch == '*' || Ch == '/')
        {
            while (Ops.Num() && Ops.Last() != '(' && GetOpPrecedence(Ops.Last()) >= GetOpPrecedence(Ch))
            {
                ApplyOperator(Values, Ops.Pop());
            }
            Ops.Add(Ch);
            ++i;
            continue;
        }

        // Unknown character -> cannot parse expression; fall back to simple atof of full expression
        return FCString::Atof(*S);
    }

    while (Ops.Num())
    {
        ApplyOperator(Values, Ops.Pop());
    }

    if (Values.Num() > 0)
    {
        return Values.Last();
    }

    // Fallback
    return FCString::Atof(*S);
}

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
	UE_LOG(LogTemp, Log, TEXT("ProcessPlayers: Evaluating interaction for Row=%d Col=%d"), Row, Col);
    if (SInteractionsArray.IsValidIndex(Row) && SInteractionsArray[Row].Values.IsValidIndex(Col))
    {
        UOpTree* Tree = SInteractionsArray[Row].Values[Col];
		UE_LOG(LogTemp, Log, TEXT("ProcessPlayers: Evaluating interaction for Row=%d Col=%d using tree %s"), Row, Col, *GetNameSafe(Tree));
        if (IsValid(Tree))
        {
            InteractionValue1 = Tree->EvaluarRaiz();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ProcessPlayers: missing tree at Row=%d Col=%d; defaulting to 0."), Row, Col);
        }
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
        UOpTree* Tree2 = SInteractionsArray[Row2].Values[Col2];
        if (IsValid(Tree2))
        {
            InteractionValue2 = Tree2->EvaluarRaiz();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ProcessPlayers: missing tree at Row=%d Col=%d; defaulting to 0."), Row2, Col2);
        }
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

int32 APlayerProccessor::GetResourceIncrement(int T, int P)
{
	UE_LOG(LogTemp, Log, TEXT("GetResourceIncrement called with T=%d P=%d"), T, P);
	ResourceFillingFormula->SetT(T);
	ResourceFillingFormula->SetP(P);
    float Result = ResourceFillingFormula->EvaluarRaiz();
	UE_LOG(LogTemp, Log, TEXT("GetResourceIncrement: Evaluated formula result = %f"), Result);
	return FMath::Max(0, FMath::Floor(Result));
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

    for (const FString& LineRaw : Lines)
    {
        const FString Line = LineRaw.TrimStartAndEnd();
        if (Line.IsEmpty()) continue;

        if (Line.StartsWith(TEXT("v=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            V = FMath::Max(0.0f, FCString::Atof(*ValueStr));
            continue;
        }
        if (Line.StartsWith(TEXT("c=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            C = FMath::Max(0.0f, FCString::Atof(*ValueStr));
            continue;
        }
        if (Line.StartsWith(TEXT("m=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            M = FMath::Max(0.0f, FCString::Atof(*ValueStr));
            continue;
        }
        if (Line.StartsWith(TEXT("i=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            I = FMath::Max(0.0f, FCString::Atof(*ValueStr));
            continue;
        }
        if (Line.StartsWith(TEXT("s=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            Speed = FMath::Max(0.1f, FCString::Atof(*ValueStr));
            continue;
        }
        if (Line.StartsWith(TEXT("p=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            MaxIteractions = FMath::Max(0, FCString::Atoi(*ValueStr));
            continue;
        }
        if (Line.StartsWith(TEXT("r=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            InitialResources = FMath::Max(0, FCString::Atoi(*ValueStr));
            continue;
        }
        if (Line.StartsWith(TEXT("u=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            MaxFitness = FMath::Max(0.0f, FCString::Atof(*ValueStr));
            continue;
        }
        if (Line.StartsWith(TEXT("t=")))
        {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            ResourceFillingType = static_cast<EResourceFilling>(FMath::Max(0, FCString::Atoi(*ValueStr)));
			continue;
        }

        if (ResourceFillingType != EResourceFilling::NONE && Line.StartsWith(TEXT("f="))) {
            const FString ValueStr = Line.Mid(2).TrimStartAndEnd();
            if (ResourceFillingFormula)
            {
                if (!ResourceFillingFormula->ConstruirDesdeInfix(ValueStr))
                {
                    // Fallback: try numeric parse and create simple constant tree
                    const float ConstVal = ShuntingYard(ValueStr);
                    const FString ConstStr = FString::SanitizeFloat(ConstVal);
                    TSharedPtr<FNodoArbol> NullNode = nullptr;
                    ResourceFillingFormula->Insertar(ConstStr, NullNode);
                }
            }
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

        FTreeArray InteractionArray;
        // Parse first two expressions as interaction matrix row values (guard indices)
        for (int32 i = 0; i < 2; ++i)
        {
            if (Parts.IsValidIndex(i))
            {
                const FString Expr = Parts[i].TrimStartAndEnd();

                // Build an OpTree for this expression and store it for runtime evaluation
                UOpTree* Tree = NewObject<UOpTree>(this);
				Tree->SetV(this->V);
				Tree->SetC(this->C);
                if (Tree)
                {
                    if (!Tree->ConstruirDesdeInfix(Expr))
                    {
                        // Fallback: try numeric parse and create simple constant tree
                        const float ConstVal = ShuntingYard(Expr);
                        const FString ConstStr = FString::SanitizeFloat(ConstVal);
                        TSharedPtr<FNodoArbol> NullNode = nullptr;
                        Tree->Insertar(ConstStr, NullNode);
                    }
                    InteractionArray.Values.Add(Tree);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("ReadFile: Failed to allocate UOpTree for expression: %s"), *Expr);
                    InteractionArray.Values.Add(nullptr);
                }
            }
            else
            {
                InteractionArray.Values.Add(nullptr);
            }
        }

        SInteractionsArray.Add(InteractionArray);

        if (Parts.IsValidIndex(2))
        {
            const FString CountStr = Parts[2].TrimStartAndEnd();
            const int32 CountVal = FMath::Max(0, FCString::Atoi(*CountStr));
            if (bAgresive)
            {
                InitialAgresivePlayers = CountVal;
            }
            else
            {
                InitialPasivePlayers = CountVal;
            }
        }
        if (Parts.IsValidIndex(3))
        {
            const FString CountStr = Parts[3].TrimStartAndEnd();
            if (bAgresive)
            {
                Player1Name = Parts[3];
            }
            else
            {
                Player2Name = Parts[3];
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ReadFile: Missing initial players value (index 4) in line: %s"), *Line);
        }
    } 

}

