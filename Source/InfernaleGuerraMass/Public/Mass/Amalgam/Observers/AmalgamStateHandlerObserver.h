// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassObserverProcessor.h"
#include "AmalgamStateHandlerObserver.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UAmalgamStateHandlerObserver : public UMassObserverProcessor
{
	GENERATED_BODY()
	
public:
	UAmalgamStateHandlerObserver();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
