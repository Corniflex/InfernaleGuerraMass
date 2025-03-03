// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "AmalgamFightProcessor.generated.h"

/**
 * 
 */

class ABuildingParent;
class ALDElement;
struct FOwner;
struct FAmalgamTargetFragment;
struct FAmalgamStateFragment;

UCLASS()
class INFERNALETESTING_API UAmalgamFightProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UAmalgamFightProcessor();
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
private:
	FMassEntityQuery EntityQuery;

	/*
	* @param TargetHandle Handle of the attacked entity, used to fetch entity from entities map
	* @param Damage The damage inflicted to the entity
	* @return True if the attack succeded and the entity survives, False otherwise.
	*/
	bool ExecuteAmalgamFight(FMassEntityHandle TargetHandle, float Damage, FVector AttackerLocation, float AttackerRange);

	/*
	* @param TargetBuilding The Building on which the damage are going to be inflicted for capture
	* @param Damage The damage inflicted to the building
	* @param AttackOwner Used to define the attacker's team, in order to change the building's team post capture
	* @return True if the attack succeded, False otherwise. It is important to note that failing means that the buidling was either destroyed or switched teams.
	*/
	bool ExecuteBuildingFight(ABuildingParent* TargetBuilding, float Damage, FOwner AttackOwner, FVector AttackerLocation, float AttackerRange);

	bool ExecuteLDFight(ALDElement* Element, float Damage, FOwner AttackOwner, FVector AttackerLocation, float AttackerRange);

	bool CheckTargetValidity(FAmalgamTargetFragment& TgtFrag, FAmalgamStateFragment StateFrag, FOwner Owner);
};
