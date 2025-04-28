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
#include "Component/ActorComponents/BattleManagerComponent.h"
#include "Component/ActorComponents/DamageableComponent.h"
#include "Manager/UnitActorManager.h"
#include "UnitAsActor/UnitActor.h"

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

				const auto OwnerInfo = OwnerFragment.GetOwner();
				
				if(!CheckTargetValidity(TargetFragment, StateFragment, OwnerInfo))
				{
					StateFragment.SetAggro(EAmalgamAggro::NoAggro);
					StateFragment.SetStateAndNotify(EAmalgamState::FollowPath, Context, Index);
					continue;
				}

				bool bShouldStillAggro = true;

				switch (StateFragment.GetAggro())
				{
				case EAmalgamAggro::Amalgam:
					{
					FMassEntityHandle TargetHandle = TargetFragment.GetTargetEntityHandle();
					EEntityType Type = ASpatialHashGrid::GetEntityData(TargetHandle).EntityType;
					bShouldStillAggro = ExecuteAmalgamFight(TargetHandle, TransFragment.GetUnitDamageModifier(FightFragment.GetDamage() * FightFragment.GetAmalgamMult()), Location, AggroFragment.GetFightRange(), TargetFragment.GetTotalRangeOffset(), OwnerInfo, AggroFragment.GetEntityType(), Type);
					break;
					}
				
				case EAmalgamAggro::Building:
					bShouldStillAggro = ExecuteBuildingFight(TargetFragment.GetTargetBuilding(), TransFragment.GetBuildingDamageModifier(FightFragment.GetBuildingDamage() * FightFragment.GetBuildingMult()), OwnerFragment.GetOwner(), AggroFragment.GetEntityType(), Location, AggroFragment.GetFightRange(), TargetFragment.GetTotalRangeOffset());
					break;

				case EAmalgamAggro::LDElement:
					bShouldStillAggro = ExecuteLDFight(TargetFragment.GetTargetLDElem(), TransFragment.GetBuildingDamageModifier(FightFragment.GetDamage() * FightFragment.GetLDMult()), OwnerFragment.GetOwner(), AggroFragment.GetEntityType(), Location, AggroFragment.GetFightRange(), TargetFragment.GetTotalRangeOffset());
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
				PathfindingFragment.SetShouldRecover(true);
			}
		}));
}

void UAmalgamFightProcessor::GetBattleManager()
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitActorManager::StaticClass(), OutActors);

	if (OutActors.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("AmalgamFightProcessor : Unable to find UnitActorManager, skipping execution."));
		return;
	}

	auto UnitActorManager = static_cast<AUnitActorManager*>(OutActors[0]);
	if (!UnitActorManager) return;
	
	BattleManager = UnitActorManager->GetBattleManagerComponent().Get();
}

bool UAmalgamFightProcessor::ExecuteAmalgamFight(FMassEntityHandle TargetHandle, float Damage, FVector AttackerLocation, float AttackerRange, float DistanceOffset, FOwner AttackerOwner, EEntityType AttackerType, EEntityType TargetType)
{
	GridCellEntityData* Data = ASpatialHashGrid::GetMutableEntityData(TargetHandle);
	if (!Data) return false;

	if ((AttackerLocation - Data->Location).Length() - DistanceOffset >= AttackerRange) return false;

	Data->DamageEntity(Damage);

	if (!BattleManager) GetBattleManager();

	if (BattleManager)
	{
		FBattleInfo BattleInfo;
		BattleInfo.AttackerUnitType = AttackerType;
		BattleInfo.TargetUnitType = TargetType;
		const auto UnitLoc = AttackerLocation;
		const auto TargetLoc = Data->Location;
		BattleInfo.BattlePositionAttackerWorld = FVector2D(UnitLoc.X, UnitLoc.Y);
		BattleInfo.BattlePositionTargetWorld = FVector2D(TargetLoc.X, TargetLoc.Y);
		BattleInfo.UnitTargetTypeTarget = EUnitTargetType::UTargetUnit;
		BattleInfo.AttackerOwner = AttackerOwner;
		BattleInfo.TargetOwner = Data->Owner;
		BattleManager->AtPosBattleInfo(BattleInfo);
	}

	if(bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5, FColor::Orange, FString::Printf(TEXT("Amalgam attacked")));
	if (ASpatialHashGrid::GetEntityData(TargetHandle).EntityHealth <= 0.f) return false;

	return true;
}

bool UAmalgamFightProcessor::ExecuteBuildingFight(TWeakObjectPtr<ABuildingParent> TargetBuilding, float Damage, FOwner AttackOwner, EEntityType AttackerType, FVector AttackerLocation, float AttackerRange, float DistanceOffset)
{
	if ((AttackerLocation - TargetBuilding->GetActorLocation()).Length() - DistanceOffset >= AttackerRange) return false;
	
	TargetBuilding->GetDamageableComponent()->DamageHealthOwner(Damage, false, AttackOwner);

	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, TEXT("Amalgam attacked building"));

	if (!TargetBuilding.IsValid())
		return false;
	const auto TargetOwner = TargetBuilding->GetOwner();
	if (TargetOwner.Team == AttackOwner.Team)
		return false;

	if (!BattleManager) GetBattleManager();

	if (BattleManager)
	{
		FBattleInfo BattleInfo;
		BattleInfo.AttackerUnitType = AttackerType;
		BattleInfo.TargetUnitType = EEntityType::EntityTypeCity;
		const auto UnitLoc = AttackerLocation;
		const auto TargetLoc = TargetBuilding.Get()->GetActorLocation();
		BattleInfo.BattlePositionAttackerWorld = FVector2D(UnitLoc.X, UnitLoc.Y);
		BattleInfo.BattlePositionTargetWorld = FVector2D(TargetLoc.X, TargetLoc.Y);
		BattleInfo.UnitTargetTypeTarget = EUnitTargetType::UTargetBuilding;
		BattleInfo.AttackerOwner = AttackOwner;
		BattleInfo.TargetOwner = TargetOwner;
		BattleManager->AtPosBattleInfo(BattleInfo);
	}

	return true;
}

//Distance offset is used to not just compare center locations, but also take radius into account
bool UAmalgamFightProcessor::ExecuteLDFight(TWeakObjectPtr<ALDElement> Element, float Damage, FOwner AttackOwner, EEntityType AttackerType, FVector AttackerLocation, float AttackerRange, float DistanceOffset)
{
	if ((AttackerLocation - Element->GetActorLocation()).Length() - DistanceOffset >= AttackerRange) return false;

	auto TargetDamageable = Cast<IDamageable>(Element);
	TargetDamageable->DamageHealthOwner(Damage, false, AttackOwner);
	
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, TEXT("Amalgam attacked LD"));

	if (!BattleManager) GetBattleManager();

	if (BattleManager)
	{
		FOwner NatureOwner = FOwner();
		NatureOwner.Team = ETeam::NatureTeam;
		NatureOwner.Player = EPlayerOwning::Nature;
		
		FBattleInfo BattleInfo;
		BattleInfo.AttackerUnitType = AttackerType;
		BattleInfo.TargetUnitType = Element->GetEntityType();
		const auto UnitLoc = AttackerLocation;
		const auto TargetLoc = Element.Get()->GetActorLocation();
		BattleInfo.BattlePositionAttackerWorld = FVector2D(UnitLoc.X, UnitLoc.Y);
		BattleInfo.BattlePositionTargetWorld = FVector2D(TargetLoc.X, TargetLoc.Y);
		BattleInfo.UnitTargetTypeTarget = EUnitTargetType::UTargetNeutralCamp;
		BattleInfo.AttackerOwner = AttackOwner;
		BattleInfo.TargetOwner = NatureOwner;
		BattleManager->AtPosBattleInfo(BattleInfo);
	}

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
		TWeakObjectPtr<ABuildingParent> Tgt = TgtFrag.GetMutableTargetBuilding();
		return Tgt != nullptr && Tgt->GetOwner().Team != Owner.Team;
	}
	case EAmalgamAggro::LDElement:
	{
		TWeakObjectPtr<ALDElement> Tgt = TgtFrag.GetMutableTargetLDElem();
		return Tgt.IsValid() && ASpatialHashGrid::Contains(Tgt->GetActorLocation(), Tgt);
	}
	default:
		return false;
	}
}