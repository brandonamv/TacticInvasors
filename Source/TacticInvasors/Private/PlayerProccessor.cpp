// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerProccessor.h"
#include "StrategyPlayer.h" // Add this include to resolve the incomplete type error

void APlayerProccessor::Initialize()
{
    ReadFile();
    // Log SInteractionsArray matrix
	if (SInteractionsArray.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("SInteractionsArray is empty."));
	}
	else
	{
		FString MatrixStr = TEXT("SInteractionsArray matrix:\n");
		for (int32 Row = 0; Row < SInteractionsArray.Num(); ++Row)
		{
			const FFloatArray& RowArray = SInteractionsArray[Row];
			FString RowStr = FString::Printf(TEXT("Row %d: ["), Row);
			for (int32 Col = 0; Col < RowArray.Values.Num(); ++Col)
			{
				// Safely access value (we already have Col within range)
				float Val = RowArray.Values[Col];
				// Append value with comma separation
				if (Col == 0)
				{
					RowStr += FString::Printf(TEXT("%f"), Val);
				}
				else
				{
					RowStr += FString::Printf(TEXT(", %f"), Val);
				}
			}
			RowStr += TEXT("]\n");
			MatrixStr += RowStr;
		}
		UE_LOG(LogTemp, Log, TEXT("%s"), *MatrixStr);
	}
}

void APlayerProccessor::ProcessPlayers(AStrategyPlayer* Player1, AStrategyPlayer* Player2)
{
	// Existing fitness calculations
    UE_LOG(LogTemp, Warning, TEXT("Processing players: P1 Aggressive: %d, P2 Aggressive: %d"), Player1->IsAggresive() ? 0 : 1,   Player2->IsAggresive() ? 0 : 1);
    float P1Fitness = Player1->GetFitness() + SInteractionsArray[!Player1->IsAggresive()].Values[!Player2->IsAggresive()];
	Player1->SetFitness(P1Fitness);
    float P2Fitness = Player2->GetFitness() + SInteractionsArray[!Player2->IsAggresive()].Values[!Player1->IsAggresive()];
	Player2->SetFitness(P2Fitness);

	UE_LOG(LogTemp, Warning, TEXT("P1 fitness: %f, P2 fitness: %f"), Player1->GetFitness(), Player2->GetFitness());
}

void APlayerProccessor::ReadFile()
{
    FString RutaArchivo = FPaths::ProjectSavedDir() / TEXT("matrix.txt");
    TArray<FString> Lineas;

    // LoadFileToStringArray divide el archivo automáticamente por cada salto de línea
    if (FFileHelper::LoadFileToStringArray(Lineas, *RutaArchivo))
    {
        for (const FString& Linea : Lineas)
        {
            UE_LOG(LogTemp, Log, TEXT("Línea leida: %s"), *Linea);
            if (Linea.StartsWith(TEXT("v=")))
            {
                FString ValorStr = Linea.RightChop(2); // Elimina "V:"
                V = FCString::Atof(*ValorStr);
            }
            else if (Linea.StartsWith(TEXT("c=")))
            {
                FString ValorStr = Linea.RightChop(2); // Elimina "C:"
                C = FCString::Atof(*ValorStr);
            }
            else if (Linea.StartsWith(TEXT("m=")))
            {
                FString ValorStr = Linea.RightChop(2); // Elimina "M:"
                M = FCString::Atof(*ValorStr);
            }
            else if (Linea.StartsWith(TEXT("r=")))
            {
                FString ValorStr = Linea.RightChop(2); // Elimina "R:"
                bResourceFilling = ValorStr.ToBool();
            }
            else if (Linea.StartsWith(TEXT("u=")))
            {
                FString ValorStr = Linea.RightChop(11); // Elimina "U:"
                MaxFitness = FCString::Atof(*ValorStr);
            }
            else
            {
                bool bAgresive = false;
                if (Linea.StartsWith(TEXT("0=")))
                {
                    bAgresive = true;
                }
                else if (Linea.StartsWith(TEXT("1=")))
                {
                    bAgresive = false;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Línea no reconocida: %s"), *Linea);
                    continue; // Salta líneas que no comienzan con A: o P:
                }
                FFloatArray InteractionArray;
                TArray<FString> ValoresStr;
                FString Content = Linea.RightChop(2); // Elimina el prefijo "0=" o "1="
                Content.ParseIntoArray(ValoresStr, TEXT(";"), true);
                for (int32 i = 0; i < 2 && i < ValoresStr.Num(); i++)
                {
                    InteractionArray.Values.Add(ShuntingYard(ValoresStr[i]));
					UE_LOG(LogTemp, Log, TEXT("Interaction value for %s: %f"), *ValoresStr[i], InteractionArray.Values.Last());
                }
                SInteractionsArray.Add(InteractionArray);
                if (bAgresive)
                {
                    AInitialFitness = FCString::Atof(*ValoresStr[3]);
                    InitialAgresivePlayers = FCString::Atoi(*ValoresStr[4]);
                }
                else {
                    PInitialFitness = FCString::Atof(*ValoresStr[3]);
                    InitialPasivePlayers = FCString::Atoi(*ValoresStr[4]);
                }

            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Error al cargar las líneas del archivo."));
    }
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
