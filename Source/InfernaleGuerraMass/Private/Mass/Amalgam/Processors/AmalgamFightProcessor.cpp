// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Processors/AmalgamFightProcessor.h"

//Tags
#include "Mass/Army/AmalgamTags.h"

//Fragments
#include "Mass/Army/AmalgamFragments.h"

#include "MassCommonFragments.h"

//Processor
#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>
#include <Mass/Collision/SpatialHashGrid.h>

//Damageable Component
#include "Component/ActorComponents/DamageableComponent.h"

UAmalgamFightProcessor::UAmalgamFightProcessor() : EntityQuery(*this)
{
	
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);
	bRequiresGameThreadExecution = true;
}

void UAmalgamFightProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAmalgamAggroFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAmalgamFightFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAmalgamTargetFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAmalgamOwnerFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAmalgamTransmutationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAmalgamPathfindingFragment>(EMassFragmentAccess::ReadOnly);
	
	EntityQuery.AddTagRequirement<FAmalgamFightTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FAmalgamClientExecuteTag>(EMassFragmentPresence::None);

	EntityQuery.RegisterWithProcessor(*this);
}

void UAmalgamFightProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, ([this](FMassExecutionContext& Context)
		{
			TArrayView<FTransformFragment> TransformFragView = Context.GetMutableFragmentView<FTransformFragment>();
			TArrayView<FAmalgamFightFragment> FightFragView = Context.GetMutableFragmentView<FAmalgamFightFragment>();
			TArrayView<FAmalgamTargetFragment> TargetFragView = Context.GetMutableFragmentView<FAmalgamTargetFragment>();
			TArrayView<FAmalgamAggroFragment> AggroFragView = Context.GetMutableFragmentView<FAmalgamAggroFragment>();
			TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();
			TArrayView<FAmalgamOwnerFragment> OwnerFragView = Context.GetMutableFragmentView<FAmalgamOwnerFragment>();
			TArrayView<FAmalgamTransmutationFragment> TransFragView = Context.GetMutableFragmentView<FAmalgamTransmutationFragment>();
			TArrayView<FAmalgamPathfindingFragment> PathFragView = Context.GetMutableFragmentView<FAmalgamPathfindingFragment>();

			float WorldDeltaTime = Context.GetDeltaTimeSeconds();

			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				FAmalgamFightFragment& FightFragment = FightFragView[Index];

				if (!FightFragment.TickAttackTimer(WorldDeltaTime)) continue;

				FVector Location = TransformFragView[Index].GetTransform().GetLocation();
				FAmalgamAggroFragment& AggroFragment = AggroFragView[Index];
				FAmalgamTargetFragment& TargetFragment = TargetFragView[Index];
				FAmalgamStateFragment& StateFragment = StateFragView[Index];
				FAmalgamOwnerFragment& OwnerFragment = OwnerFragView[Index];
				FAmalgamTransmutationFragment& TransFragment = TransFragView[Index];
				FAmalgamPathfindingFragment& PathfindingFragment = PathFragView[Index];

				if(!CheckTargetValidity(TargetFragment, StateFragment, OwnerFragment.GetOwner()))
				{
					StateFragment.SetAggro(EAmalgamAggro::NoAggro);
					StateFragment.SetStateAndNotify(EAmalgamState::FollowPath, Context, Index);
					continue;
				}

				bool bShouldStillAggro = true;

				switch (StateFragment.GetAggro())
				{
				case EAmalgamAggro::Amalgam:
					bShouldStillAggro = ExecuteAmalgamFight(TargetFragment.GetTargetEntityHandle(), TransFragment.GetUnitDamageModifier(FightFragment.GetDamage()), Location, AggroFragment.GetFightRange());
					break;
				
				case EAmalgamAggro::Building:
					bShouldStillAggro = ExecuteBuildingFight(TargetFragment.GetTargetBuilding(), TransFragment.GetBuildingDamageModifier(FightFragment.GetDamage()), OwnerFragment.GetOwner(), Location, AggroFragment.GetFightRange());
					break;

				case EAmalgamAggro::LDElement:
					bShouldStillAggro = ExecuteLDFight(TargetFragment.GetTargetLDElem(), TransFragment.GetBuildingDamageModifier(FightFragment.GetDamage()), OwnerFragment.GetOwner(), Location, AggroFragment.GetFightRange());
					break;

				default:
					continue;
				}

				if (bShouldStillAggro) continue;

				if (StateFragment.GetAggro() == EAmalgamAggro::Amalgam)
					ASpatialHashGrid::GetMutableEntityData(TargetFragment.GetTargetEntityHandle())->AggroCount--;

				StateFragment.SetAggro(EAmalgamAggro::NoAggro);
				StateFragment.SetState(EAmalgamState::FollowPath);
				FightFragment.ResetTimer();
				TargetFragment.ResetTargets();
				PathfindingFragment.bRecoverPath = true;
			}
		}));
}

bool UAmalgamFightProcessor::ExecuteAmalgamFight(FMassEntityHandle TargetHandle, float Damage, FVector AttackerLocation, float AttackerRange)
{
	GridCellEntityData* Data = ASpatialHashGrid::GetMutableEntityData(TargetHandle);

	if ((AttackerLocation - Data->Location).Length() >= AttackerRange) return false;

	Data->DamageEntity(Damage);

	GEngine->AddOnScreenDebugMessage(-1, 2.5, FColor::Orange, FString::Printf(TEXT("Amalgam attacked")));

	if (ASpatialHashGrid::GetEntityData(TargetHandle).EntityHealth <= 0.f)
		return false;

	return true;
}

bool UAmalgamFightProcessor::ExecuteBuildingFight(ABuildingParent* TargetBuilding, float Damage, FOwner AttackOwner, FVector AttackerLocation, float AttackerRange)
{
	if ((AttackerLocation - TargetBuilding->GetActorLocation()).Length() >= AttackerRange) return false;
	
	TargetBuilding->GetDamageableComponent()->DamageHealthOwner(Damage, false, AttackOwner);

	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, TEXT("Amalgam attacked building"));

	if (TargetBuilding->GetOwner().Team == AttackOwner.Team || !TargetBuilding)
		return false;

	return true;
}

bool UAmalgamFightProcessor::ExecuteLDFight(ALDElement* Element, float Damage, FOwner AttackOwner, FVector AttackerLocation, float AttackerRange)
{
	if ((AttackerLocation - Element->GetActorLocation()).Length() >= AttackerRange) return false;

	auto TargetDamageable = Cast<IDamageable>(Element);
	TargetDamageable->DamageHealthOwner(Damage, false, AttackOwner);
	
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, TEXT("Amalgam attacked LD"));

	return true;
}

bool UAmalgamFightProcessor::CheckTargetValidity(FAmalgamTargetFragment& TgtFrag, FAmalgamStateFragment StateFrag, FOwner Owner)
{
	switch (StateFrag.GetAggro())
	{
	case EAmalgamAggro::Amalgam:
		return ASpatialHashGrid::Contains(TgtFrag.GetTargetEntityHandle());

	case EAmalgamAggro::Building:
	{
		ABuildingParent* Tgt = TgtFrag.GetMutableTargetBuilding();
		return Tgt != nullptr && Tgt->GetOwner().Team != Owner.Team;
	}
	case EAmalgamAggro::LDElement:
	{
		ALDElement* Tgt = TgtFrag.GetMutableTargetLDElem();
		return Tgt != nullptr && ASpatialHashGrid::Contains(Tgt->GetActorLocation(), Tgt);
	}
	default:
		return false;
	}
}