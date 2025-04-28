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

//Spawner
#include "Mass/Spawner/AmalgamSpawerParent.h"

//Spatial hash grid
#include <Mass/Collision/SpatialHashGrid.h>

// Visual Manager
#include "Manager/AmalgamVisualisationManager.h"

//FogofWarManager
#include "FogOfWar/FogOfWarManager.h"

//Misc
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
	EntityQuery.AddRequirement<FAmalgamSightFragment>(EMassFragmentAccess::ReadWrite);
	
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
	if (!FogManager)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFogOfWarManager::StaticClass(), OutActors);

		if (OutActors.Num() > 0)
			FogManager = static_cast<AFogOfWarManager*>(OutActors[0]);
		else
			FogManager = nullptr;

		//check(FogManager);
		if (!FogManager)
		{
			if(bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InitializeProcessor : Unable to find fog of war manager, skipping execution."));
			//return;
		}
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
			TArrayView<FAmalgamSightFragment> SightFragView = Context.GetMutableFragmentView<FAmalgamSightFragment>();
			
			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				if(bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("AmalgamInitializeProcessor : Initializing Amalgam %d"), Index));

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

					auto Closer = MinSpawnerDistance < newDist;
					ClosestSpawnerIndex = Closer ? ClosestSpawnerIndex : SpawnerIndex;
					MinSpawnerDistance = Closer ? MinSpawnerDistance : newDist;
				}
				auto CurrentSpawner = Spawners[ClosestSpawnerIndex];
				auto Flux = CurrentSpawner->GetFlux();
				
				if (!Flux.IsValid())
				{
					if(bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("AmalgamInitializeProcessor : No Flux Found")));
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
				FAmalgamSightFragment& SightFragment = SightFragView[Index];


				auto StrMult = CurrentSpawner->GetStrengthMultiplier();
				FightFragment.SetStrengthMult(StrMult);

				//FogManager->AddMassEntityVision(Context.GetEntity(Index), SightFragment.GetRange(), SightFragment.GetType());

				OwnerFragment.SetOwner(CurrentSpawner->GetOwner());

				if(bDebug) GEngine->AddOnScreenDebugMessage(-1, .5f, FColor::Orange, FString::Printf(TEXT("AmalgamInitializeProcessor : Spawner team is %d"), CurrentSpawner->GetOwner().Team));



				// Reference entity in grid
				FTransform* Transform = &TransformView[Index].GetMutableTransform();
				FVector Location = Transform->GetLocation();
				FIntVector2 GridCoord = ASpatialHashGrid::WorldToGridCoords(Location);
				if (bDebug) GEngine->AddOnScreenDebugMessage(-1, .5f, FColor::Orange, FString::Printf(TEXT("AmalgamInitializeProcessor : Location : %f;%f ; GridCoord: %f;%f"), Location.X, Location.Y, GridCoord.X, GridCoord.Y));

				GridFragment.SetGridCoordinates(GridCoord);

				if(!ASpatialHashGrid::AddEntityToGrid(Location, Context.GetEntity(Index), OwnerFragment.GetOwner(), TransformFragment, TransmutationFragment.GetHealthModifier(FightFragment.GetHealth()), AggroFragView[Index].GetTargetableRange(), FightFragment.GetEntityType()))
				{
					if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("AmalgamInitializeProcessor : Failed to add entity to cell"));
					Context.Defer().AddTag<FAmalgamKillTag>(Context.GetEntity(Index));
				}
				const auto SpeedMult = Flux->GetAmalgamsSpeedMult();
				const auto Handle = Context.GetEntity(Index);
				
				FDataForSpawnVisual DataForSpawnVisual = FDataForSpawnVisual();
				DataForSpawnVisual.EntityOwner = OwnerFragment.GetOwner();
				DataForSpawnVisual.World = Context.GetWorld();
				DataForSpawnVisual.BPVisualisation = NiagaraFragment.GetBP();
				DataForSpawnVisual.Location = Location;
				DataForSpawnVisual.SpeedMultiplier = SpeedMult;
				DataForSpawnVisual.EntityType = FightFragment.GetEntityType();
				
				if(NiagaraFragment.UseBP())
					VisualisationManager->CreateAndAddToMapP(Handle, DataForSpawnVisual);
				else	
					VisualisationManager->CreateAndAddToMapP(Handle, CurrentSpawner->GetOwner(), Context.GetWorld(), NiagaraFragment.GetSystem().Get(), Location);

				Context.Defer().RemoveTag<FAmalgamInitializeTag>(Handle);
				
				StateFragView[Index].SetStateAndNotify(EAmalgamState::FollowPath, Context, Index);
			}
		}));
}