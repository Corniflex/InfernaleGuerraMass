// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Observers/AmalgamStateHandlerObserver.h"

//Tags
#include "Mass/Army/AmalgamTags.h"

//Fragments
#include "Mass/Army/AmalgamFragments.h"
#include "MassCommonFragments.h"

//Processor
#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>

UAmalgamStateHandlerObserver::UAmalgamStateHandlerObserver() : EntityQuery(*this)
{
	ObservedType = FAmalgamStateChangeTag::StaticStruct();

	Operation = EMassObservedOperation::Add;

	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
}

void UAmalgamStateHandlerObserver::ConfigureQueries()
{
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadWrite);

	EntityQuery.AddTagRequirement<FAmalgamStateChangeTag>(EMassFragmentPresence::All);

	EntityQuery.RegisterWithProcessor(*this);
}

void UAmalgamStateHandlerObserver::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
		{
			TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();

			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				FAmalgamStateFragment& StateFragment = StateFragView[Index];

				Context.Defer().RemoveTag<FAmalgamInitializeTag>(Context.GetEntity(Index));
				Context.Defer().RemoveTag<FAmalgamInactiveTag>(Context.GetEntity(Index));
				
				switch (StateFragment.GetState())
				{
				case EAmalgamState::Aggroed:
					Context.Defer().AddTag<FAmalgamAggroTag>(Context.GetEntity(Index));
					break;

				case EAmalgamState::Fighting:
					Context.Defer().AddTag<FAmalgamFightTag>(Context.GetEntity(Index));
					Context.Defer().RemoveTag<FAmalgamMoveTag>(Context.GetEntity(Index));
					break;

				case EAmalgamState::FollowPath:
					Context.Defer().AddTag<FAmalgamMoveTag>(Context.GetEntity(Index));
					Context.Defer().RemoveTag<FAmalgamAggroTag>(Context.GetEntity(Index));
					Context.Defer().RemoveTag<FAmalgamFightTag>(Context.GetEntity(Index));
					break;

				case EAmalgamState::Inactive:
					Context.Defer().AddTag<FAmalgamInactiveTag>(Context.GetEntity(Index));
					break;

				default:
					Context.Defer().AddTag<FAmalgamKillTag>(Context.GetEntity(Index));
					break;
				}	
				
				Context.Defer().RemoveTag<FAmalgamStateChangeTag>(Context.GetEntity(Index));
			}
		});
}
