// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulBeaconRewardAsset.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API USoulBeaconRewardAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	float HoldActivationDuration; // Duration to hold click on Beacon to activate @todo switch to different DA

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Effect")
	float EffectDuration; //@todo switch to different DA

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Effect")
	float EffectCooldown; //@todo switch to different DA

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
	int32 AmalgamReward = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
	int32 NeutralReward = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
	int32 BossReward = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
	int32 BuildingReward = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
	int32 MainBuildingReward = 0;

};
