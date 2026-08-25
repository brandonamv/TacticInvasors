// Fill out your copyright notice in the Description page of Project Settings.

#include "OpTree.h"
#include "Containers/Array.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Char.h"

int UOpTree::GetPrecedence(const char& Op) const
{
	if (Op == '+' || Op == '-') return 1;
	if (Op == '*' || Op == '/') return 2;
	if (Op == '^') return 3; // Power operator has higher precedence
	return 0;
}

void UOpTree::Insertar(const FString& Dato, TSharedPtr<FNodoArbol> Nodo)
{
	// Inserción recursiva simple: primero raiz, luego izquierdo, luego derecho
	if (!Raiz.IsValid())
	{
		Raiz = MakeShared<FNodoArbol>(Dato);
		return;
	}

	if (!Nodo.IsValid())
	{
		return;
	}

	if (!Nodo->Izquierdo.IsValid())
	{
		Nodo->Izquierdo = MakeShared<FNodoArbol>(Dato);
	}
	else if (!Nodo->Derecho.IsValid())
	{
		Nodo->Derecho = MakeShared<FNodoArbol>(Dato);
	}
	else
	{
		Insertar(Dato, Nodo->Izquierdo);
	}
}

static bool IsOperatorChar(TCHAR C)
{
	return C == '+' || C == '-' || C == '*' || C == '/' || C == '^';
}

bool UOpTree::ConstruirDesdeInfix(const FString& Expresion)
{
	// Tokenizar (números, variables, operadores, paréntesis)
	TArray<FString> Tokens;
	const int32 Len = Expresion.Len();
	FString NumberAcc;

	for (int32 i = 0; i < Len; ++i)
	{
		TCHAR CurrChar = Expresion[i];
		if (FChar::IsWhitespace(CurrChar)) continue;

		// digits or decimal point
		if (FChar::IsDigit(CurrChar) || CurrChar == '.')
		{
			NumberAcc.AppendChar(CurrChar);
			if (i + 1 >= Len || !(FChar::IsDigit(Expresion[i + 1]) || Expresion[i + 1] == '.'))
			{
				if (!NumberAcc.IsEmpty())
				{
					Tokens.Add(NumberAcc);
					NumberAcc.Empty();
				}
			}
			continue;
		}

		// posible signo unario +/-
		if ((CurrChar == '+' || CurrChar == '-') && (Tokens.Num() == 0 || Tokens.Last() == "(" || IsOperatorChar(TCHAR(Tokens.Last()[0]))))
		{
			// si sigue un número, agregamos como parte del número
			int32 j = i + 1;
			FString acc;
			bool hasDigit = false;
			if (j < Len && (FChar::IsDigit(Expresion[j]) || Expresion[j] == '.'))
			{
				acc.AppendChar(CurrChar);
				while (j < Len && (FChar::IsDigit(Expresion[j]) || Expresion[j] == '.'))
				{
					acc.AppendChar(Expresion[j]);
					hasDigit = true;
					++j;
				}
			}
			if (hasDigit)
			{
				Tokens.Add(acc);
				i = j - 1;
				continue;
			}
		}

		// variables (letras) — tratamos cada letra como token (v, c, m)
		if (FChar::IsAlpha(CurrChar))
		{
			FString Var; Var.AppendChar(CurrChar);
			Tokens.Add(Var);
			continue;
		}

		// operadores y paréntesis
		if (IsOperatorChar(CurrChar) || CurrChar == '(' || CurrChar == ')')
		{
			Tokens.Add(FString::Chr(CurrChar));
			continue;
		}

		// ignorar otros caracteres
	}

	// Shunting-yard -> postfix
	TArray<FString> OutputQueue;
	TArray<FString> OpStack;

	for (const FString& Tok : Tokens)
	{
		const TCHAR First = Tok[0];
		// número o variable
		if (FChar::IsDigit(First) || First == '.' || (Tok.Len() > 1 && (Tok[0] == '+' || Tok[0] == '-') && FChar::IsDigit(Tok[1])) || FChar::IsAlpha(First))
		{
			OutputQueue.Add(Tok);
		}
		else if (Tok == "(")
		{
			OpStack.Push(Tok);
		}
		else if (Tok == ")")
		{
			while (OpStack.Num() > 0 && OpStack.Last() != "(")
			{
				OutputQueue.Add(OpStack.Pop());
			}
			if (OpStack.Num() > 0 && OpStack.Last() == "(") OpStack.Pop();
		}
		else if (IsOperatorChar(First))
		{
			char OpChar = (char)First;
			while (OpStack.Num() > 0)
			{
				FString Top = OpStack.Last();
				if (Top == "(") break;
				char TopChar = (char)Top[0];
				int TopPrec = GetPrecedence(TopChar);
				int CurPrec = GetPrecedence(OpChar);
				bool CurRightAssoc = (OpChar == '^');
				if (TopPrec > CurPrec || (TopPrec == CurPrec && !CurRightAssoc))
				{
					OutputQueue.Add(OpStack.Pop());
				}
				else break;
			}
			OpStack.Push(Tok);
		}
	}

	while (OpStack.Num() > 0) OutputQueue.Add(OpStack.Pop());

	// Construir árbol desde postfix
	TArray<TSharedPtr<FNodoArbol>> NodeStack;
	for (const FString& Tok : OutputQueue)
	{
		const TCHAR First = Tok[0];
		if (Tok.Len() == 1 && IsOperatorChar(First))
		{
			if (NodeStack.Num() < 2) return false;
			TSharedPtr<FNodoArbol> Right = NodeStack.Pop();
			TSharedPtr<FNodoArbol> Left = NodeStack.Pop();
			TSharedPtr<FNodoArbol> NewNode = MakeShared<FNodoArbol>(Tok);
			NewNode->Izquierdo = Left;
			NewNode->Derecho = Right;
			NodeStack.Push(NewNode);
		}
		else
		{
			NodeStack.Push(MakeShared<FNodoArbol>(Tok));
		}
	}

	if (NodeStack.Num() == 1)
	{
		Raiz = NodeStack[0];
		return true;
	}
	return false;
}

float UOpTree::RealizarOperacion(TSharedPtr<FNodoArbol> Nodo) const
{
	UE_LOG(LogTemp, Verbose, TEXT("RealizarOperacion: Evaluating node with data '%s'"), *Nodo->Dato);
	if (!Nodo.IsValid()) return 0.0f;

	// hoja => número o variable
	if (!Nodo->Izquierdo.IsValid() && !Nodo->Derecho.IsValid())
	{
		const FString& S = Nodo->Dato;
		// variables simples: v, c, t, p (minúsculas)
		if (S.Equals(TEXT("v"), ESearchCase::IgnoreCase)) return V;
		if (S.Equals(TEXT("c"), ESearchCase::IgnoreCase)) return C;
		if (S.Equals(TEXT("t"), ESearchCase::IgnoreCase)) return T;
		if (S.Equals(TEXT("p"), ESearchCase::IgnoreCase)) return P;
		// número
		return FCString::Atof(*S);
	}

	// nodo operador
	float LeftVal = Nodo->Izquierdo.IsValid() ? RealizarOperacion(Nodo->Izquierdo) : 0.0f;
	float RightVal = Nodo->Derecho.IsValid() ? RealizarOperacion(Nodo->Derecho) : 0.0f;

	const TCHAR Op = Nodo->Dato[0];
	switch (Op)
	{
	case '+': return LeftVal + RightVal;
	case '-': return LeftVal - RightVal;
	case '*': return LeftVal * RightVal;
	case '/':
		if (FMath::IsNearlyZero(RightVal))
		{
			// evitar división por cero -> devolver gran valor o decidir otra política
			return FLT_MAX;
		}
		return LeftVal / RightVal;
	case '^': return FMath::Pow(LeftVal, RightVal);
	default: return 0.0f;
	}
}
