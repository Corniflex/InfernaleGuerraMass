// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Flux/Flux.h"
#include "NiagaraComponent.h"
#include "Interfaces/Ownable.h"
#include "Components/SplineComponent.h"
#include "Mass/Army/AmalgamTags.h"

#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>

#include <FogOfWar/FogOfWarManager.h>

#include <Kismet/GameplayStatics.h>
#include "Component/PlayerState/TransmutationComponent.h"

#include "LD/LDElement/LDElement.h"

#include "AmalgamFragments.generated.h"

UENUM()
enum EAmalgamState : uint8
{
	Inactive,
	FollowPath,
	Aggroed,
	Fighting,
	Killed
};

UENUM()
enum EAmalgamAggro : uint8
{
	NoAggro,
	Amalgam,
	Building,
	LDElement
};

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

public:
	void SetParameters(float SpeedParam, float RushSpeedParam)
	{ 
		LocalSpeed = SpeedParam;
		LocalRushSpeed = RushSpeedParam;
	}

	float GetSpeed() const { return LocalSpeed; }
	void SetSpeed(float InSpeed) { LocalSpeed = InSpeed; }

	float GetRushSpeed() const { return LocalRushSpeed; }
	void SetRushSpeed(float InRushSpeed) { LocalRushSpeed = InRushSpeed; }
};

/*
* Stores data regarding entity aggro
*/
USTRUCT()
struct FAmalgamAggroFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private: 
	UPROPERTY(EditAnywhere)
	float LocalAggroRange;
	UPROPERTY(EditAnywhere)
	float LocalAggroAngle;

	UPROPERTY(EditAnywhere)
	float LocalMaxFightRange;
	float LocalFightRange;

	UPROPERTY(EditAnywhere)
	float LocalTargetableRange;

public:
	void SetParameters(float AggroRangeParam, float MaxFightRangeParam, float AggroAngle, float TargetableRange)
	{ 
		LocalAggroRange = AggroRangeParam;
		LocalAggroAngle = AggroAngle;
		LocalMaxFightRange = MaxFightRangeParam;
		LocalFightRange = MaxFightRangeParam;
		LocalTargetableRange = TargetableRange;
	}

	float GetAggroRange() const { return LocalAggroRange; }
	void SetAggroRange(float InAggroRange) { LocalAggroRange = InAggroRange; }

	float GetAggroAngle() const { return LocalAggroAngle; }
	void SetAggroAngle(float InAngle) { LocalAggroAngle = InAngle; }

	float GetMaxFightRange() const { return LocalMaxFightRange; }

	float GetFightRange() const { return LocalFightRange; }
	void SetFightRange(float InFightRange) { LocalFightRange = InFightRange; }
};

/*
* Stores data regarding fighting
*/
USTRUCT()
struct FAmalgamFightFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	UPROPERTY(EditAnywhere)
	float LocalHealth;
	UPROPERTY(EditAnywhere)
	float LocalDamage;

	UPROPERTY(EditAnywhere)
	float LocalAttackDelay;
	float LocalAttackTimer = 0.f;

public:
	void SetParameters(float HealthParam, float DamageParam, float DelayParam)
	{
		LocalHealth = HealthParam;
		LocalDamage = DamageParam;
		LocalAttackDelay = DelayParam;
	}

	float GetHealth() const { return LocalHealth; }
	void SetHealth(float InHealth) { LocalHealth = InHealth; }

	float GetDamage() const { return LocalDamage; }
	void SetDamage(float InDamage) { LocalDamage = InDamage; }

	float GetAttackDelay() const { return LocalAttackDelay; }
	void SetAttackDelay(float InAttackDelay) { LocalAttackDelay = InAttackDelay; }

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
	ABuildingParent* TargetBuilding = nullptr;
	ALDElement* TargetLDElem = nullptr;

public:
	FMassEntityHandle GetTargetEntityHandle() const { return TargetEntity; }
	FMassEntityHandle& GetMutableTargetEntityHandle() { return TargetEntity; }
	void SetTargetEntityHandle(FMassEntityHandle InHandle) { TargetEntity = InHandle; }

	ABuildingParent* GetTargetBuilding() const { return TargetBuilding; }
	ABuildingParent* GetMutableTargetBuilding() { return TargetBuilding; }
	void SetTargetBuilding (ABuildingParent* InBuilding) { TargetBuilding = InBuilding; }

	ALDElement* GetTargetLDElem() const { return TargetLDElem; }
	ALDElement* GetMutableTargetLDElem() { return TargetLDElem; }
	void SetTargetLDElem(ALDElement* InLDElement) { TargetLDElem = InLDElement; }

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

	void CopyPathFromFlux(AFlux* FluxTarget, FVector EntityLocation, bool StartAtBeginning)
	{
		Path.Empty();
		if (!FluxTarget) return;

		auto FluxPath = FluxTarget->GetPath();
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

		FluxUpdateID = FluxTarget->GetUpdateID();
	}

	void RecoverPath(FVector EntityLocation)
	{
		int ClosestIndex = -1;
		float SmallestDistance = std::numeric_limits<float>::max();

		for (auto Point : Path)
		{
			float NextDistance = (Point - EntityLocation).Length();
			if (NextDistance > SmallestDistance)
				break;

			ClosestIndex++;
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

	TArray<FVector> Path = TArray<FVector>();

	bool bRecoverPath = false;

	float AcceptancePathfindingRadius;
	float AcceptanceRadiusAttack;

	uint32 FluxUpdateID = -1; // Used to reevaluate path, defaults as -1 so that path is evalutated on first movement iteration
};

USTRUCT()
struct FAmalgamStateFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	EAmalgamState AmalgamState = EAmalgamState::Inactive;
	EAmalgamAggro AggroState = EAmalgamAggro::NoAggro;

	void NotifyContext(FMassExecutionContext& Context, int32 EntityIndexInContext) { Context.Defer().AddTag<FAmalgamStateChangeTag>(Context.GetEntity(EntityIndexInContext)); }

public:
	EAmalgamState GetState() const { return AmalgamState; }
	void SetState(EAmalgamState InState) { AmalgamState = InState; }
	void SetStateAndNotify(EAmalgamState InState, FMassExecutionContext& Context, int32 EntityIndexInContext) { SetState(InState); NotifyContext(Context, EntityIndexInContext); }

	EAmalgamAggro GetAggro() const { return AggroState; }
	void SetAggro(EAmalgamAggro InAggro) { AggroState = InAggro; }
};

/*
* Contains a reference to the flux the amalgam is following
*/
USTRUCT()
struct FAmalgamFluxFragment : public FMassFragment
{
	GENERATED_USTRUCT_BODY()

private:
	AFlux* Flux;
	int32 SplinePointIndex;	

public:
	AFlux* GetFlux() const { return Flux; }
	AFlux* GetMutableFlux() { return Flux; }
	void SetFlux(AFlux* InFlux) { Flux = InFlux; }

	int32 GetSplinePointIndex() const { return SplinePointIndex; }
	void SetSplinePointIndex(int32 InIndex) { SplinePointIndex = InIndex; }
	void NextSplinePoint() { ++SplinePointIndex; }
	bool CheckFluxIsValid() 
	{
		bool bIsFluxInvalid = (!GetFlux());
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
	UTransmutationComponent* TransmutationComponent;
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
	float BaseDamage;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	float BaseAttackDelay;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Combat")
	float BaseRange;
};

USTRUCT()
struct FAmalgamMovementParams : public FMassFragment
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, Category = "Amalgam|Movement")
	float BaseSpeed;
	UPROPERTY(EditAnywhere, Category = "Amalgam|Movement")
	float BaseRushSpeed;
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
	UNiagaraSystem* NiagaraSystem;
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
	UNiagaraSystem* NiagaraSystem;
	float HeightOffset;
	FVector RotationOffset;
	
public:
	void SetParameters(UNiagaraSystem* SystemParam, float HeightOffsetParam, FVector RotOffsetParam)
	{
		NiagaraSystem = SystemParam;
		HeightOffset = HeightOffsetParam;
		RotationOffset = RotOffsetParam;
	}

	UNiagaraSystem* GetSystem() { return NiagaraSystem; }
};