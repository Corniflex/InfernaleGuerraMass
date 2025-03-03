// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Amalgam/Processors/AmalgamClientInitializeProcessor.h"

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

//Spawner
#include "Mass/Spawner/AmalgamSpawerParent.h"

//Niagara
#include "NiagaraFunctionLibrary.h"
#include <GameMode/Infernale/PlayerStateInfernale.h>
#include "Kismet/GameplayStatics.h"

UAmalgamClientInitializeProcessor::UAmalgamClientInitializeProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::Client | (int32)EProcessorExecutionFlags::Standalone;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);

	// Should run on game thread because it is requiered by Niagara to spawn systems
	bRequiresGameThreadExecution = true;
}

void UAmalgamClientInitializeProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAmalgamNiagaraFragment>(EMassFragmentAccess::ReadOnly);
	
	EntityQuery.AddSharedRequirement<FAmalgamInitializeFragment>(EMassFragmentAccess::ReadOnly);

	EntityQuery.AddTagRequirement<FAmalgamClientInitializeTag>(EMassFragmentPresence::All);
	
	EntityQuery.RegisterWithProcessor(*this);
}

/*
* No more processing outside of server
*/
void UAmalgamClientInitializeProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [&](FMassExecutionContext& Context)
		{
			TArrayView<FTransformFragment> TransformFragView = Context.GetMutableFragmentView<FTransformFragment>();
			TArrayView<FAmalgamNiagaraFragment> NiagaraFragView = Context.GetMutableFragmentView<FAmalgamNiagaraFragment>();
			FAmalgamInitializeFragment InitFragment = Context.GetSharedFragment<FAmalgamInitializeFragment>();
			
			for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
			{
				/*FAmalgamNiagaraFragment& NiagaraFragment = NiagaraFragView[Index];
				
				CycleCount++;
				uint8 PlayerTeam = (uint8)((APlayerControllerInfernale*)UGameplayStatics::GetPlayerController(Context.GetWorld(), 0))->GetTeam();
				GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, FString::Printf(TEXT("> ClientInit %d Cycles : %d"), PlayerTeam, CycleCount));

				FVector Location = TransformFragView[Index].GetMutableTransform().GetLocation();

				UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(Context.GetWorld(), InitFragment.NiagaraSystem, Location);
				NiagaraFragment.SetNiagaraComponent(NC);

				Context.Defer().RemoveTag<FAmalgamClientInitializeTag>(Context.GetEntity(Index));*/
			}
		});
}
