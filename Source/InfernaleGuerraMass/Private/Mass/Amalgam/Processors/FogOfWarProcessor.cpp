// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Processors/FogOfWarProcessor.h"

//Tags
#include "Mass/Army/AmalgamTags.h"

//Fragments
#include "Mass/Army/AmalgamFragments.h"
#include "MassCommonFragments.h"

//Processor
#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>
#include <MassStateTreeExecutionContext.h>

//Subsystem
#include "MassSignalSubsystem.h"

//FogofWarManager
#include "FogOfWar/FogOfWarManager.h"

// Misc
#include "Kismet/GameplayStatics.h"

UFogOfWarProcessor::UFogOfWarProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);

	bRequiresGameThreadExecution = true;
}

void UFogOfWarProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamDirectionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamSightFragment>(EMassFragmentAccess::ReadOnly);

	EntityQuery.AddTagRequirement<FAmalgamClientExecuteTag>(EMassFragmentPresence::None);

	EntityQuery.RegisterWithProcessor(*this);
}

void UFogOfWarProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	if (!FogManager)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFogOfWarManager::StaticClass(), OutActors);

		if (OutActors.Num() > 0)
			FogManager = static_cast<AFogOfWarManager*>(OutActors[0]);
		else
			FogManager = nullptr;

		//check(FogManager);
		if (!FogManager) return;
	}

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
		{
			TArrayView<FTransformFragment> TransformView = Context.GetMutableFragmentView<FTransformFragment>();

			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				/*FTransform Transform = TransformView[Index].GetTransform();

				FogManager->SetMassEntityVision(Context.GetEntity(Index), Transform.GetLocation());*/
			}
		});
}
