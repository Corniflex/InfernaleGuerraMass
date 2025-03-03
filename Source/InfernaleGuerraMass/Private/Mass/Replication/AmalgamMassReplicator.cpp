// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/AmalgamMassReplicator.h"

// UE Includes
#include "MassCommonFragments.h"

// InfernaleGuerra Includes
#include "Mass/Replication/AmalgamMassBubbleInfoClient.h"
#include "Mass/Replication/AmalgamMassFastArray.h"
#include "Mass/Army/AmalgamFragments.h"

void UAmalgamMassReplicator::AddRequirements(FMassEntityQuery& EntityQuery)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadWrite);
}

void UAmalgamMassReplicator::ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE

	// Cached variables used in the other lambda functions
	FMassReplicationSharedFragment* RepSharedFrag = nullptr;
	TConstArrayView<FTransformFragment> TransformFragments;
	TConstArrayView<FAmalgamStateFragment> StateFragments;
	TArrayView<FMassReplicatedAgentFragment> AgentFragments;

	auto CacheViewsCallback = [&](FMassExecutionContext& InContext)
		{
			TransformFragments = InContext.GetFragmentView<FTransformFragment>();
			StateFragments = InContext.GetFragmentView<FAmalgamStateFragment>();
			AgentFragments = InContext.GetMutableFragmentView<FMassReplicatedAgentFragment>();
			RepSharedFrag = &InContext.GetMutableSharedFragment<FMassReplicationSharedFragment>();
		};

	auto AddEntityCallback = [&](FMassExecutionContext& InContext, const int32 EntityIdx, FAmalgamReplicatedAgent& InReplicatedAgent, const FMassClientHandle ClientHandle)
		{
			// Retrieves the bubble of the relevant client
			AAmalgamMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AAmalgamMassClientBubbleInfo>(ClientHandle);

			// Sets the location in the entity agent
			InReplicatedAgent.SetEntityLocation(TransformFragments[EntityIdx].GetTransform().GetLocation());
			InReplicatedAgent.SetEntityState(StateFragments[EntityIdx].GetState());
			InReplicatedAgent.SetEntityAggro(StateFragments[EntityIdx].GetAggro());

			// Adds the new agent in the client bubble
			FAmalgamMassClientBubbleHandler* ClientHandler = static_cast<FAmalgamMassClientBubbleHandler*>(BubbleInfo.GetBubbleSerializer().GetClientHandler());
			return ClientHandler->AddAgent(InContext.GetEntity(EntityIdx), InReplicatedAgent);
		};

	auto ModifyEntityCallback = [&]
	(FMassExecutionContext& InContext, const int32 EntityIdx, const EMassLOD::Type LOD, const double Time, const FMassReplicatedAgentHandle Handle,
		const FMassClientHandle ClientHandle)
		{
			// Grabs the client bubble
			AAmalgamMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AAmalgamMassClientBubbleInfo>(ClientHandle);
			FAmalgamMassClientBubbleHandler* Bubble = static_cast<FAmalgamMassClientBubbleHandler*>(BubbleInfo.GetBubbleSerializer().GetClientHandler());

			// Retrieves the entity agent
			FAmalgamMassFastArrayItem* Item = Bubble->GetMutableItem(Handle);

			bool bMarkItemDirty = false;

			const FVector& EntityLocation = TransformFragments[EntityIdx].GetTransform().GetLocation();
			constexpr float LocationTolerance = 10.0f;
			if (!FVector::PointsAreNear(EntityLocation, Item->Agent.GetEntityLocation(), LocationTolerance))
			{
				// Only updates the agent position if the transform fragment location has changed
				Item->Agent.SetEntityLocation(EntityLocation);
				Item->Agent.SetEntityState(StateFragments[EntityIdx].GetState());
				Item->Agent.SetEntityAggro(StateFragments[EntityIdx].GetAggro());
				bMarkItemDirty = true;
			}

			if (bMarkItemDirty)
			{
				// Marks the agent as dirty so it replicated to the client
				Bubble->MarkItemDirty(*Item);
			}
		};

	auto RemoveEntityCallback = [&](FMassExecutionContext& Context, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
		{
			// Retrieve the client bubble
			AAmalgamMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AAmalgamMassClientBubbleInfo>(ClientHandle);
			FAmalgamMassClientBubbleHandler* Bubble = static_cast<FAmalgamMassClientBubbleHandler*>(BubbleInfo.GetBubbleSerializer().GetClientHandler());

			// Remove the entity agent from the bubble
			Bubble->RemoveAgent(Handle);
		};

	CalculateClientReplication<FAmalgamMassFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}
