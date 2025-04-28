// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Spawner/AmalgamSpawerParent.h"

// UE Imports
#include <Component/ActorComponents/SpawnerComponent.h>

// Infernale Guerra Imports
#include "Mass/Army/AmalgamFragments.h"
#include "LD/Buildings/BuildingParent.h"
#include <Kismet/GameplayStatics.h>

TArray<AAmalgamSpawnerParent*> AAmalgamSpawnerParent::Spawners = TArray<AAmalgamSpawnerParent*>();

AAmalgamSpawnerParent::AAmalgamSpawnerParent()
{
	bReplicates = true;
	NetCullDistanceSquared = 2200049013489586007688871936.0f; // ?
}

void AAmalgamSpawnerParent::DoAmalgamSpawning(const int Index)
{
	SpawnQueue.Enqueue(Index);
	Super::DoSpawning();
	uint8 PlayerTeam = (uint8)((APlayerControllerInfernale*) UGameplayStatics::GetPlayerController(GetWorld(), 0))->GetTeam();
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, FString::Printf(TEXT("Spawning on client %d"), PlayerTeam));
} 

void AAmalgamSpawnerParent::Initialize(USpawnerComponent* NewSpawnerComponent, int32 EntitySpawnCount)
{
	/* 
	*	Either SpawnerComponent or NewSpawnerComponent might be null if the value is replicated and the initialize 
	*	function gets called before the spawner hits BeginPlay)
	*/

	//checkf(this, TEXT("Called Initialize on non existent object"));

	if (!SpawnerComponent && !NewSpawnerComponent)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("AAmalgamSpawnerParent : \n\t Spawner Component is null"));
		return;
	}

	SpawnerComponent = NewSpawnerComponent;

	Owner = SpawnerComponent->GetBuilding()->GetOwner();

	AmalgamData = SpawnerComponent->GetSpawnData();

	Count = EntitySpawnCount;

	EntityTypes = SpawnerComponent->GetEntityTypes();

	SpawnDataGenerators = SpawnerComponent->GetSpawnDataGenerators();

	NiagaraSystem = AmalgamData.NiagaraSystem;

	if(SpawnerComponent->GetOwner()->HasAuthority())
		AAmalgamSpawnerParent::Spawners.Add(this);
}

TWeakObjectPtr<AFlux> AAmalgamSpawnerParent::GetFlux()
{
	if (!CanDequeueSpawnIndex()) return nullptr;
	const int Index = DequeueSpawnIndex();
	if (!SpawnerComponent->GetBuilding()->GetFluxes().IsValidIndex(Index)) return nullptr;
	return SpawnerComponent->GetBuilding()->GetFluxes()[Index];
}

float AAmalgamSpawnerParent::GetStrengthMultiplier()
{
	if (!SpawnerComponent) return 1.f;
	return SpawnerComponent->GetStrengthMultiplier();
}

void AAmalgamSpawnerParent::BeginPlay()
{
	Super::BeginPlay();
	if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple, TEXT("SPAWNED"));
	bAutoSpawnOnBeginPlay = false;
}

void AAmalgamSpawnerParent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AAmalgamSpawnerParent::PostLoad()
{
	Super::PostLoad();
}

void AAmalgamSpawnerParent::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
}

void AAmalgamSpawnerParent::BeginDestroy()
{
	AAmalgamSpawnerParent::Spawners.Remove(this);
	Super::BeginDestroy();
}

void AAmalgamSpawnerParent::SetSpawnerComponent(USpawnerComponent* NewSpawnerComponent)
{
	SpawnerComponent = NewSpawnerComponent;
}

void AAmalgamSpawnerParent::UpdateOwner()
{
	Owner = SpawnerComponent->GetBuilding()->GetOwner();
}

void AAmalgamSpawnerParent::PostRegister()
{
	PostRegisterAllComponents();
}
