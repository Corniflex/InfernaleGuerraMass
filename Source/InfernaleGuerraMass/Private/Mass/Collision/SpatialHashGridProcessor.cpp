// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Collision/SpatialHashGridProcessor.h"

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

//Spawner
#include "Mass/Spawner/AmalgamSpawerParent.h"

#include <Mass/Collision/SpatialHashGrid.h>

USpatialHashGridProcessor::USpatialHashGridProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);
}

void USpatialHashGridProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamGridFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamTransmutationFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.AddTagRequirement<FAmalgamInitializeTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FAmalgamKillTag>(EMassFragmentPresence::None);

	EntityQuery.RegisterWithProcessor(*this);
}

void USpatialHashGridProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	if (!ASpatialHashGrid::IsValid())
		return;

	EntityQuery.ForEachEntityChunk(EntityManager, Context, ([this](FMassExecutionContext& Context)
		{
			TArrayView<FTransformFragment> TransformView = Context.GetMutableFragmentView<FTransformFragment>();
			TArrayView<FAmalgamGridFragment> GridFragView = Context.GetMutableFragmentView<FAmalgamGridFragment>();
			TArrayView<FAmalgamTargetFragment> TargetFragView = Context.GetMutableFragmentView<FAmalgamTargetFragment>();
			TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();
			TArrayView<FAmalgamTransmutationFragment> TransmFragView = Context.GetMutableFragmentView<FAmalgamTransmutationFragment>();

			const float WorldDeltaTime = Context.GetDeltaTimeSeconds();

			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				FMassEntityHandle EntityHandle = Context.GetEntity(Index);

				FAmalgamGridFragment& GridFragment = GridFragView[Index];
				FTransformFragment& TransformFragment = TransformView[Index];
				FAmalgamStateFragment& StateFragment = StateFragView[Index];
				FAmalgamTransmutationFragment& TransmutationFragment = TransmFragView[Index];

				GridCellEntityData* GridEntityData = ASpatialHashGrid::GetMutableEntityData(EntityHandle);

				if (TransmutationFragment.WasUpdated())
				{
					float NewMax = TransmutationFragment.GetHealthModifier(GridEntityData->MaxEntityHealth);
					GridEntityData->EntityHealth = (GridEntityData->EntityHealth / GridEntityData->MaxEntityHealth) * NewMax;
					GridEntityData->MaxEntityHealth = NewMax;

					TransmutationFragment.Update();
				}

				if (GridEntityData->EntityHealth <= 0.f) 
				{
					StateFragment.SetStateAndNotify(EAmalgamState::Killed, Context, Index);
					continue;
				}


				FVector WorldLocation = TransformFragment.GetTransform().GetLocation();
				

				if (!ASpatialHashGrid::IsInGrid(WorldLocation))
				{
					if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("SHG Processor Error : \n \t Entity out of grid")));
					continue;
				}

				FIntVector2 GridLocation = ASpatialHashGrid::WorldToGridCoords(WorldLocation);
				
				if (!ASpatialHashGrid::IsInGrid(GridLocation))
				{
					if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("SHG Processor Error : \n \t Wrong Grid Coordinates")));
					continue;
				}

				// Check for entity movement since last pass
				if (GridLocation != GridFragment.GetGridCoordinates())
				{
					if (!ASpatialHashGrid::Contains(EntityHandle))
					{
						if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("SHG Processor Error : Entity out of grid."));
						Context.Defer().AddTag<FAmalgamKillTag>(EntityHandle);
						continue;
					}

					if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("SHG Processor : World Location : %f;%f"), WorldLocation.X, WorldLocation.Y));
					
					if (!ASpatialHashGrid::MoveEntityToCell(EntityHandle, WorldLocation))
						if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("SHG Processor Error : Unable to move entity to new cell. %f;%f  => %d; %d"), WorldLocation.X, WorldLocation.Y, GridLocation.X, GridLocation.Y));
					
					ASpatialHashGrid::UpdateCellTransform(EntityHandle, TransformView[Index].GetMutableTransform());
					GridFragment.SetGridCoordinates(GridLocation);
				}
			}
		}));
}
