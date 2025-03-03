// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include <Manager/AmalgamVisualisationManager.h>

#include "AmalgamInitializeProcessor.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UAmalgamInitializeProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UAmalgamInitializeProcessor();
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
	
	AAmalgamVisualisationManager* VisualisationManager;

	int32 CycleCount = 0;
	bool bDebug = false;
};
