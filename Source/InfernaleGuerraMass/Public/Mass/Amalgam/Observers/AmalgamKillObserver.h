// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassObserverProcessor.h"

#include <Manager/AmalgamVisualisationManager.h>

#include "AmalgamKillObserver.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UAmalgamKillObserver : public UMassObserverProcessor
{
	GENERATED_BODY()
	
public:
	UAmalgamKillObserver();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
	AAmalgamVisualisationManager* VisualisationManager;

	bool bDebug = true;
};
