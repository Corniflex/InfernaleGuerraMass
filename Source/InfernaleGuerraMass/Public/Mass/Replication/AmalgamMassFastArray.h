// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassReplicationTypes.h"
#include "MassClientBubbleHandler.h"
#include "../Army/AmalgamFragments.h"

#include "AmalgamMassFastArray.generated.h"

USTRUCT()
struct FAmalgamReplicatedAgent : public FReplicatedAgentBase
{
	GENERATED_BODY()

	const FVector& GetEntityLocation() const { return EntityLocation; }
	const EAmalgamState& GetEntityState() const { return EntityState; }
	const EAmalgamAggro& GetEntityAggro() const { return EntityAggro; }
	
	void SetEntityLocation(const FVector& InEntityLocation) { EntityLocation = InEntityLocation; }
	void SetEntityState(const EAmalgamState& InEntityState) { EntityState = InEntityState; }
	void SetEntityAggro(const EAmalgamAggro& InEntityAggro) { EntityAggro = InEntityAggro; }
	
private:
	UPROPERTY(Transient)
	FVector_NetQuantize EntityLocation;
	EAmalgamState EntityState;
	EAmalgamAggro EntityAggro;
};

/** Fast array item for efficient agent replication. Remember to make this dirty if any FReplicatedCrowdAgent member variables are modified */
USTRUCT()
struct FAmalgamMassFastArrayItem : public FMassFastArrayItemBase
{
	GENERATED_BODY()

	FAmalgamMassFastArrayItem() = default;
	FAmalgamMassFastArrayItem(const FAmalgamReplicatedAgent& InAgent, const FMassReplicatedAgentHandle InHandle)
		: FMassFastArrayItemBase(InHandle), Agent(InAgent) {
	}

	/** This typedef is required to be provided in FMassFastArrayItemBase derived classes (with the associated FReplicatedAgentBase derived class) */
	typedef FAmalgamReplicatedAgent FReplicatedAgentType;

	UPROPERTY()
	FAmalgamReplicatedAgent Agent;
};