// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//UE includes
#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassCommonFragments.h"

// Project Includes
#include <Manager/AmalgamVisualisationManager.h>
#include "Flux/Flux.h"

#include "AmalgamMoveProcessor.generated.h"

/**
 * 
 */

class AGameModeInfernale;
struct FAmalgamFluxFragment;
struct FAmalgamTargetFragment;
struct FAmalgamAggroFragment;
struct FAmalgamStateFragment;
struct FAmalgamPathfindingFragment;
struct FAmalgamDirectionFragment;

enum EAmalgamState : uint8;

UCLASS()
class INFERNALETESTING_API UAmalgamMoveProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UAmalgamMoveProcessor();
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
private:
	FMassEntityQuery EntityQuery;
	
	AAmalgamVisualisationManager* VisualisationManager;
	AGameModeInfernale* GameModeInfernale;

	float DistanceThreshold = 15.f;

	bool bDebugMove = false;
	bool bDebugEntities = false;

	FVector GetDirectionFollow(const FVector Location, FVector& Destination, FAmalgamFluxFragment& FluxFragment);
	FVector GetDirectionAggroed(const FVector Location, FVector& Destination, FAmalgamTargetFragment& TargetFragment, FAmalgamAggroFragment& AggroFragment, FAmalgamStateFragment& StateFragment);

	FVector GetTargetLocation(const FAmalgamTargetFragment TargetFrag);

	bool CheckIfPathEnded(FAmalgamFluxFragment& FluxFrag, FVector Location, FVector Destination, EAmalgamState State);

	bool FollowPath(FTransformFragment& TrsfFrag, FAmalgamFluxFragment& FlxFrag, FAmalgamPathfindingFragment& PathFragment, FAmalgamDirectionFragment& DirFragment, float Speed, const float DeltaTime);
	bool FollowTarget(FTransformFragment& TrsfFrag, FVector TargetLocation, FAmalgamDirectionFragment& DirFragment, float Speed, float AcceptancePathfindingRadius, const float DeltaTime);
};
