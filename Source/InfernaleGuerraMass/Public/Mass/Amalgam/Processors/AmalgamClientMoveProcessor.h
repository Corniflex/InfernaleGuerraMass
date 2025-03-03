// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "AmalgamClientMoveProcessor.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UAmalgamClientMoveProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UAmalgamClientMoveProcessor();
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
	int32 CycleCount = 0;
	bool bDebug = true;
};
