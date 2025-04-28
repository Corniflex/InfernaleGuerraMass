// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SpatialHashGridProcessor.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API USpatialHashGridProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	USpatialHashGridProcessor();
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
private:

	FMassEntityQuery EntityQuery;
	bool bDebug = false;
};
