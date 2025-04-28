// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "FogOfWarProcessor.generated.h"

/**
 * 
 */
class AFogOfWarManager;

UCLASS()
class INFERNALETESTING_API UFogOfWarProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
	UFogOfWarProcessor();
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;

	AFogOfWarManager* FogManager;
};
