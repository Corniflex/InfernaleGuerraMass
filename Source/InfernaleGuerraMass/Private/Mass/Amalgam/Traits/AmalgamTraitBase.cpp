// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Traits/AmalgamTraitBase.h"

// Unreal Includes
#include "Engine/World.h"
#include "MassEntityTemplateRegistry.h"
#include "MassEntityManager.h"
#include "MassSpawnerTypes.h"
#include "MassEntityUtils.h"
#include "MassCommonTypes.h"
#include "MassCommonFragments.h"
#include "MassEntityView.h"
#include "SharedStruct.h"
#include "StructUtils.h"
#include "StructUtilsTypes.h"
#include "Serialization/ArchiveCrc32.h"
#include "StructView.h"

#include "NiagaraFunctionLibrary.h"
#include <MassRepresentationSubsystem.h>

// Custom Includes
#include "Mass/Army/AmalgamFragments.h"
#include "Mass/Army/AmalgamTags.h"


void UAmalgamTraitBase::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	// Add No-Params Fragments
	BuildContext.AddFragment<FTransformFragment>(); 
	BuildContext.AddFragment<FAmalgamFluxFragment>();
	BuildContext.AddFragment<FAmalgamTargetFragment>();
	BuildContext.AddFragment<FAmalgamOwnerFragment>();
	BuildContext.AddFragment<FAmalgamGridFragment>();
	BuildContext.AddFragment<FAmalgamStateFragment>();
	BuildContext.AddFragment<FAmalgamDirectionFragment>();

	// Add Param bound Fragments
	FAmalgamMovementFragment& MvtFrag = BuildContext.AddFragment_GetRef<FAmalgamMovementFragment>();
	FAmalgamAggroFragment& AgrFrag = BuildContext.AddFragment_GetRef<FAmalgamAggroFragment>();
	FAmalgamFightFragment& FghtFrag = BuildContext.AddFragment_GetRef<FAmalgamFightFragment>();
	FAmalgamNiagaraFragment& NiagFrag = BuildContext.AddFragment_GetRef<FAmalgamNiagaraFragment>();
	FAmalgamPathfindingFragment& PathFrag = BuildContext.AddFragment_GetRef<FAmalgamPathfindingFragment>();
	FAmalgamTransmutationFragment& TransmutFrag = BuildContext.AddFragment_GetRef<FAmalgamTransmutationFragment>();
		
	TransmutFrag.Init(World);

	MvtFrag.SetParameters(MovementParams.BaseSpeed, MovementParams.BaseRushSpeed);
	AgrFrag.SetParameters(DetectionParams.BaseDetectionRange, CombatParams.BaseRange, DetectionParams.BaseDetectionAngle, DetectionParams.TargetableRange);
	FghtFrag.SetParameters(HealthParams.BaseHealth, CombatParams.BaseDamage, CombatParams.BaseAttackDelay);
	NiagFrag.SetParameters(NiagaraParams.NiagaraSystem, NiagaraParams.NiagaraHeightOffset, NiagaraParams.NiagaraRotationOffset);
	PathFrag.SetParameters(AcceptanceParams.AcceptancePathfindingRadius, AcceptanceParams.AcceptanceRadiusAttack);

	//FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	//int32 InitParamsHash = UE::StructUtils::GetStructCrc32(FConstStructView::Make(InitParams)); 
	//FSharedStruct InitSharedFragment = EntityManager.GetOrCreateSharedFragmentByHash<FAmalgamInitializeFragment>(InitParamsHash, InitParams);
	//BuildContext.AddSharedFragment(InitSharedFragment);

	
	// Add Fragment to notify the amalgam is ready for initialization
	BuildContext.AddTag<FAmalgamInitializeTag>(); // Data Initialization On Server
}
