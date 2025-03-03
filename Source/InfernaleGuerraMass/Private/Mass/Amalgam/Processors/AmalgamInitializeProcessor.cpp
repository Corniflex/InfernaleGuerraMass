// Fill out your copyright notice in the Description page of Project Settings.

#include "Mass/Amalgam/Processors/AmalgamInitializeProcessor.h"

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

//Spawner
#include "Mass/Spawner/AmalgamSpawerParent.h"

//Spatial hash grid
#include <Mass/Collision/SpatialHashGrid.h>

//Niagara
#include "NiagaraFunctionLibrary.h"

// Visual Manager
#include "Manager/AmalgamVisualisationManager.h"

//Misc
#include "Components/SplineComponent.h"
#include <Component/ActorComponents/SpawnerComponent.h>
#include "LD/Buildings/BuildingParent.h"
#include "Kismet/GameplayStatics.h"

UAmalgamInitializeProcessor::UAmalgamInitializeProcessor() : EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);

	// Shouldn't run if the visualisation manager wasn't found
	bAutoRegisterWithProcessingPhases = true;

	// Should run on game thread because it is requiered by Niagara to spawn systems
	bRequiresGameThreadExecution = true;
}

void UAmalgamInitializeProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamGridFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamFluxFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamOwnerFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.AddRequirement<FAmalgamMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamAggroFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamFightFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamNiagaraFragment>(EMassFragmentAccess::ReadWrite);

	EntityQuery.AddRequirement<FAmalgamTransmutationFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.AddTagRequirement<FAmalgamInitializeTag>(EMassFragmentPresence::All);

	EntityQuery.RegisterWithProcessor(*this);
}

void UAmalgamInitializeProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
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
	EntityQuery.ForEachEntityChunk(EntityManager, Context, ([this](FMassExecutionContext& Context)
		{

			TArray<AAmalgamSpawnerParent*> Spawners = AAmalgamSpawnerParent::Spawners;

			TArrayView<FTransformFragment> TransformView = Context.GetMutableFragmentView<FTransformFragment>();
			TArrayView<FAmalgamMovementFragment> MovementFragView = Context.GetMutableFragmentView<FAmalgamMovementFragment>();
			TArrayView<FAmalgamAggroFragment> AggroFragView = Context.GetMutableFragmentView<FAmalgamAggroFragment>();
			TArrayView<FAmalgamFightFragment> FightFragView = Context.GetMutableFragmentView<FAmalgamFightFragment>();
			TArrayView<FAmalgamOwnerFragment> OwnerFragView = Context.GetMutableFragmentView<FAmalgamOwnerFragment>();
			TArrayView<FAmalgamFluxFragment> FluxFragView = Context.GetMutableFragmentView<FAmalgamFluxFragment>();
			TArrayView<FAmalgamGridFragment> GridFragView = Context.GetMutableFragmentView<FAmalgamGridFragment>();
			TArrayView<FAmalgamNiagaraFragment> NiagaraFragView = Context.GetMutableFragmentView<FAmalgamNiagaraFragment>();
			TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();
			TArrayView<FAmalgamTransmutationFragment> TransmFragView = Context.GetMutableFragmentView<FAmalgamTransmutationFragment>();
			
			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				if(bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("AmalgamInitializeProcessor : Initializing Amalgam %d"), Index));

				if (Index == 0 && bDebug)
				{
					CycleCount++;
					GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, FString::Printf(TEXT("> Server InitCycles : %d"), CycleCount));
				}

				FTransformFragment* TransformFragment = &TransformView[Index];
				FTransform& EntityTransform = TransformFragment->GetMutableTransform();
				FAmalgamFluxFragment& FluxFragment = FluxFragView[Index];

				/*
				* Find closest spawner in order to fill-in the data fragments
				*/

				float MinSpawnerDistance = (Spawners[0]->GetActorLocation() - EntityTransform.GetLocation()).Length();
				int32 ClosestSpawnerIndex = 0;

				for (int32 SpawnerIndex = 1; SpawnerIndex < Spawners.Num(); ++SpawnerIndex)
				{
					float newDist = (Spawners[SpawnerIndex]->GetActorLocation() - EntityTransform.GetLocation()).Length();
					
					ClosestSpawnerIndex = MinSpawnerDistance < newDist ? ClosestSpawnerIndex : SpawnerIndex;
					MinSpawnerDistance = MinSpawnerDistance < newDist ? MinSpawnerDistance : newDist;
				}
				auto CurrentSpawner = Spawners[ClosestSpawnerIndex];
				auto Flux = CurrentSpawner->GetFlux();
				
				if (!Flux)
				{
					GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("AmalgamInitializeProcessor : No Flux Found")));
					Context.Defer().SwapTags<FAmalgamInitializeTag, FAmalgamKillTag>(Context.GetEntity(Index));
					continue;
				}
				
				FluxFragment.SetFlux(Flux);
				FluxFragment.SetSplinePointIndex(0);
				
				// Initialize data fragments
				FAmalgamFightFragment& FightFragment = FightFragView[Index];
				
				FAmalgamNiagaraFragment& NiagaraFragment = NiagaraFragView[Index];
				FAmalgamTransmutationFragment& TransmutationFragment = TransmFragView[Index];
				FAmalgamGridFragment& GridFragment = GridFragView[Index];
				FAmalgamOwnerFragment& OwnerFragment = OwnerFragView[Index];
				
				OwnerFragment.SetOwner(CurrentSpawner->GetOwner());

				if(bDebug) GEngine->AddOnScreenDebugMessage(-1, .5f, FColor::Orange, FString::Printf(TEXT("AmalgamInitializeProcessor : Spawner team is %d"), CurrentSpawner->GetOwner().Team));

				// Reference entity in grid
				FTransform* Transform = &TransformView[Index].GetMutableTransform();
				FVector Location = Transform->GetLocation();
				FIntVector2 GridCoord = ASpatialHashGrid::WorldToGridCoords(Location);
				if (bDebug) GEngine->AddOnScreenDebugMessage(-1, .5f, FColor::Orange, FString::Printf(TEXT("AmalgamInitializeProcessor : Location : %f;%f ; GridCoord: %f;%f"), Location.X, Location.Y, GridCoord.X, GridCoord.Y));

				GridFragment.SetGridCoordinates(GridCoord);

				if(!ASpatialHashGrid::AddEntityToGrid(Location, Context.GetEntity(Index), OwnerFragment.GetOwner(), TransformFragment, TransmutationFragment.GetHealthModifier(FightFragment.GetHealth())))
				{
					if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("AmalgamInitializeProcessor : Failed to add entity to cell"));
					Context.Defer().AddTag<FAmalgamKillTag>(Context.GetEntity(Index));
				}

				VisualisationManager->CreateAndAddToMapP(Context.GetEntity(Index), CurrentSpawner->GetOwner(), Context.GetWorld(), NiagaraFragment.GetSystem(), Location);

				Context.Defer().RemoveTag<FAmalgamInitializeTag>(Context.GetEntity(Index));
				
				StateFragView[Index].SetStateAndNotify(EAmalgamState::FollowPath, Context, Index);
			}
		}));
}