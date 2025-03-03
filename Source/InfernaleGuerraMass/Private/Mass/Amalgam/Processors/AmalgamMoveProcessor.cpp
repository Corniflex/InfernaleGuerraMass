// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Processors/AmalgamMoveProcessor.h"

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
#include <Mass/Collision/SpatialHashGrid.h>

#include "Components/SplineComponent.h"
#include "LD/Buildings/BuildingParent.h"

// Misc
#include "Kismet/GameplayStatics.h"

UAmalgamMoveProcessor::UAmalgamMoveProcessor() : EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);

	// Shouldn't run if the visualisation manager wasn't found
	bAutoRegisterWithProcessingPhases = true;

	bRequiresGameThreadExecution = true;
}

void UAmalgamMoveProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamPathfindingFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamAggroFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamFluxFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamNiagaraFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamDirectionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamTransmutationFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.AddTagRequirement<FAmalgamMoveTag>(EMassFragmentPresence::All);
	
	EntityQuery.RegisterWithProcessor(*this);
}

void UAmalgamMoveProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
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
		TArrayView<FTransformFragment> TransformView = Context.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FAmalgamFluxFragment> FluxFragView = Context.GetMutableFragmentView<FAmalgamFluxFragment>();
		TArrayView<FAmalgamMovementFragment> MovementFragView = Context.GetMutableFragmentView<FAmalgamMovementFragment>();
		TArrayView<FAmalgamPathfindingFragment> PathFragView = Context.GetMutableFragmentView<FAmalgamPathfindingFragment>();
		TArrayView<FAmalgamAggroFragment> AggroFragView = Context.GetMutableFragmentView<FAmalgamAggroFragment>();
		TArrayView<FAmalgamTargetFragment> TargetFragView = Context.GetMutableFragmentView<FAmalgamTargetFragment>();
		TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();
		TArrayView<FAmalgamNiagaraFragment> NiagaraFragView = Context.GetMutableFragmentView<FAmalgamNiagaraFragment>();
		TArrayView<FAmalgamDirectionFragment> DirectionFragView = Context.GetMutableFragmentView<FAmalgamDirectionFragment>();
		TArrayView<FAmalgamTransmutationFragment> TransFragView = Context.GetMutableFragmentView<FAmalgamTransmutationFragment>();
		
		const float WorldDeltaTime = Context.GetDeltaTimeSeconds();

		for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
		{
			FAmalgamStateFragment& StateFragment = StateFragView[Index];
			FAmalgamNiagaraFragment& NiagaraFragment = NiagaraFragView[Index];
			FAmalgamDirectionFragment& DirectionFragment = DirectionFragView[Index];
			FAmalgamAggroFragment& AggroFragment = AggroFragView[Index];
			FAmalgamTransmutationFragment& TransFrag = TransFragView[Index];

			FAmalgamFluxFragment& FluxFragment = FluxFragView[Index];
			FAmalgamTargetFragment& TargetFragment = TargetFragView[Index];
			FAmalgamMovementFragment* MovementFragment = &MovementFragView[Index];
			FAmalgamPathfindingFragment& PathFragment = PathFragView[Index];
			FTransformFragment& TransformFragment = TransformView[Index];
			FTransform& Transform = TransformFragment.GetMutableTransform();
			FVector Location = Transform.GetLocation();

			FVector Destination;
			FVector Direction;

			const auto State = StateFragView[Index].GetState();
			bool bSucceeded = true;

			if (PathFragment.FluxUpdateID != FluxFragment.GetFlux()->GetUpdateID())
				PathFragment.CopyPathFromFlux(FluxFragment.GetFlux(), Location, false);

			if (PathFragment.bRecoverPath)
				PathFragment.RecoverPath(Location);

			FVector TargetLocation;

			switch (State)
			{
			case EAmalgamState::FollowPath:
				bSucceeded = FollowPath(TransformFragment, FluxFragment, PathFragment, DirectionFragment, TransFrag.GetSpeedModifier(MovementFragment->GetSpeed()), WorldDeltaTime);
				break;

			case EAmalgamState::Aggroed:
				TargetLocation = GetTargetLocation(TargetFragment);

				if (TargetLocation == FVector::ZeroVector)
				{
					StateFragment.SetAggro(EAmalgamAggro::NoAggro);
					StateFragment.SetStateAndNotify(EAmalgamState::FollowPath, Context, Index);
					continue;
				}
				else if ((Location - TargetLocation).Length() < AggroFragment.GetFightRange())
				{
					StateFragment.SetStateAndNotify(EAmalgamState::Fighting, Context, Index);
					continue;
				}
				
				bSucceeded = FollowTarget(TransformFragment, TargetLocation, DirectionFragment, TransFrag.GetSpeedModifier(MovementFragment->GetSpeed()), PathFragment.AcceptanceRadiusAttack, WorldDeltaTime);
				break;

			case EAmalgamState::Fighting:
				break;

			default:
				bSucceeded = false;
				if (bDebugMove) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("AmalgamMoveProcessor : \n\t Amalgam @Index %d should not be handled")));
				break;
			}

			if(!bSucceeded)
			{
				StateFragment.SetStateAndNotify(EAmalgamState::Killed, Context, Index);
				continue;
			}
			
			VisualisationManager->UpdatePositionP(Context.GetEntity(Index), TransformFragment.GetTransform().GetLocation(), DirectionFragment.Direction);
		}
	}));
}

FVector UAmalgamMoveProcessor::GetDirectionFollow(const FVector Location, FVector& Destination, FAmalgamFluxFragment& FluxFragment)
{
	int32 SplinePointIndex = FluxFragment.GetSplinePointIndex();

	USplineComponent* Spline = FluxFragment.GetFlux()->GetSplineForAmalgamsComponent();
	Destination = Spline->GetSplinePointAt(SplinePointIndex, ESplineCoordinateSpace::World).Position;

	Destination.Z = 0.f;

	return (Destination - Location).GetSafeNormal();
}

FVector UAmalgamMoveProcessor::GetDirectionAggroed(const FVector Location, FVector& Destination, FAmalgamTargetFragment& TargetFragment, FAmalgamAggroFragment& AggroFragment, FAmalgamStateFragment& StateFragment)
{
	switch (StateFragment.GetAggro())
	{
	case EAmalgamAggro::Amalgam:
		Destination = ASpatialHashGrid::GetEntityData(TargetFragment.GetTargetEntityHandle()).Location;
		break;

	case EAmalgamAggro::Building:
		if (!TargetFragment.GetTargetBuilding())
		{
			if(bDebugMove) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("MoveProcessor : \n\t Target building is null.")));
			return FVector::ZeroVector;
		}

		Destination = TargetFragment.GetTargetBuilding()->GetActorLocation();
		break;
	}

	Destination.Z = 0.f;

	return (Destination - Location).GetSafeNormal();
}

FVector UAmalgamMoveProcessor::GetTargetLocation(const FAmalgamTargetFragment TargetFrag)
{
	FVector Location = FVector::ZeroVector;
	if (TargetFrag.GetTargetBuilding()) 
	{
		Location = TargetFrag.GetTargetBuilding()->GetActorLocation();
	}
	else if (TargetFrag.GetTargetLDElem())
	{
		Location = TargetFrag.GetTargetLDElem()->GetActorLocation();
	}
	else
	{
		if (!ASpatialHashGrid::Contains(TargetFrag.GetTargetEntityHandle()))
			return FVector::ZeroVector;

		Location = ASpatialHashGrid::GetEntityData(TargetFrag.GetTargetEntityHandle()).Location;
	}

	return Location;
}

bool UAmalgamMoveProcessor::CheckIfPathEnded(FAmalgamFluxFragment& FluxFrag, FVector Location, FVector Destination, EAmalgamState State)
{
	float Distance = (Location - Destination).Length();

	switch (State)
	{
	case EAmalgamState::FollowPath:
		if (!FluxFrag.CheckFluxIsValid()) return false;
		break;

	case EAmalgamState::Aggroed:

		break;

	default:
		break;
	}

	return Distance <= DistanceThreshold;
}

bool UAmalgamMoveProcessor::FollowPath(FTransformFragment& TrsfFrag, FAmalgamFluxFragment& FlxFrag, FAmalgamPathfindingFragment& PathFragment, FAmalgamDirectionFragment& DirFragment, float Speed, const float DeltaTime)
{
	TArray<FVector>& Path = PathFragment.Path;

	FTransform& Transform = TrsfFrag.GetMutableTransform();
	const auto CurrentLocation = TrsfFrag.GetTransform().GetLocation();
	if (Path.Num() == 0)
	{
		return false;
	}

	const FVector TargetLocation = Path[0];
	const auto Direction = TargetLocation - CurrentLocation;
	const auto Distance = Direction.Length();

	if (Distance < PathFragment.AcceptancePathfindingRadius)
	{
		FlxFrag.NextSplinePoint();
		PathFragment.NextPoint();
		return true;
	}

	const auto DirectionNormalized = Direction.GetSafeNormal();
	//Speed = TransmutationComponent->GetEffectUnitSpeed(Speed);
	const auto NewLocation = CurrentLocation + DirectionNormalized * Speed * DeltaTime;
	const auto OldLocation = CurrentLocation;
	Transform.SetLocation(NewLocation);
	
	DirFragment.Direction = DirectionNormalized;

	return true;
	
}

bool UAmalgamMoveProcessor::FollowTarget(FTransformFragment& TrsfFrag, FVector TargetLocation, FAmalgamDirectionFragment& DirFragment, float Speed, float AcceptanceRadiusAttack, const float DeltaTime)
{
	FTransform& Transform = TrsfFrag.GetMutableTransform();
	const auto CurrentLocation = Transform.GetLocation();
	
	const auto Direction = TargetLocation - CurrentLocation;
	const auto Distance = Direction.Length();

	if (Distance < AcceptanceRadiusAttack) return true;

	const auto DirectionNormalized = Direction.GetSafeNormal();
	//Speed = TransmutationComponent->GetEffectUnitSpeed(Speed);
	const auto NewLocation = CurrentLocation + DirectionNormalized * Speed * DeltaTime;
	const auto OldLocation = CurrentLocation;
	Transform.SetLocation(NewLocation);

	DirFragment.Direction = DirectionNormalized;

	return true;
}
