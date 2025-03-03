// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/AmalgamMassBubbleInfoClient.h"

// UE Includes
#include "Net/UnrealNetwork.h"
#include "MassCommonFragments.h"

// Infernale Guerra Includes
#include "Net/Serialization/FastArraySerializer.h"

#if UE_REPLICATION_COMPILE_SERVER_CODE

FAmalgamMassFastArrayItem* FAmalgamMassClientBubbleHandler::GetMutableItem(FMassReplicatedAgentHandle Handle)
{
	if (AgentHandleManager.IsValidHandle(Handle))
	{
		const FMassAgentLookupData& LookUpData = AgentLookupArray[Handle.GetIndex()];
		return &(*Agents)[LookUpData.AgentsIdx];
	}
	return nullptr;
}

void FAmalgamMassClientBubbleHandler::MarkItemDirty(FAmalgamMassFastArrayItem& Item) const
{
	Serializer->MarkItemDirty(Item);
}

#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE

void FAmalgamMassClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	TArrayView<FTransformFragment> TransformFragments;
	TArrayView<FAmalgamStateFragment> StateFragments;
	
	// Add the requirements for the query used to grab all the transform fragments
	auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
		{
			AddQueryRequirements(InQuery);
		};

	// Cache the transform fragments
	auto CacheFragmentViewsForSpawnQuery = [&]
	(FMassExecutionContext& InExecContext)
		{
			TransformFragments = InExecContext.GetMutableFragmentView<FTransformFragment>();
			StateFragments = InExecContext.GetMutableFragmentView<FAmalgamStateFragment>();
		};

	// Called when a new entity is spawned. Stores the entity location in the transform fragment
	auto SetSpawnedEntityData = [&]
	(const FMassEntityView& EntityView, const FAmalgamReplicatedAgent& ReplicatedEntity, const int32 EntityIdx)
		{
			FVector EntityLocation = ReplicatedEntity.GetEntityLocation();

			TransformFragments[EntityIdx].GetMutableTransform().SetLocation(EntityLocation);

			StateFragments[EntityIdx].SetState(ReplicatedEntity.GetEntityState());
			StateFragments[EntityIdx].SetAggro(ReplicatedEntity.GetEntityAggro());
		};

	auto PostReplicatedChange = [this](const FMassEntityView& EntityView, const FAmalgamReplicatedAgent& Item)
		{
			PostReplicatedChangeEntity(EntityView, Item);
		};

	// PostReplicatedChangeEntity is called when there are multiples adds without a remove so it's treated as a change
	PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, PostReplicatedChange);
}

void FAmalgamMassClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	PostReplicatedChangeHelper(ChangedIndices, [this](const FMassEntityView& EntityView, const FAmalgamReplicatedAgent& Item)
		{
			PostReplicatedChangeEntity(EntityView, Item);
		});
}

void FAmalgamMassClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FAmalgamReplicatedAgent& Item) const
{
	// Grabs the transform fragment from the entity
	FTransformFragment& TransformFragment = EntityView.GetFragmentData<FTransformFragment>();
	FAmalgamStateFragment StateFragment = EntityView.GetFragmentData<FAmalgamStateFragment>();

	// Sets the transform location with the agent location
	TransformFragment.GetMutableTransform().SetLocation(Item.GetEntityLocation());
	StateFragment.SetState(Item.GetEntityState());
	StateFragment.SetAggro(Item.GetEntityAggro());
}

void FAmalgamMassClientBubbleHandler::AddQueryRequirements(FMassEntityQuery& InQuery) const
{
	InQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	InQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadWrite);
	//InQuery.AddTagRequirement<FAmalgamInitializeTag>(EMassFragmentPresence::None);
}

#endif //UE_REPLICATION_COMPILE_CLIENT_CODE

FAmalgamMassClientBubbleSerializer::FAmalgamMassClientBubbleSerializer()
{
	Bubble.Initialize(Entities, *this);
}

bool FAmalgamMassClientBubbleSerializer::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FAmalgamMassFastArrayItem, FAmalgamMassClientBubbleSerializer>(Entities, DeltaParams, *this);
}

AAmalgamMassClientBubbleInfo::AAmalgamMassClientBubbleInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Serializers.Add(&BubbleSerializer);
}

void AAmalgamMassClientBubbleInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	// Technically, this doesn't need to be PushModel based because it's a FastArray and they ignore it.
	DOREPLIFETIME_WITH_PARAMS_FAST(AAmalgamMassClientBubbleInfo, BubbleSerializer, SharedParams);
}
