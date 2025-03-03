// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Processors/AmalgamClientMoveProcessor.h"

//Tags
#include "Mass/Army/AmalgamTags.h"

//Fragments
#include "Mass/Army/AmalgamFragments.h"
#include "MassCommonFragments.h"

//Processor
#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>

//Subsystem
#include "MassSignalSubsystem.h"

UAmalgamClientMoveProcessor::UAmalgamClientMoveProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = false;
	ExecutionFlags = (int32)EProcessorExecutionFlags::Client | (int32)EProcessorExecutionFlags::Standalone;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);

	// Should run on game thread because it is requiered by Niagara to move systems
	bRequiresGameThreadExecution = true;
}

void UAmalgamClientMoveProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAmalgamNiagaraFragment>(EMassFragmentAccess::ReadWrite);

	EntityQuery.RegisterWithProcessor(*this);
}

/*
* No more processing outside of server
*/
void UAmalgamClientMoveProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [&](FMassExecutionContext& Context)
		{
		});

}
