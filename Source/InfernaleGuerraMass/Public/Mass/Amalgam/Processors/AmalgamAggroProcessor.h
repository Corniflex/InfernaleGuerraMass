// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "AmalgamAggroProcessor.generated.h"

/**
 * 
 */

enum EAmalgamAggro : uint8;
struct FAmalgamStateFragment;
struct FAmalgamTargetFragment;

class IUnitTargetable;

UCLASS()
class INFERNALETESTING_API UAmalgamAggroProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
	UAmalgamAggroProcessor();
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:

	EAmalgamAggro ClosestDetected(float AmalgamDist, float BuildingDist, float LDDist);
	void AggroDetected(EAmalgamAggro Detected, FAmalgamStateFragment& StateFragment, FAmalgamTargetFragment& TargetFragment, FMassEntityHandle Handle, IUnitTargetable* Target, FMassExecutionContext& Context, int32 EntityIndex);

private:
	FMassEntityQuery EntityQuery;

	float CheckDelay = 1.0f;
	float CheckTimer = 0.f;
	bool bDebug = false;
};
