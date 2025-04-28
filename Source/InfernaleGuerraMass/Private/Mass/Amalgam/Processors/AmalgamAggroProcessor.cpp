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
			/*CheckTimer += Context.GetDeltaTimeSeconds();
			if (!(CheckTimer > CheckDelay)) return;

			CheckTimer = 0.f;*/

			TArrayView<FTransformFragment> TransformView = Context.GetMutableFragmentView<FTransformFragment>();
			TArrayView<FAmalgamTargetFragment> TargetFragView = Context.GetMutableFragmentView<FAmalgamTargetFragment>();
			TArrayView<FAmalgamAggroFragment> AggroFragView = Context.GetMutableFragmentView<FAmalgamAggroFragment>();
			TArrayView<FAmalgamOwnerFragment> OwnerFragView = Context.GetMutableFragmentView<FAmalgamOwnerFragment>();
			TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();
			TArrayView<FAmalgamDirectionFragment> DirectionFragView = Context.GetMutableFragmentView<FAmalgamDirectionFragment>();

			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				if (StateFragView[Index].GetState() == EAmalgamState::Fighting)
					continue;
				
				FTransformFragment& TransformFragment = TransformView[Index];
				FAmalgamTargetFragment& TargetFragment = TargetFragView[Index];
				FAmalgamAggroFragment& AggroFragment = AggroFragView[Index];
				FAmalgamOwnerFragment& OwnerFragment = OwnerFragView[Index];
				FAmalgamDirectionFragment& DirectionFragment = DirectionFragView[Index];

				FVector Location = TransformFragment.GetTransform().GetLocation();

				FAmalgamStateFragment& StateFragment = StateFragView[Index];
			

				// check for entities in fight range before anything else

				//float DetectionRange = 2800.f;
				float DetectionRange = AggroFragment.GetAggroRange() + AggroFragment.GetTargetableRange();


				/*FMassEntityHandle PrioritizedEntity = ASpatialHashGrid::FindClosestEntity(Location, AggroFragment.GetFightRange(), 360.f, DirectionFragment.Direction, Context.GetEntity(Index), OwnerFragment.GetOwner().Team);

				if (PrioritizedEntity.IsValid())
				{
					if(bDebug) DrawDebugLine(Context.GetWorld(), Location, ASpatialHashGrid::GetEntityData(PrioritizedEntity).Location, FColor::Orange, false, 1.5f);
					StateFragment.SetAggro(EAmalgamAggro::Amalgam);
					TargetFragment.SetTargetBuilding(nullptr);
					TargetFragment.SetTargetLDElem(nullptr);
					TargetFragment.SetTargetEntityHandle(PrioritizedEntity);
					if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("AggroProcessor : \n\t Entity prioritized in fight range")));
					StateFragment.SetStateAndNotify(EAmalgamState::Aggroed, Context, Index);
					continue;
				}*/

				/*FMassEntityHandle FoundEntity = ASpatialHashGrid::FindClosestEntity(Location, DetectionRange, AggroFragment.GetAggroAngle(), DirectionFragment.Direction, Context.GetEntity(Index), OwnerFragment.GetOwner().Team);
				TWeakObjectPtr<ABuildingParent> FoundBuilding = ASpatialHashGrid::FindClosestBuilding(Location, DetectionRange, AggroFragment.GetAggroAngle(), DirectionFragment.Direction, Context.GetEntity(Index), OwnerFragment.GetOwner().Team);
				TWeakObjectPtr<ALDElement> FoundLDElem = ASpatialHashGrid::FindClosestLDElement(Location, DetectionRange, AggroFragment.GetAggroAngle(), DirectionFragment.Direction, Context.GetEntity(Index), OwnerFragment.GetOwner().Team);*/

				FDetectionResult Detected = ASpatialHashGrid::FindClosestElementsInRange(Location, DetectionRange, AggroFragment.GetAggroAngle(), DirectionFragment.Direction, Context.GetEntity(Index));

				float AmalgamDist = TNumericLimits<float>::Max();
				if (Detected.Entity.IsSet())
					AmalgamDist = Detected.EntityDistance - AggroFragment.GetTargetableRange();

				float BuildingDist = TNumericLimits<float>::Max();
				if (Detected.Building.IsValid())
					BuildingDist = Detected.BuildingDistance - AggroFragment.GetTargetableRange();

				float LDDist = TNumericLimits<float>::Max();
				if (Detected.LD.IsValid())
					LDDist = Detected.LDDistance - AggroFragment.GetTargetableRange();

				EAmalgamAggro TypeDetected = ClosestDetected(AmalgamDist, BuildingDist, LDDist);
				
				//ASpatialHashGrid::DebugDetectionCell(Location, DetectionRange);
				//if (Index % 2 == 0)
					//ASpatialHashGrid::DebugDetectionCone(Location, DetectionRange, AggroFragment.GetAggroAngle(), DirectionFragment.Direction, Detected != EAmalgamAggro::NoAggro);
				
				IUnitTargetable* TargetActor = TypeDetected == EAmalgamAggro::Building ? Cast<IUnitTargetable>(Detected.Building.Get()) : Cast<IUnitTargetable>(Detected.LD.Get());
				AggroDetected(TypeDetected, StateFragment, TargetFragment, Detected.Entity, TargetActor, Context, Index);
			}
		});
}

EAmalgamAggro UAmalgamAggroProcessor::ClosestDetected(float AmalgamDist, float BuildingDist, float LDDist)
{
	bool BuildingSmaller = BuildingDist < LDDist;
	float SmallestDist = BuildingSmaller ? BuildingDist : LDDist;
	if (AmalgamDist < SmallestDist)
	{
		return EAmalgamAggro::Amalgam;
	}
	
	if (SmallestDist == TNumericLimits<float>::Max())
	{
		return EAmalgamAggro::NoAggro;
	}

	return BuildingSmaller ? EAmalgamAggro::Building : EAmalgamAggro::LDElement;
}

void UAmalgamAggroProcessor::AggroDetected(EAmalgamAggro Detected, FAmalgamStateFragment& StateFragment, FAmalgamTargetFragment& TargetFragment, FMassEntityHandle Handle, IUnitTargetable* Target, FMassExecutionContext& Context, int32 EntityIndex)
{
	StateFragment.SetAggro(Detected);
	TargetFragment.ResetTargets();

	switch (Detected)
	{
	case EAmalgamAggro::NoAggro:
		StateFragment.SetStateAndNotify(EAmalgamState::FollowPath, Context, EntityIndex);
		return;
	case EAmalgamAggro::Amalgam:
		TargetFragment.SetTargetEntityHandle(Handle);
		break;
	case EAmalgamAggro::Building:
		TargetFragment.SetTargetBuilding(Cast<ABuildingParent>(Target));
		break;
	case EAmalgamAggro::LDElement:
		TargetFragment.SetTargetLDElem(Cast<ALDElement>(Target));
		break;
	default:
		StateFragment.Kill(EAmalgamDeathReason::Error, Context, EntityIndex);
		return;
	}

	StateFragment.SetStateAndNotify(EAmalgamState::Aggroed, Context, EntityIndex);
}
