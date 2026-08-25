// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OpTree.generated.h"

struct FNodoArbol
{
	FString Dato;

	TSharedPtr<FNodoArbol> Izquierdo;
	TSharedPtr<FNodoArbol> Derecho;
	
	FNodoArbol(const FString& InDato)
		: Dato(InDato), Izquierdo(nullptr), Derecho(nullptr) {
	}
};


/**
 * Operación: construir árbol desde infix y evaluar pasando valores actuales de `v`, `c`, `m`.
 */
UCLASS()
class TACTICINVASORS_API UOpTree : public UObject
{
	GENERATED_BODY()
	
private:
	TSharedPtr<FNodoArbol> Raiz = nullptr;
	int GetPrecedence(const char& Op) const;
	float V = 0.0f; // Valor de recurso
	float C = 0.0f; // Costo de recurso
	int P = 0; // Poblacion actual
	int T = 0; // Iteracion actual

public:
	// Inserción genérica (no usada por ConstruirDesdeInfix pero útil)
	void Insertar(const FString& Dato, TSharedPtr<FNodoArbol> Nodo);

	// Construir árbol desde expresión infija (ej: "0.5*(v-c)+1-1")
	bool ConstruirDesdeInfix(const FString& Expresion);

	// Evaluar árbol usando valores actuales de variables
	float RealizarOperacion(TSharedPtr<FNodoArbol> Nodo) const;

	// Evaluar desde la raíz (comodidad)
	float EvaluarRaiz() const { return RealizarOperacion(Raiz); }

	FNodoArbol* ObtenerRaiz() const { return Raiz.Get(); }

	float GetV() const { return V; }
	void SetV(float InV) { V = InV; }

	float GetC() const { return C; }
	void SetC(float InC) { C = InC; }

	int GetP() const { return P; }
	void SetP(int InP) { P = InP; }

	int GetT() const { return T; }
	void SetT(int InT) { T = InT; }

};
