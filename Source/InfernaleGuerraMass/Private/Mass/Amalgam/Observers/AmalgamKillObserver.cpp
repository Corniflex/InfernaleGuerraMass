// Fill out your copyright notice in the Description page of Project Settings.

#include "Mass/Amalgam/Observers/AmalgamKillObserver.h"

//Tags
#include "Mass/Army/AmalgamTags.h"

//Fragments
#include "Mass/Army/AmalgamFragments.h"
#include "MassCommonFragments.h"

//Processor
#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>
#include <Kismet/GameplayStatics.h>

// Grid

#include "Mass/Collision/SpatialHashGrid.h"

UAmalgamKillObserver::UAmalgamKillObserver() : EntityQuery(*this)
{
	ObservedType = FAmalgamKillTag::StaticStruct();

	Operation = EMassObservedOperation::Add;

	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);

	bRequiresGameThreadExecution = true;
}

void UAmalgamKillObserver::ConfigureQueries()
{
	EntityQuery.AddRequirement<FAmalgamTargetFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.AddTagRequirement<FAmalgamKillTag>(EMassFragmentPresence::All);

	EntityQuery.RegisterWithProcessor(*this);
}

void UAmalgamKillObserver::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	if (!VisualisationManager)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAmalgamVisualisationManager::StaticClass(), OutActors);

		if (OutActors.Num() > 0)
			VisualisationManager = static_cast<AAmalgamVisualisationManager*>(OutActors[0]);
		else
			VisualisationManager = nullptr;

		check(VisualisationManager);
	}
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
		{
			TArrayView<FAmalgamTargetFragment> TargetFragView = Context.GetMutableFragmentView<FAmalgamTargetFragment>();
			
			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				FMassEntityHandle Handle = TargetFragView[Index].GetTargetEntityHandle();

				if(ASpatialHashGrid::Contains(Handle))
					ASpatialHashGrid::GetMutableEntityData(Handle)->AggroCount--;

				ASpatialHashGrid::RemoveEntityFromGrid(Context.GetEntity(Index));
				VisualisationManager->RemoveFromMapP(Context.GetEntity(Index));
			}

			if(bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, FString::Printf(TEXT("KillObserver : Cleared %d Entities"), Context.GetNumEntities()));
			Context.Defer().DestroyEntities(Context.GetEntities());
		});
}
