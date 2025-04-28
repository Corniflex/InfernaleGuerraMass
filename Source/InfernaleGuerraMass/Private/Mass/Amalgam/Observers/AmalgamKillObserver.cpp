// Fill out your copyright notice in the Description page of Project Settings.

#include "Mass/Amalgam/Observers/AmalgamKillObserver.h"

//Tags
#include "Mass/Army/AmalgamTags.h"

//Fragments
#include "Mass/Army/AmalgamFragments.h"
#include "MassCommonFragments.h"

//Processor
#include "MassExecutionContext.h"
#include <MassEntityTemplateRegistry.h>
#include <Kismet/GameplayStatics.h>

// Grid

#include "Mass/Collision/SpatialHashGrid.h"

UAmalgamKillObserver::UAmalgamKillObserver() : EntityQuery(*this)
{
	ObservedType = FAmalgamKillTag::StaticStruct();

	Operation = EMassObservedOperation::Add;

	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);

	bRequiresGameThreadExecution = true;
}

void UAmalgamKillObserver::ConfigureQueries()
{
	EntityQuery.AddRequirement<FAmalgamTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAmalgamStateFragment>(EMassFragmentAccess::ReadOnly);
	
	EntityQuery.AddTagRequirement<FAmalgamKillTag>(EMassFragmentPresence::All);

	EntityQuery.RegisterWithProcessor(*this);
}

void UAmalgamKillObserver::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
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
	/*if (!FogManager)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFogOfWarManager::StaticClass(), OutActors);

		if (OutActors.Num() > 0)
			FogManager = static_cast<AFogOfWarManager*>(OutActors[0]);
		else
			FogManager = nullptr;

		check(FogManager);
	}*/

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
		{
			TArrayView<FAmalgamTargetFragment> TargetFragView = Context.GetMutableFragmentView<FAmalgamTargetFragment>();
			TArrayView<FAmalgamStateFragment> StateFragView = Context.GetMutableFragmentView<FAmalgamStateFragment>();
			
			TMap<TWeakObjectPtr<ASoulBeacon>, int> RewardMap;

			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				EAmalgamDeathReason DeathReason = StateFragView[Index].GetDeathReason();

				FMassEntityHandle Handle = Context.GetEntity(Index);
				FMassEntityHandle TargetHandle = TargetFragView[Index].GetTargetEntityHandle();

				//HashGridCell* Cell = ASpatialHashGrid::GetCellRef(Handle);
				//TWeakObjectPtr<ASoulBeacon> Beacon = ASpatialHashGrid::IsInSoulBeaconRangeByCell(Cell);
				TWeakObjectPtr<ASoulBeacon> Beacon = ASpatialHashGrid::IsInSoulBeaconRange(Handle);
				if (Beacon != nullptr && DeathReason == EAmalgamDeathReason::Eliminated)
				{
					//Beacon->Reward(ESoulBeaconRewardType::RewardAmalgam);
					if (RewardMap.Contains(Beacon)) RewardMap[Beacon]++;
					else RewardMap.Add(Beacon, 1);
				}

				if(ASpatialHashGrid::Contains(TargetHandle))
					ASpatialHashGrid::GetMutableEntityData(TargetHandle)->AggroCount--;

				/*if (FogManager->Contains(Handle))
					FogManager->RemoveMassEntityVision(Handle);*/

				ASpatialHashGrid::RemoveEntityFromGrid(Handle);
				VisualisationManager->RemoveFromMapP(Handle);

				if (DeathReason == EAmalgamDeathReason::Error)
					UE_LOG(LogTemp, Error, TEXT("Error caused amalgam death"));
			}

			TArray<TWeakObjectPtr<ASoulBeacon>> Keys;
			RewardMap.GenerateKeyArray(Keys);
			for (int32 BeaconIndex = 0; BeaconIndex < Keys.Num(); ++BeaconIndex)
			{
				TWeakObjectPtr<ASoulBeacon> Beacon = Keys[BeaconIndex];
				Beacon->RewardMultiple(ESoulBeaconRewardType::RewardAmalgam, RewardMap[Beacon]);
			}

			if(bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, FString::Printf(TEXT("KillObserver : Cleared %d Entities"), Context.GetNumEntities()));
			Context.Defer().DestroyEntities(Context.GetEntities());
		});
}
