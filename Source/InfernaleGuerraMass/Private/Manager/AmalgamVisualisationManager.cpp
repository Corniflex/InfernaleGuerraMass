// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/AmalgamVisualisationManager.h"

// Niagara includes
#include <NiagaraFunctionLibrary.h>

// Project function library, for getting emissive color from owner
#include <FunctionLibraries/FunctionLibraryInfernale.h>

// Sets default values
AAmalgamVisualisationManager::AAmalgamVisualisationManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	//bReplicates = true;
}

// Called when the game starts or when spawned
void AAmalgamVisualisationManager::BeginPlay()
{
	Super::BeginPlay();
	ElementArray = TArray<FNiagaraVisualElement>();
	
}

// Called every frame
void AAmalgamVisualisationManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!HasAuthority()) return;
	if (Trucs.Num() > 0)
	{
		auto Truc = Trucs[Trucs.Num()-1];
		AddToMapMulticast(Truc.EntityHandle, Truc.NiagaraComponent);
		AddToMapMulticast2();
		Trucs.RemoveAt(Trucs.Num() - 1);
	}
}

void AAmalgamVisualisationManager::AddToMapP(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("AddToMapP: No autority"));
		return;
	}
	Trucs.Add(FTruc(EntityHandle, NiagaraComponent));
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("AddToMapP: added autority"));
}

void AAmalgamVisualisationManager::CreateAndAddToMapP(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("CreateAndAddToMapP: No autority"));
		return;
	}
	CreateAndAddToMapMulticast(EntityHandle, EntityOwner, World, NiagaraSystem, Location);
}

void AAmalgamVisualisationManager::UpdatePositionP(FMassEntityHandle EntityHandle, FVector const Location, FVector const Rotation)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("UpdatePositionP: No autority"));
		return;
	}
	UpdatePositionMulticast(EntityHandle, Location, Rotation);
}

void AAmalgamVisualisationManager::RemoveFromMapP(FMassEntityHandle EntityHandle)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("RemoveFromMap: No autority"));
		return;
	}
	RemoveFromMapMulticast(EntityHandle);
}

void AAmalgamVisualisationManager::AddToMapMulticast_Implementation(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent)
{
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("Added to map"));
	AddToMap(EntityHandle, NiagaraComponent);
}

void AAmalgamVisualisationManager::AddToMapMulticast2_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("Added to map2"));
}

void AAmalgamVisualisationManager::CreateAndAddToMapMulticast_Implementation(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location)
{
	CreateAndAddToMap(EntityHandle, EntityOwner, World, NiagaraSystem, Location);
}

void AAmalgamVisualisationManager::UpdatePositionMulticast_Implementation(FMassEntityHandle EntityHandle, FVector const Location, FVector const Rotation)
{
	UpdateItemPosition(EntityHandle, Location, Rotation);
}

void AAmalgamVisualisationManager::RemoveFromMapMulticast_Implementation(FMassEntityHandle EntityHandle)
{
	RemoveFromMap(EntityHandle);
}

void AAmalgamVisualisationManager::AddToMap(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	checkf(!ContainsElement(HandleAsNumber), TEXT("Map already contains handle."));
	FNiagaraVisualElement NewElement(HandleAsNumber, NiagaraComponent);
	
	ElementArray.Add(NewElement);
}

void AAmalgamVisualisationManager::CreateAndAddToMap(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	checkf(!ContainsElement(HandleAsNumber), TEXT("Map already contains handle."));
	UNiagaraComponent* SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, NiagaraSystem, Location);
	auto Color = UFunctionLibraryInfernale::GetTeamColorCpp(EntityOwner.Team);
	SpawnedComponent->SetColorParameter("Color", Color);
	FNiagaraVisualElement NewElement(HandleAsNumber, SpawnedComponent);

	ElementArray.Add(NewElement);
}

void AAmalgamVisualisationManager::UpdateItemPosition(FMassEntityHandle EntityHandle, FVector const Location, FVector const Rotation)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	checkf(ContainsElement(HandleAsNumber), TEXT("Handle is not in map."));
	UNiagaraComponent* NC = GetNiagaraComponent(HandleAsNumber);
	NC->SetRelativeLocation(Location);
	NC->SetRelativeRotation(Rotation.Rotation());
}

void AAmalgamVisualisationManager::RemoveFromMap(FMassEntityHandle EntityHandle)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	checkf(ContainsElement(HandleAsNumber), TEXT("Handle is not in map."));
	UNiagaraComponent* NC = GetNiagaraComponent(HandleAsNumber);
	NC->DestroyComponent();

	int Idx = FindElementIndex(HandleAsNumber);
	ElementArray.RemoveAt(Idx);
}

UNiagaraComponent* AAmalgamVisualisationManager::GetNiagaraComponent(uint64 ElementHandle)
{
	UNiagaraComponent* NC = FindElement(ElementHandle)->NiagaraComponent;
	return NC;
}

FNiagaraVisualElement* AAmalgamVisualisationManager::FindElement(uint64 ElementHandle)
{
	FNiagaraVisualElement* FoundElement = ElementArray.FindByPredicate([&](const FNiagaraVisualElement& Element)
		{
			return Element.Handle == ElementHandle;
		});

	return FoundElement;
}

int32 AAmalgamVisualisationManager::FindElementIndex(uint64 ElementHandle)
{
	int32 FoundElement = ElementArray.IndexOfByPredicate([&](const FNiagaraVisualElement& Element)
		{
			return Element.Handle == ElementHandle;
		});

	return FoundElement;
}

bool AAmalgamVisualisationManager::ContainsElement(uint64 ElementHandle)
{
	FNiagaraVisualElement* FoundElement = ElementArray.FindByPredicate([&](const FNiagaraVisualElement& Element)
		{
			return Element.Handle == ElementHandle;
		});

	return FoundElement != nullptr;
}

