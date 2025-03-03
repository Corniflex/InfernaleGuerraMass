// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"

#include "NiagaraSystem.h"

#include "AmalgamClientInitializeProcessor.generated.h"

/**
 * 
 */

UCLASS()
class INFERNALETESTING_API UAmalgamClientInitializeProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	UAmalgamClientInitializeProcessor();
	
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;

	int32 CycleCount = 0;
	bool bDebug = true;
};
