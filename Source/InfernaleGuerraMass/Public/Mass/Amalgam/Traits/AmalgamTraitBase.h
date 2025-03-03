// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "MassEntityTraitBase.h"
#include "Mass/Army/AmalgamFragments.h"

#include "AmalgamTraitBase.generated.h"

/**
 * 
 */

USTRUCT()
struct FAmalgamParams {
	GENERATED_USTRUCT_BODY()
};

UCLASS()
class INFERNALETESTING_API UAmalgamTraitBase : public UMassEntityTraitBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	FAmalgamHealthParams HealthParams;
	UPROPERTY(EditAnywhere)
	FAmalgamCombatParams CombatParams;
	UPROPERTY(EditAnywhere)
	FAmalgamMovementParams MovementParams;
	UPROPERTY(EditAnywhere)
	FAmalgamDetectionParams DetectionParams;
	UPROPERTY(EditAnywhere)
	FAmalgamSightParams SightParams;
	UPROPERTY(EditAnywhere)
	FAmalgamNiagaraParams NiagaraParams;
	UPROPERTY(EditAnywhere)
	FAmalgamAcceptanceParams AcceptanceParams;

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
