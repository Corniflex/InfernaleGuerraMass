// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "AmalgamAggroProcessor.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UAmalgamAggroProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
	UAmalgamAggroProcessor();
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;

	float CheckDelay = 2.5f;
	float CheckTimer = 0.f;
};
