// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "AmalgamLifeProcessor.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UAmalgamLifeProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UAmalgamLifeProcessor();
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
private:
	FMassEntityQuery EntityQuery;
};
