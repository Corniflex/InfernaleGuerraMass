// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mass/Army/AmalgamFragments.h"

#include "AmalgamDataStruct.generated.h"

USTRUCT(BlueprintType)
struct FAmalgamSpawnData
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditAnywhere)
	float Health; 
	UPROPERTY(EditAnywhere)
	float Attack; 
	UPROPERTY(EditAnywhere)
	float Count; 
	UPROPERTY(EditAnywhere)
	float Speed;
	UPROPERTY(EditAnywhere)
	float RushSpeed;
	UPROPERTY(EditAnywhere)
	int AggroRadius; 
	UPROPERTY(EditAnywhere)
	float FightRadius; 
	UPROPERTY(EditAnywhere)
	float AttackDelay;
	UPROPERTY(EditAnywhere)
	float CollisionRadius;
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* NiagaraSystem;

	/*void InitializeDataFragment(FAmalgamData& DataFragment, FAmalgamNiagara& NiagaraFragment)
	{
		DataFragment.LocalMaxHealth = DataFragment.LocalHealth = Health;
		DataFragment.LocalAttack = Attack;
		DataFragment.LocalSpeed = Speed;
		DataFragment.LocalRushSpeed = RushSpeed;
		DataFragment.LocalAggroRadius = AggroRadius;
		DataFragment.LocalMaxFightRadius = FightRadius;
		DataFragment.LocalFightRadius = FightRadius;
		DataFragment.LocalAttackDelay = AttackDelay;
		DataFragment.CollisionRadius = CollisionRadius;

		NiagaraFragment.NiagaraSystem = NiagaraSystem;
	}*/
};