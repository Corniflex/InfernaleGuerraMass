// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Processors/AmalgamLifeProcessor.h"

//Tags
#include "Mass/Army/AmalgamTags.h"

//Fragments
#include "Mass/Army/AmalgamFragments.h"

#include "MassCommonFragments.h"

//Processor
#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>
#include "Mass/Collision/SpatialHashGrid.h"

UAmalgamLifeProcessor::UAmalgamLifeProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);
}

void UAmalgamLifeProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadWrite);

	EntityQuery.AddTagRequirement<FAmalgamInitializeTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FAmalgamKillTag>(EMassFragmentPresence::None);
	
	EntityQuery.RegisterWithProcessor(*this);
}

void UAmalgamLifeProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, ([this](FMassExecutionContext& Context)
		{
			/*TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();

			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				FAmalgamStateFragment StateFrag = StateFragView[Index];
				GridCellEntityData Data = ASpatialHashGrid::GetEntityData(Context.GetEntity(Index));
				if (Data.EntityHealth <= 0)
					StateFrag.SetStateAndNotify(EAmalgamState::Killed, Context, Index);
			}*/
		}));
}
