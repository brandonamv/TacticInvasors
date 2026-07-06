// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawner.h"
#include "Pasive.h"
#include "Agresive.h"
#include "StrategyPlayer.h"
#include "Resource.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Misc/Char.h" // For FChar::IsDigit, FChar::IsWhitespace
#include "Math/UnrealMathUtility.h" // For FMath::Pow, often included implicitly but good to be explicit
#include "Containers/UnrealString.h" // For FString operations, usually in CoreMinimal.h

void ASpawner::PushFreeResource(AResource* Resource)
{
    if (!Resource)
    {
        return;
    }

    // Store resource in the free pool
    SFreeResources.Push(Resource);

    UStaticMeshComponent* Mesh = Resource->FindComponentByClass<UStaticMeshComponent>();
    if (Mesh)
        Mesh->SetSimulatePhysics(true);

    UE_LOG(LogTemp, Log, TEXT("Resource free [%s]: Resources Freed %d"),
        *Resource->GetName(), SFreeResources.Num());
}
AResource* ASpawner::PopFreeResource()
{
    if (SFreeResources.Num() == 0)
    {
        return nullptr;
    }

    TWeakObjectPtr<AResource> WeakRes = SFreeResources.Pop();
    AResource* Resource = WeakRes.Get();
    if (!IsValid(Resource))
    {
        return nullptr;
    }

    // Claim one player slot on the resource
    Resource->SetPlayer();
    // If resource still has free slots, put it back into pool
    if (Resource->Aviable())
    {
        UStaticMeshComponent* Mesh = Resource->FindComponentByClass<UStaticMeshComponent>();
        FVector BaseLocation = SpawnerCenterLocation.IsZero() ? GetActorLocation() : SpawnerCenterLocation;
        FVector SpawnLocation = BaseLocation + FVector(
            FMath::FRandRange(-SpawnAreaSize.X, SpawnAreaSize.X),
            FMath::FRandRange(-SpawnAreaSize.Y, SpawnAreaSize.Y),
            110.0f
        );        
		Resource->SetActorLocation(SpawnLocation);
        if (Mesh)
        {
            Mesh->SetWorldLocation(SpawnLocation);
            Mesh->SetSimulatePhysics(false);
        }
        SFreeResources.Add(Resource);
    }

    return Resource;
}

void ASpawner::PushFreePlayer(AStrategyPlayer* Player)
{
    if (!IsValid(Player))
    {
        return;
    }

    SFreePlayers.Add(Player);
    UE_LOG(LogTemp, Log, TEXT("Player free [%s]: Players Freed %d"),
        *Player->GetName(), SFreePlayers.Num());

    if (bSpawningAgents)
    {
        return;
    }

    const int32 TotalAgents = SAliveAgresives.Num() + SAlivePasives.Num();
    if (TotalAgents <= 0)
    {
        return;
    }

    if (SFreePlayers.Num() < TotalAgents)
    {
        return;
    }

    // Schedule a non-blocking delayed call instead of blocking the game thread
    if (UWorld* World = GetWorld())
    {
        // Avoid scheduling multiple timers if one is already active
        if (!World->GetTimerManager().IsTimerActive(SpawnPlayersTimerHandle))
        {
            World->GetTimerManager().SetTimer(SpawnPlayersTimerHandle, this, &ASpawner::AssignFreePlayers, 2.0f, false);
        }
    }
}

void ASpawner::AssignFreePlayers()
{
    if (bSpawningAgents)
    {
        return;
    }

    bSpawningAgents = true;

    // Move the free players into a local array so we can operate without holding onto the original container
    TArray<AStrategyPlayer*> PlayersToAssign = SFreePlayers;
    SFreePlayers.Empty();

    for (AStrategyPlayer* FreePlayer : PlayersToAssign)
    {
        if (!IsValid(FreePlayer))
        {
            continue;
        }

        FreePlayer->InitializeAgent(this);
    }

    bSpawningAgents = false;
}

ASpawner::ASpawner()
{
    // Inicializamos variables con valores por defecto seguros
    InitialAgresivePlayers = 5;
    InitialPasivePlayers = 5;
    InitialResourcePlayers = 5;
    SpawnAreaSize = FVector(500.0f, 500.0f, 100.0f);
    SpawnerCenterLocation = FVector::ZeroVector;

}

void ASpawner::BeginPlay()
{
    Super::BeginPlay();
    this->ReadFile();
    this->SpawnAgents();
}

void ASpawner::ReadFile()
{
    FString RutaArchivo = FPaths::ProjectSavedDir() / TEXT("matrix.txt");
    TArray<FString> Lineas;

    // LoadFileToStringArray divide el archivo automáticamente por cada salto de línea
    if (FFileHelper::LoadFileToStringArray(Lineas, *RutaArchivo))
    {
        for (const FString& Linea : Lineas)
        {
            UE_LOG(LogTemp, Log, TEXT("Línea leida: %s"), *Linea);
            if(Linea.StartsWith(TEXT("v=")))
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

float ASpawner::ApplyOp(char InOp, float B, float A)
{
    switch (InOp) {
    case '+': return A + B;
    case '-': return A - B;
    case '*': return A * B;
    case '/':
        if (B == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Division by zero in ASpawner::ApplyOp for expression with operator '%c'"), InOp);
            return 0.0f; // Return a default value or handle error
        }
        return A / B;
    case '^': return FMath::Pow(A, B);
    }
    return 0.0f; // Should not reach here for valid operators
}

int ASpawner::GetPrecedence(char InOp)
{
    if (InOp == '+' || InOp == '-') return 1;
    if (InOp == '*' || InOp == '/') return 2;
    if (InOp == '^') return 3; // Power operator has higher precedence
    return 0; // For '(' or unrecognized characters
}

void ASpawner::ProcessOperator(TArray<char>& OpsStack, TArray<float>& ValuesStack)
{
    if (ValuesStack.Num() < 2 || OpsStack.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("ASpawner::ProcessOperator - Malformed expression: not enough operands or operators. Values: %d, Ops: %d"), ValuesStack.Num(), OpsStack.Num());
        // In a real application, you might want to return a boolean indicating success/failure
        return;
    }

    float B = ValuesStack.Pop(); // Get second operand
    float A = ValuesStack.Pop(); // Get first operand
    char Op = OpsStack.Pop();    // Get operator
    ValuesStack.Push(ApplyOp(Op, B, A));
}

float ASpawner::ShuntingYard(const FString& Expression)
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
                UE_LOG(LogTemp, Error, TEXT("ASpawner::ShuntingYard - Mismatched parentheses in expression: %s (Missing opening parenthesis)"), *Expression);
                return 0.0f; // Error: Mismatched parentheses
            }

            OpsStack.Pop(); // Pop the '('
            CurrentIndex++;
            continue;
        }

        // Unrecognized character
        UE_LOG(LogTemp, Warning, TEXT("ASpawner::ShuntingYard - Unrecognized character '%c' in expression: %s at index %d"), CurrentChar, *Expression, CurrentIndex);
        return 0.0f; // Error: Unrecognized character
    }

    // 5. Process remaining operators
    while (OpsStack.Num() > 0)
    {
        if (OpsStack.Top() == '(')
        {
            UE_LOG(LogTemp, Error, TEXT("ASpawner::ShuntingYard - Mismatched parentheses (remaining '(' on stack) in expression: %s"), *Expression);
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
        UE_LOG(LogTemp, Error, TEXT("ASpawner::ShuntingYard - Malformed expression: Too many values left on stack for expression: %s"), *Expression);
    }
    else // ResultsStack.Num() == 0
    {
        UE_LOG(LogTemp, Error, TEXT("ASpawner::ShuntingYard - Expression resulted in no value: %s"), *Expression);
    }

    return 0.0f;
}

void ASpawner::SpawnAgents()
{
    UE_LOG(LogTemp, Log, TEXT("initial agresive: %d, initial pasive: %d"), InitialAgresivePlayers, InitialPasivePlayers);
    UWorld* World = GetWorld();
    if (!World) return;

    // Si no has configurado SpawnerCenterLocation, usamos la posición del Actor en el mapa
    FVector BaseLocation = SpawnerCenterLocation.IsZero() ? GetActorLocation() : SpawnerCenterLocation;
    FRotator SpawnRotation = FRotator::ZeroRotator;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    InitialResourcePlayers = FMath::Max((InitialAgresivePlayers + InitialPasivePlayers) / 2, 1); // Aseguramos al menos un recurso

    for (int32 x = 0; x < InitialResourcePlayers; x++)
    {
        // Validamos que tengamos una clase válida asignada antes de spawnear
        if (ResourceClass)
        {
			AResource* NewResource = World->SpawnActor<AResource>(ResourceClass, BaseLocation, SpawnRotation, SpawnParams);
            if (NewResource)
            {
                this->SFreeResources.Push(NewResource);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s"),
                    *GetName(), *NewResource->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Spawner [%s]: No se ha asignado una PasiveClass en el Blueprint."), *GetName());
            break;
        }
    }
    int32 ITotalAgents = InitialAgresivePlayers + InitialPasivePlayers;
    int32 ISpawnedAgents = 0;
    int32 IMaxAgresives = InitialAgresivePlayers;
    int32 IMaxPasives = InitialPasivePlayers;
    bool bSpawningAgresives = true;
    while (ISpawnedAgents < ITotalAgents)
    {
        FVector SpawnLocation = BaseLocation + FVector(
            FMath::FRandRange(-SpawnAreaSize.X * 2, SpawnAreaSize.X * 2),
            FMath::FRandRange(-SpawnAreaSize.Y * 2, SpawnAreaSize.Y * 2),
            FMath::FRandRange(50.0f, 50.0f + SpawnAreaSize.Z)
        );
        if (bSpawningAgresives)
        {
            if (IMaxAgresives == 0)
            {
                bSpawningAgresives = false;
                continue;
            }
            AAgresive* NewAgresive = World->SpawnActor<AAgresive>(AgresiveClass, SpawnLocation, SpawnRotation, SpawnParams);

            if (NewAgresive)
            {
                NewAgresive->InitializeAgent(this);
                this->SAliveAgresives.Push(NewAgresive);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s at %s"),
                    *GetName(), *NewAgresive->GetName(), *SpawnLocation.ToString());
                bSpawningAgresives = false;
                IMaxAgresives--;
            }
            else {
                continue;
            }
        }
        else
        {
            if (IMaxPasives == 0)
            {
                bSpawningAgresives = true;
                continue;
			}
            
            APasive* NewPasive = World->SpawnActor<APasive>(PasiveClass, SpawnLocation, SpawnRotation, SpawnParams);

            if (NewPasive)
            {
                NewPasive->InitializeAgent(this);
                this->SAlivePasives.Push(NewPasive);
                UE_LOG(LogTemp, Log, TEXT("Spawner [%s]: Successfully spawned %s at %s"),
                    *GetName(), *NewPasive->GetName(), *SpawnLocation.ToString());
                bSpawningAgresives = true;
                IMaxPasives--;
            }
            else {
                continue;
            }
        }
        ISpawnedAgents++;
    }
}
