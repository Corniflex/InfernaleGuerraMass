// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Processors/AmalgamAggroProcessor.h"

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
#include "MassStateTreeExecutionContext.h"

//Spawner
#include "Mass/Spawner/AmalgamSpawerParent.h"

//Spatial hash grid
#include <Mass/Collision/SpatialHashGrid.h>

//Niagara
#include "NiagaraFunctionLibrary.h"

//Misc
#include "Components/SplineComponent.h"

UAmalgamAggroProcessor::UAmalgamAggroProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);
}

void UAmalgamAggroProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamAggroFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamOwnerFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamDirectionFragment>(EMassFragmentAccess::ReadWrite);

	EntityQuery.AddTagRequirement<FAmalgamAggroTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FAmalgamClientExecuteTag>(EMassFragmentPresence::None);

	EntityQuery.RegisterWithProcessor(*this);
}

void UAmalgamAggroProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
		{
			CheckTimer += Context.GetDeltaTimeSeconds();
			if (!(CheckTimer > CheckDelay)) return;

			CheckTimer = 0.f;

			TArrayView<FTransformFragment> TransformView = Context.GetMutableFragmentView<FTransformFragment>();
			TArrayView<FAmalgamTargetFragment> TargetFragView = Context.GetMutableFragmentView<FAmalgamTargetFragment>();
			TArrayView<FAmalgamAggroFragment> AggroFragView = Context.GetMutableFragmentView<FAmalgamAggroFragment>();
			TArrayView<FAmalgamOwnerFragment> OwnerFragView = Context.GetMutableFragmentView<FAmalgamOwnerFragment>();
			TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();
			TArrayView<FAmalgamDirectionFragment> DirectionFragView = Context.GetMutableFragmentView<FAmalgamDirectionFragment>();

			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				if (StateFragView[Index].GetState() == EAmalgamState::Fighting) continue;

				FTransformFragment& TransformFragment = TransformView[Index];
				FAmalgamTargetFragment& TargetFragment = TargetFragView[Index];
				FAmalgamAggroFragment& AggroFragment = AggroFragView[Index];
				FAmalgamOwnerFragment& OwnerFragment = OwnerFragView[Index];
				FAmalgamDirectionFragment& DirectionFragment = DirectionFragView[Index];

				FVector Location = TransformFragment.GetTransform().GetLocation();

				FAmalgamStateFragment& StateFragment = StateFragView[Index];
				
				// check for entities in fight range before anything else

				FMassEntityHandle PrioritizedEntity = ASpatialHashGrid::FindClosestEntity(Location, AggroFragment.GetFightRange(), 360.f, DirectionFragment.Direction, Context.GetEntity(Index), OwnerFragment.GetOwner().Team);
				
				if (PrioritizedEntity.IsValid())
				{
					DrawDebugLine(Context.GetWorld(), Location, ASpatialHashGrid::GetEntityData(PrioritizedEntity).Location, FColor::Orange, false, 1.5f);
					StateFragment.SetAggro(EAmalgamAggro::Amalgam);
					TargetFragment.SetTargetBuilding(nullptr);
					TargetFragment.SetTargetEntityHandle(PrioritizedEntity);
					GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("AggroProcessor : \n\t Entity prioritized in fight range")));
				}

				FMassEntityHandle FoundEntity = ASpatialHashGrid::FindClosestEntity(Location, AggroFragment.GetAggroRange(), AggroFragment.GetAggroAngle(), DirectionFragment.Direction, Context.GetEntity(Index), OwnerFragment.GetOwner().Team);

				ABuildingParent* FoundBuilding = ASpatialHashGrid::FindClosestBuilding(Location, AggroFragment.GetAggroRange(), AggroFragment.GetAggroAngle(), DirectionFragment.Direction, Context.GetEntity(Index), OwnerFragment.GetOwner().Team);

				ALDElement* FoundLDElem = ASpatialHashGrid::FindClosestLDElement(Location, AggroFragment.GetAggroRange(), AggroFragment.GetAggroAngle(), DirectionFragment.Direction, Context.GetEntity(Index), OwnerFragment.GetOwner().Team);

				if (!FoundEntity.IsValid() && FoundBuilding == nullptr && FoundLDElem == nullptr) continue;

				if (FoundEntity.IsValid())
				{
					DrawDebugLine(Context.GetWorld(), Location, ASpatialHashGrid::GetEntityData(FoundEntity).Location, FColor::Orange, false, 1.5f);
					StateFragment.SetAggro(EAmalgamAggro::Amalgam);
					TargetFragment.SetTargetBuilding(nullptr);
					TargetFragment.SetTargetEntityHandle(FoundEntity);
					GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("AggroProcessor : \n\t Amalgam closest")));
				}
				else if(FoundBuilding != nullptr)
				{
					DrawDebugLine(Context.GetWorld(), Location, FoundBuilding->GetActorLocation(), FColor::Orange, false, 1.5f);
					StateFragment.SetAggro(EAmalgamAggro::Building);
					TargetFragment.SetTargetBuilding(FoundBuilding);
					GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("AggroProcessor : \n\t Building closest @ distance %f"), (Location - FoundBuilding->GetActorLocation()).Length()));
				}
				else
				{
					DrawDebugLine(Context.GetWorld(), Location, FoundLDElem->GetActorLocation(), FColor::Orange, false, 1.5f);
					StateFragment.SetAggro(EAmalgamAggro::LDElement);
					TargetFragment.SetTargetLDElem(FoundLDElem);
					GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("AggroProcessor : \n\t LDElement closest @ distance %f"), (Location - FoundLDElem->GetActorLocation()).Length()));
				}

				StateFragment.SetStateAndNotify(EAmalgamState::Aggroed, Context, Index);
			}
		});
}