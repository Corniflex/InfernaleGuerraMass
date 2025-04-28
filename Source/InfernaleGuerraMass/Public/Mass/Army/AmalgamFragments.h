// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Flux/Flux.h"
#include "NiagaraComponent.h"
#include "Components/SplineComponent.h"
#include "Mass/Army/AmalgamTags.h"

#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>

#include <FogOfWar/FogOfWarManager.h>

#include <Kismet/GameplayStatics.h>
#include "Component/PlayerState/TransmutationComponent.h"
#include "Enums/Enums.h"

#include "LD/LDElement/LDElement.h"
#include "Structs/SimpleStructs.h"

#include "AmalgamFragments.generated.h"

/*
* Stores data regarding entity movement
*/
USTRUCT()
struct FAmalgamMovementFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	UPROPERTY(EditAnywhere)
	float LocalSpeed;

	UPROPERTY(EditAnywhere)
	float LocalRushSpeed;

	UPROPERTY(EditAnywhere)
	float LocalSpeedMult = 1;

	UPROPERTY(EditAnywhere)
	TMap<ETeam, bool> VisibleBy;
	
public:
	void SetParameters(float SpeedParam, float RushSpeedParam, float SpeedMultParam)
	{ 
		LocalSpeed = SpeedParam;
		LocalRushSpeed = RushSpeedParam;
		LocalSpeedMult = SpeedMultParam;

		VisibleBy = TMap<ETeam, bool>();
		VisibleBy.Add(ETeam::Team1, true);
		VisibleBy.Add(ETeam::Team2, true);
		VisibleBy.Add(ETeam::Team3, true);
		VisibleBy.Add(ETeam::Team4, true);
		VisibleBy.Add(ETeam::NatureTeam, true);
	}

	float GetSpeed() const { return LocalSpeed * LocalSpeedMult; }
	void SetSpeed(float InSpeed) { LocalSpeed = InSpeed; }

	float GetRushSpeed() const { return LocalRushSpeed * LocalSpeedMult; }
	void SetRushSpeed(float InRushSpeed) { LocalRushSpeed = InRushSpeed; }
	
	void SetSpeedMult(float InSpeedMult) { LocalSpeedMult = InSpeedMult; }
	float GetSpeedMult() { return LocalSpeedMult; }

	bool IsVisibleBy(const ETeam InTeam) { return VisibleBy[InTeam]; }
	void SetVisibleBy(const ETeam InTeam, const bool NewVisible) { VisibleBy[InTeam] = NewVisible; }
};

/*
* Stores data regarding entity aggro
*/
USTRUCT()
struct FAmalgamAggroFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private: 
	float LocalAggroRange;
	float LocalAggroAngle;
	float LocalMaxFightRange;
	float LocalFightRange;
	float LocalTargetableRange;

	EAmalgamAggroPriority Priority = EAmalgamAggroPriority::Standard;

	int AggroCount = 0;
	int MaxAggroCount;
	EEntityType LocalEntityType = EEntityType::EntityTypeNone;

public:
	void SetParameters(float AggroRangeParam, float MaxFightRangeParam, float AggroAngle, float TargetableRange, EEntityType EntityType/*, float MaxAggro*/)
	{ 
		LocalAggroRange = AggroRangeParam;
		LocalAggroAngle = AggroAngle;
		LocalMaxFightRange = MaxFightRangeParam;
		LocalFightRange = MaxFightRangeParam;
		LocalTargetableRange = TargetableRange;
		LocalEntityType = EntityType;
	}

	float GetAggroRange() const { return LocalAggroRange; }
	void SetAggroRange(float InAggroRange) { LocalAggroRange = InAggroRange; }

	float GetAggroAngle() const { return LocalAggroAngle; }
	void SetAggroAngle(float InAngle) { LocalAggroAngle = InAngle; }

	float GetMaxFightRange() const { return LocalMaxFightRange; }

	float GetFightRange() const { return LocalFightRange; }
	void SetFightRange(float InFightRange) { LocalFightRange = InFightRange; }

	float GetTargetableRange() const { return LocalTargetableRange; }
	void SetTargetableRange(float InRange) { LocalTargetableRange = InRange; }

	EEntityType GetEntityType() const { return LocalEntityType; }
};

/*
* Stores data regarding fighting
*/
USTRUCT()
struct FAmalgamFightFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	float LocalHealth;
	float LocalDamage;
	float LocalAmalgamDamageMult = 1.0f;
	float LocalBuildingDamageMult = 1.0f;
	float LocalLDDamageMult = 1.0f;
	float LocalBuildingDamage;

	float LocalStrengthMult = 1.0f;
	
	float LocalAttackDelay;
	float LocalAttackTimer = 0.f;
	EEntityType LocalEntityType = EEntityType::EntityTypeNone;

public:
	void SetParameters(float HealthParam, float DamageParam, float DelayParam, float BuildingDamage, EEntityType EntityType)
	{
		LocalHealth = HealthParam * GetStrengthMult();
		LocalDamage = DamageParam;
		LocalBuildingDamage = BuildingDamage;
		LocalAttackDelay = DelayParam;
		LocalEntityType = EntityType;
	}

	void SetMultipliers(float AmalgamMult, float BuildingMult, float LDMult)
	{
		LocalAmalgamDamageMult = AmalgamMult;
		LocalBuildingDamageMult = BuildingMult;
		LocalLDDamageMult = LDMult;
	}

	float GetHealth() const { return LocalHealth; }
	void SetHealth(float InHealth) { LocalHealth = InHealth * GetStrengthMult(); }

	float GetDamage() const { return LocalDamage * GetStrengthMult(); }
	void SetDamage(float InDamage) { LocalDamage = InDamage; }

	float GetAmalgamMult() const { return LocalAmalgamDamageMult; }
	float GetBuildingMult() const { return LocalBuildingDamageMult; }
	float GetLDMult() const { return LocalLDDamageMult; }

	float GetBuildingDamage() const { return LocalDamage; }
	void SetBuildingDamage(float Damage) { LocalBuildingDamage = Damage; }

	float GetAttackDelay() const { return LocalAttackDelay; }
	void SetAttackDelay(float InAttackDelay) { LocalAttackDelay = InAttackDelay; }

	float GetStrengthMult() const { return LocalStrengthMult; }
	void SetStrengthMult(float InStrengthMult)
	{
		auto Prev = LocalStrengthMult;
		LocalStrengthMult = InStrengthMult;
		auto Ratio = LocalStrengthMult / Prev;
		LocalHealth *= Ratio;
	}

	EEntityType GetEntityType() const
	{
		return LocalEntityType;
	}

	/*
	* Increases the attack timer based on DeltaTimeParameter, if the timer exceeds the attack delay, sets it to 0.
	* @param DeltaTime Time elapsed since last tick in seconds.
	* @return Returns true if the timer has exceeded the AttackDelay, false otherwise.
	*/
	bool TickAttackTimer(float DeltaTime) 
	{ 
		LocalAttackTimer += DeltaTime;
		if (LocalAttackTimer < LocalAttackDelay)
			return false;

		LocalAttackTimer = 0.f;
		return true;
	}

	void ResetTimer() { LocalAttackTimer = 0.f; }
};

USTRUCT()
struct FAmalgamOwnerFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	FOwner Owner;

public:
	FOwner GetOwner() const { return Owner; }
	FOwner& GetMutableOwner() { return Owner; }
	void SetOwner(FOwner InOwner) { Owner = InOwner; }
};

/*
 * Stores targeted entities
 */
USTRUCT()
struct FAmalgamTargetFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	FMassEntityHandle TargetEntity;
	EEntityType TargetEntityType = EEntityType::EntityTypeNone;
	TWeakObjectPtr<ABuildingParent> TargetBuilding = nullptr;
	TWeakObjectPtr<ALDElement> TargetLDElem = nullptr;

	float TotalRangeOffset = TNumericLimits<float>::Max();

public:
	FMassEntityHandle GetTargetEntityHandle() const { return TargetEntity; }
	FMassEntityHandle& GetMutableTargetEntityHandle() { return TargetEntity; }
	void SetTargetEntityHandle(FMassEntityHandle InHandle) { TargetEntity = InHandle; }

	TWeakObjectPtr<ABuildingParent> GetTargetBuilding() const { return TargetBuilding; }
	TWeakObjectPtr<ABuildingParent> GetMutableTargetBuilding() { return TargetBuilding; }
	void SetTargetBuilding(TWeakObjectPtr<ABuildingParent> InBuilding) { TargetBuilding = InBuilding; }

	TWeakObjectPtr<ALDElement> GetTargetLDElem() const { return TargetLDElem; }
	TWeakObjectPtr<ALDElement> GetMutableTargetLDElem() { return TargetLDElem; }
	void SetTargetLDElem(TWeakObjectPtr<ALDElement> InLDElement) { TargetLDElem = InLDElement; }

	float GetTargetRangeOffset(EAmalgamAggro AggroType) const;

	float GetTotalRangeOffset() const { return TotalRangeOffset; }
	void SetTotalRangeOffset(float NewOffset) { TotalRangeOffset = NewOffset; }

	EEntityType GetEntityType() const { return TargetEntityType; }
	void SetEntityType(EEntityType InType) { TargetEntityType = InType; }

	void ResetTargets() 
	{
		TargetEntity = FMassEntityHandle(0, 0);
		TargetBuilding = nullptr;
		TargetLDElem = nullptr;
	}
};

USTRUCT()
struct FAmalgamPathfindingFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()
public:
	void SetParameters(float AccpetancePathfindingRadiusParam, float AcceptanceRadiusAttackParam)
	{
		AcceptancePathfindingRadius = AccpetancePathfindingRadiusParam;
		AcceptanceRadiusAttack = AcceptanceRadiusAttackParam;
	}

	void CopyPathFromFlux(TWeakObjectPtr<AFlux> FluxTarget, FVector EntityLocation, bool StartAtBeginning)
	{
		if (!FluxTarget.IsValid()) return;
		
		Path.Empty();

		AFlux* FluxPtr = FluxTarget.Get();

		auto FluxPath = FluxPtr->GetPath();
		if (FluxPath.Num() == 0) return;

		auto StartIndex = StartAtBeginning ? 0 : -1;
		if (!StartAtBeginning)
		{
			const auto Location = EntityLocation;
			auto ClosestPoint = FluxPath[0];
			auto ClosestDistance = FVector::Dist(Location, ClosestPoint);
			StartIndex = 0;
			for (int i = 1; i < FluxPath.Num(); i++)
			{
				const auto Distance = FVector::Dist(Location, FluxPath[i]);
				if (Distance >= ClosestDistance) continue;
				ClosestDistance = Distance;
				ClosestPoint = FluxPath[i];
				StartIndex = i;
			}
		}

		Path = TArray<FVector>();
		for (int i = StartIndex; i < FluxPath.Num(); i++)
		{
			Path.Add(FluxPath[i]);
		}

		FluxUpdateID = FluxPtr->GetUpdateID();
		FluxUpdateVersion = FluxPtr->GetUpdateVersion();
	}

	void RecoverPath(FVector EntityLocation)
	{
		if (Path.Num() == 0) return;

		int ClosestIndex = -1;
		float SmallestDistance = TNumericLimits<float>::Max();

		/*for (FVector& Point : Path)
		{
			float NextDistance = (Point - EntityLocation).Length();
			if (NextDistance > SmallestDistance)
				break;

			ClosestIndex++;
			SmallestDistance = NextDistance;
		}*/

		for (int32 Index = 0; Index < Path.Num(); ++Index)
		{
			float NextDistance = (Path[Index] - EntityLocation).Length();
			if (NextDistance > SmallestDistance)
				continue;

			ClosestIndex = Index;
			SmallestDistance = NextDistance;
		}

		TArray<FVector> NewPath(&Path[ClosestIndex], Path.Num() - ClosestIndex);

		Path = NewPath;
		bRecoverPath = false;
	}

	void NextPoint()
	{
		Path.RemoveAt(0);
	}

	bool IsPathFinal() { return bFinalPath; }

	void MakePathFinal()
	{
		bFinalPath = true;
	}

	bool ShouldRecover() { return bRecoverPath; }
	void SetShouldRecover(bool Value) { bRecoverPath = Value; }

	uint32 GetUpdateID() { return FluxUpdateID; }
	uint32 GetUpdateVersion() { return FluxUpdateVersion; }

	float GetAcceptanceAttackRadius() { return AcceptanceRadiusAttack; }
	float GetAcceptancePathfindingRadius() { return AcceptancePathfindingRadius; }

	
public:
	TArray<FVector> Path = TArray<FVector>();

private:

	bool bRecoverPath = false;
	bool bFinalPath = false;

	float AcceptancePathfindingRadius;
	float AcceptanceRadiusAttack;

	uint32 FluxUpdateID = -1; // Used to reevaluate path, defaults as -1 so that path is evalutated on first movement iteration
	uint32 FluxUpdateVersion = -1; // Used to reevaluate path, defaults as -1 so that path is evalutated on first movement iteration
};

USTRUCT()
struct FAmalgamStateFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	EAmalgamState AmalgamState = EAmalgamState::Inactive;
	EAmalgamAggro AggroState = EAmalgamAggro::NoAggro;
	EAmalgamDeathReason DeathReason = EAmalgamDeathReason::NoReason;

	void NotifyContext(FMassExecutionContext& Context, int32 EntityIndexInContext) { Context.Defer().AddTag<FAmalgamStateChangeTag>(Context.GetEntity(EntityIndexInContext)); }

public:
	EAmalgamState GetState() const { return AmalgamState; }
	void SetState(EAmalgamState InState) { AmalgamState = InState; }
	void SetStateAndNotify(EAmalgamState InState, FMassExecutionContext& Context, int32 EntityIndexInContext) 
	{
		if (AmalgamState == EAmalgamState::Killed) return;

		AmalgamState = InState; 
		NotifyContext(Context, EntityIndexInContext); 
	}

	EAmalgamAggro GetAggro() const { return AggroState; }
	void SetAggro(EAmalgamAggro InAggro) { AggroState = InAggro; }
	
	EAmalgamDeathReason GetDeathReason() const { return DeathReason; }
	void SetDeathReason(EAmalgamDeathReason Reason) { DeathReason = Reason; }

	void Kill(EAmalgamDeathReason Reason, FMassExecutionContext& Context, int32 EntityIndexInContext)
	{
		DeathReason = Reason;
		SetStateAndNotify(EAmalgamState::Killed, Context, EntityIndexInContext);
	}
};

/*
* Contains a reference to the flux the amalgam is following
*/
USTRUCT()
struct FAmalgamFluxFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	TWeakObjectPtr<AFlux> Flux;
	int32 SplinePointIndex;	

public:
	TWeakObjectPtr<AFlux> GetFlux() const { return Flux; }
	TWeakObjectPtr<AFlux> GetMutableFlux() { return Flux; }
	void SetFlux(TWeakObjectPtr<AFlux> InFlux) { Flux = InFlux; }

	int32 GetSplinePointIndex() const { return SplinePointIndex; }
	void SetSplinePointIndex(int32 InIndex) { SplinePointIndex = InIndex; }
	void NextSplinePoint() { ++SplinePointIndex; }
	bool CheckFluxIsValid() 
	{
		bool bIsFluxInvalid = (!GetFlux().IsValid());
		bool bIsSplinePointExceeded = SplinePointIndex >= GetFlux()->GetSplineForAmalgamsComponent()->GetNumberOfSplinePoints();
		return (!bIsFluxInvalid && !bIsSplinePointExceeded);
	}
};

/*
* Stores the entity's coordinates in GridSpace (see SpatialHashGrid)
*/
USTRUCT()
struct FAmalgamGridFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	FIntVector2 GridCoordinates;

public:
	FIntVector2 GetGridCoordinates() const { return GridCoordinates; }
	FIntVector2& GetMutableGridCoordinates() { return GridCoordinates; }
	void SetGridCoordinates(FIntVector2 InCoordinates) { GridCoordinates = InCoordinates; }
};

USTRUCT()
struct FAmalgamDirectionFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

public:
	FVector Direction;
};

USTRUCT()
struct FAmalgamTransmutationFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	TWeakObjectPtr<UTransmutationComponent> TransmutationComponent;
	int32 UpdateID = -1;

public:
	void Init(const UWorld& World) 
	{ TransmutationComponent = UGameplayStatics::GetPlayerController(&World, 0)->GetComponentByClass<UTransmutationComponent>(); }

	float GetSpeedModifier(float BaseSpeed) { return TransmutationComponent->GetEffectUnitSpeed(BaseSpeed); }

	float GetUnitDamageModifier(float BaseDamage) { return TransmutationComponent->GetEffectUnitSpeed(BaseDamage); }
	float GetBuildingDamageModifier(float BaseDamage) { return TransmutationComponent->GetEffectUnitSpeed(BaseDamage); }
	float GetMonsterDamageModifier(float BaseDamage) { return TransmutationComponent->GetEffectUnitSpeed(BaseDamage); }

	float GetHealthModifier(float BaseHealth) { return TransmutationComponent->GetEffectUnitSpeed(BaseHealth); }

	float GetSightModifier(float BaseSight) { return TransmutationComponent->GetEffectUnitSpeed(BaseSight); }

	bool WasUpdated() { return UpdateID != TransmutationComponent->GetUpdateID(); }
	void Update() { UpdateID = TransmutationComponent->GetUpdateID(); }
};

USTRUCT()
struct FAmalgamSightFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

public:

	void SetParameters(float InRange, float InAngle, VisionType InVisiontype)
	{
		BaseSightRange = InRange;
		BaseSightAngle = InAngle;
		BaseSightType = InVisiontype;
	}

	float GetRange() { return BaseSightRange; }
	float GetAngle() { return BaseSightAngle; }
	VisionType GetType() { return BaseSightType; }

private:
	float BaseSightRange;
	float BaseSightAngle;
	VisionType BaseSightType;
};

/*
* ------------------------ Parameters Fragments ------------------------
* Used w/ the Traits to give modularity and customizable parameters
*/

USTRUCT()
struct FAmalgamHealthParams : public FMassFragment
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, Category = "Amalgam|Health")
	float BaseHealth;
};

USTRUCT()
struct FAmalgamCombatParams : public FMassFragment
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	TEnumAsByte<EAmalgamAggroPriority> AggroPriority = EAmalgamAggroPriority::Standard;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	float BaseDamage;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	float BaseBuildingDamage;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	float BaseAttackDelay;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	float BaseRange;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	float AmalgamDamageMultiplier = 1.f;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	float BuildingDamageMultiplier = 1.f;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	float LDDamageMultiplier = 1.f;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	EEntityType EntityType = EEntityType::EntityTypeNone;
};

USTRUCT()
struct FAmalgamMovementParams : public FMassFragment
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Amalgam|Movement")
	float BaseSpeed;
	
	UPROPERTY(EditAnywhere, Category = "Amalgam|Movement")
	float BaseRushSpeed;
	
	UPROPERTY(EditAnywhere, Category = "Amalgam|Movement")
	float SpeedMultiplier = 1;
};

USTRUCT()
struct FAmalgamDetectionParams : public FMassFragment
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, Category = "Amalgam|Detection")
	float BaseDetectionRange;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Detection")
	float BaseDetectionAngle;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	float TargetableRange;
};

USTRUCT()
struct FAmalgamSightParams : public FMassFragment
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, Category = "Amalgam|Sight")
	float BaseSightRange;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Sight")
	float BaseSightAngle;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Sight")
	VisionType BaseSightType;
};

USTRUCT()
struct FAmalgamNiagaraParams : public FMassFragment
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, Category = "Amalgam|Niagara")
	bool bUseBlueprints = false;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Niagara")
	TWeakObjectPtr<UNiagaraSystem> NiagaraSystem;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Niagara")
	TSubclassOf<AActor> BPVisualisation;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Niagara")
	float NiagaraHeightOffset = 20.f;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Niagara")
	FVector NiagaraRotationOffset = FVector(0.f, -90.f, 0.f);
};

USTRUCT()
struct FAmalgamAcceptanceParams : public FMassFragment
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, Category = "Amalgam|Acceptance")
	float AcceptancePathfindingRadius;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Acceptance")
	float AcceptanceRadiusAttack;
};

USTRUCT()
struct FAmalgamInitializeFragment : public FMassSharedFragment
{
	GENERATED_USTRUCT_BODY()

	FAmalgamInitializeFragment() = default;
	FAmalgamInitializeFragment(const FAmalgamInitializeFragment& InFragment)
	{
		Health = InFragment.Health;
		Attack = InFragment.Attack;
		Count = InFragment.Count;
		Speed = InFragment.Speed;
		RushSpeed = InFragment.RushSpeed;
		AggroRadius = InFragment.AggroRadius;
		FightRadius = InFragment.FightRadius;
		AttackDelay = InFragment.AttackDelay;
		NiagaraSystem = InFragment.NiagaraSystem;
	}

public:
	UPROPERTY(EditAnywhere, Category = "Amalgam | Stats")
	float Health;

	UPROPERTY(EditAnywhere, Category = "Amalgam | Stats")
	float Attack;

	UPROPERTY(EditAnywhere, Category = "Amalgam | Misc")
	int Count; // Used of # of particles inside amalgam, not currently useful

	UPROPERTY(EditAnywhere, Category = "Amalgam | Stats")
	float Speed;

	UPROPERTY(EditAnywhere, Category = "Amalgam | Stats")
	float RushSpeed;

	UPROPERTY(EditAnywhere, Category = "Amalgam | Detection")
	float AggroRadius;

	UPROPERTY(EditAnywhere, Category = "Amalgam | Detection")
	float FightRadius;

	UPROPERTY(EditAnywhere, Category = "Amalgam | Stats")
	float AttackDelay;

	UPROPERTY(EditAnywhere, Category = "Amalgam | Visuals")
	UNiagaraSystem* NiagaraSystem;
};

USTRUCT()
struct FAmalgamNiagaraFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	TWeakObjectPtr<UNiagaraSystem> NiagaraSystem;
	TSubclassOf<AActor> BPVisualisation;
	float HeightOffset;
	FVector RotationOffset;

	bool bUseBlueprint = false;
	
public:
	void SetParameters(TWeakObjectPtr<UNiagaraSystem> SystemParam, float HeightOffsetParam, FVector RotOffsetParam, bool UseBlueprint = false)
	{
		NiagaraSystem = SystemParam;
		HeightOffset = HeightOffsetParam;
		RotationOffset = RotOffsetParam;
		bUseBlueprint = UseBlueprint;
	}

	void SetParameters(TSubclassOf<AActor> BP, float HeightOffsetParam, FVector RotOffsetParam, bool UseBlueprint = true)
	{
		BPVisualisation = BP;
		HeightOffset = HeightOffsetParam;
		RotationOffset = RotOffsetParam;
		bUseBlueprint = UseBlueprint;
	}

	TWeakObjectPtr<UNiagaraSystem> GetSystem() { return NiagaraSystem; }
	TSubclassOf<AActor> GetBP() { return BPVisualisation; }

	bool UseBP() { return bUseBlueprint; }
};