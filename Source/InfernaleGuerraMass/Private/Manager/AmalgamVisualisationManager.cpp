// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/AmalgamVisualisationManager.h"

// Niagara includes
#include <NiagaraFunctionLibrary.h>

// Project function library, for getting emissive color from owner
#include <FunctionLibraries/FunctionLibraryInfernale.h>
#include <UnitAsActor/NiagaraUnitAsActor.h>

#include "Kismet/GameplayStatics.h"
#include "Mass/Army/AmalgamFragments.h"
#include "Structs/ReplicationStructs.h"

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

	if (!HasAuthority()) return;
	const auto GameMode = Cast<AGameModeInfernale>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!GameMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("GameMode is not valid"));
		return;
	}
	GameMode->PreLaunchGame.AddDynamic(this, &AAmalgamVisualisationManager::OnPreLaunchGame);
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

void AAmalgamVisualisationManager::CreateAndAddToMapP(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("CreateAndAddToMapP: No autority"));
		return;
	}
	CreateAndAddToMapMulticastBP(EntityHandle, DataForSpawnVisual, DataForSpawnVisual.BPVisualisation);
}

void AAmalgamVisualisationManager::BatchUpdatePosition(const TArray<FMassEntityHandle>& EntityHandles, const TArray<FDataForVisualisation>& DataForVisualisations)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("BatchUpdatePosition: No autority"));
		return;
	}

	// if (Entities.Num() != Locations.Num() && Entities.Num() != Rotations.Num())
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("BatchUpdatePosition: Update Count Mismatch"));
	// 	return;
	// }

	/*for (int Index = 0; Index < Entities.Num(); ++Index)
	{
		FVector CurrentLoc = Locations[Index];
		FVector CurrentRot = Rotations[Index];
		UpdatePositionMulticast(Entities[Index], CurrentLoc.X, CurrentLoc.Y, CurrentRot.Z);
	}*/

	BatchUpdatePositionMulticast(EntityHandles, DataForVisualisations);
}

void AAmalgamVisualisationManager::UpdatePositionP(FMassEntityHandle EntityHandle, const FDataForVisualisation DataForVisualisation)
{
	if (!HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("UpdatePositionP: No autority"));
		return;
	}
	
	UpdatePositionMulticast(EntityHandle, DataForVisualisation);
	//UpdatePositionMulticast(EntityHandle, Location.X, Location.Y, Rotation.Z);
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

void AAmalgamVisualisationManager::ChangeBatch(int Value)
{
	ChangeBatchMulticast(Value);
}

float AAmalgamVisualisationManager::GetRadius()
{
	return Radius;
}

void AAmalgamVisualisationManager::UpdatePositionOfSpecificUnits(
	const TArray<FDataForVisualisation>& DataForVisualisations,
	const TArray<FMassEntityHandle>& EntitiesToHide)
{
	if (bDebugReplicationNumbers) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, FString::Printf(TEXT("UpdatePositionOfSpecificUnits: %d"), DataForVisualisations.Num()));

	if (!InfernalePawn)
	{
		const auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		const auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(PlayerController);
		if (!PlayerControllerInfernale) return;
		InfernalePawn = Cast<AInfernalePawn>(PlayerControllerInfernale->GetPawn());
	}
	if (!InfernalePawn) return;
	
	LocalPointLocation = InfernalePawn->GetLookAtLocationPoint();


	for (int Index = 0; Index < DataForVisualisations.Num(); ++Index)
	{
		const auto& DataForVisualisation = DataForVisualisations[Index];
		UpdateItemPosition(DataForVisualisation.EntityHandle, DataForVisualisation);
	}

	for (const auto EntityToHide : EntitiesToHide)
	{
		HideItem(EntityToHide);
	}
}

void AAmalgamVisualisationManager::AddToMapMulticast_Implementation(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent)
{
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("Added to map"));

	AddToMap(EntityHandle, NiagaraComponent);
}

void AAmalgamVisualisationManager::AddToMapMulticastBP_Implementation(FMassEntityHandle EntityHandle, AActor* BPVisualisation)
{
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("Added to map"));

	AddToMap(EntityHandle, BPVisualisation);
}

void AAmalgamVisualisationManager::AddToMapMulticast2_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("Added to map2"));
}

void AAmalgamVisualisationManager::CreateAndAddToMapMulticast_Implementation(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location)
{
	CreateAndAddToMap(EntityHandle, EntityOwner, World, NiagaraSystem, Location);
}

void AAmalgamVisualisationManager::CreateAndAddToMapMulticastBP_Implementation(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual, TSubclassOf<AActor> BPVisualisation)
{
	CreateAndAddToMap(EntityHandle, DataForSpawnVisual, BPVisualisation);
}

void AAmalgamVisualisationManager::UpdatePositionMulticast_Implementation(FMassEntityHandle EntityHandle, const FDataForVisualisation DataForVisualisation)
{
	//FVector Location, Rotation;

	//Location = FVector(xLoc, yLoc, 0.f); //@todo replace 0 w/ height offset
	//Rotation = FVector(0.f, 0.f, Rot);

	UpdateItemPosition(EntityHandle, DataForVisualisation);
}

void AAmalgamVisualisationManager::BatchUpdatePositionMulticast_Implementation(const TArray<FMassEntityHandle>& EntityHandles, const TArray<FDataForVisualisation>& DataForVisualisations)
{
	if (EntityHandles.Num() != DataForVisualisations.Num())
	{
		//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("BatchUpdatePositionMulticast: EntityHandles and DataForVisualisations size mismatch."));
		return;
	}

	// const auto Mins = FMath::Min(EntityHandles.Num(), CurrentBatch + UnitsByBatch);
	//
	// for (int Index = CurrentBatch; Index < Mins; ++Index)
	// {
	// 	UpdateItemPosition(EntityHandles[Index], DataForVisualisations[Index]);
	// }
	// CurrentBatch += UnitsByBatch;
	// if (CurrentBatch >= EntityHandles.Num()) CurrentBatch = 0;

	if (!InfernalePawn)
	{
		const auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		const auto PlayerControllerInfernale = Cast<APlayerControllerInfernale>(PlayerController);
		if (!PlayerControllerInfernale) return;
		InfernalePawn = Cast<AInfernalePawn>(PlayerControllerInfernale->GetPawn());
	}
	if (!InfernalePawn) return;
	
	LocalPointLocation = InfernalePawn->GetLookAtLocationPoint();
	LocalPointLocation.Z = 0.f;
	
	for (int Index = 0; Index < EntityHandles.Num(); ++Index)
	{
		UpdateItemPosition(EntityHandles[Index], DataForVisualisations[Index]);
	}
    
}

void AAmalgamVisualisationManager::RemoveFromMapMulticast_Implementation(FMassEntityHandle EntityHandle)
{
	RemoveFromMap(EntityHandle);
}

void AAmalgamVisualisationManager::AddToMap(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(!ContainsElement(HandleAsNumber), TEXT("Map already contains handle."));

	if (ContainsElement(HandleAsNumber)) return;

	FNiagaraVisualElement NewElement(HandleAsNumber, NiagaraComponent);
	
	ElementArray.Add(NewElement);
}

void AAmalgamVisualisationManager::AddToMap(FMassEntityHandle EntityHandle, AActor* BPActor)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(!ContainsElement(HandleAsNumber), TEXT("Map already contains handle."));

	if (ContainsElement(HandleAsNumber)) return;

	FBPVisualElement NewElement(HandleAsNumber, BPActor);

	BPElementsArray.Add(NewElement);
}

void AAmalgamVisualisationManager::CreateAndAddToMap(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(!ContainsElement(HandleAsNumber), TEXT("Map already contains handle."));

	if (ContainsElement(HandleAsNumber)) return;

	const auto TeamColor = UFunctionLibraryInfernale::GetTeamColorCpp(EntityOwner.Team, EEntityType::EntityTypeBehemot);
	const auto EmissiveColor = UFunctionLibraryInfernale::GetTeamColorEmissiveCpp(EntityOwner.Team, EEntityType::EntityTypeBehemot);
	

	UNiagaraComponent* SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, NiagaraSystem, Location);
	SpawnedComponent->SetColorParameter("TeamColor", TeamColor);
	SpawnedComponent->SetColorParameter("EmissiveColor", EmissiveColor);
	FNiagaraVisualElement NewElement(HandleAsNumber, SpawnedComponent);

	ElementArray.Add(NewElement);
}

void AAmalgamVisualisationManager::CreateAndAddToMap(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual, TSubclassOf<AActor> BPVisualisation)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(!ContainsElement(HandleAsNumber), TEXT("Map already contains handle."));

	if (HasAuthority())
	{
		if (ContainsElement(HandleAsNumber))
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("CreateAndAddToMap: Map already contains handle."));
			return;
		}
	}

	const auto TeamColor = UFunctionLibraryInfernale::GetTeamColorCpp(DataForSpawnVisual.EntityOwner.Team, DataForSpawnVisual.EntityType);
	const auto EmissiveColor = UFunctionLibraryInfernale::GetTeamColorEmissiveCpp(DataForSpawnVisual.EntityOwner.Team, DataForSpawnVisual.EntityType);
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, TeamColor.ToFColorSRGB() , FString::Printf(TEXT("GetTeamColorCpp: Team: %s, EntityType: %s"), *UEnum::GetValueAsString(DataForSpawnVisual.EntityOwner.Team), *UEnum::GetValueAsString(DataForSpawnVisual.EntityType)));

	AActor* Spawned;
	Spawned = GetWorld()->SpawnActor<AActor>(BPVisualisation, DataForSpawnVisual.Location, FRotator(0.f, 0.f, 0.f));
	auto NiagaraUnitAsActor = Cast<ANiagaraUnitAsActor>(Spawned);
	if (!NiagaraUnitAsActor)
	{
		if (Spawned) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, FString::Printf(TEXT("CreateAndAddToMap: Failed to cast to ANiagaraUnitAsActor, actor: %s"), *Spawned->GetName()));
		else GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Red, TEXT("CreateAndAddToMap: Failed to cast to ANiagaraUnitAsActor, actor is null"));
		return;
	}
	NiagaraUnitAsActor->GetNiagaraComponent()->SetColorParameter("TeamColor", TeamColor);
	NiagaraUnitAsActor->GetNiagaraComponent()->SetColorParameter("EmissiveColor", EmissiveColor);
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("-------------------------------------------------------------")));
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, TeamColor.ToFColorSRGB(), FString::Printf(TEXT("GetTeamColorCpp: Team: %s, EntityType: %s"), *UEnum::GetValueAsString(DataForSpawnVisual.EntityOwner.Team), *UEnum::GetValueAsString(DataForSpawnVisual.EntityType)));
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, EmissiveColor.ToFColorSRGB(), FString::Printf(TEXT("GetTeamColorCpp: Team: %s, EntityType: %s"), *UEnum::GetValueAsString(DataForSpawnVisual.EntityOwner.Team), *UEnum::GetValueAsString(DataForSpawnVisual.EntityType)));
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("-------------------------------------------------------------")));
	NiagaraUnitAsActor->SpeedMultiplierUpdated(DataForSpawnVisual.SpeedMultiplier);
	NiagaraUnitAsActor->OwnerOnCreation(DataForSpawnVisual.EntityOwner);
	FBPVisualElement NewElement(HandleAsNumber, Spawned);
	BPElementsArray.Add(NewElement);
	return;
}

void AAmalgamVisualisationManager::UpdateItemPosition(FMassEntityHandle EntityHandle, FDataForVisualisation DataForVisualisation)
{
	if (!bHideAll) return;
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(ContainsElement(HandleAsNumber), TEXT("Handle is not in map."));
	
	if (!ContainsElement(HandleAsNumber)) return;
	auto Location = FVector(DataForVisualisation.LocationX, DataForVisualisation.LocationY, 0.f);
	auto Rotation = FVector(DataForVisualisation.RotationX, DataForVisualisation.RotationY,0.f);
	
	if (bUseBPVisualisation)
	{
		AActor* Visualisation = FindElementBP(HandleAsNumber)->Element.Get();
		ANiagaraUnitAsActor* NiagaraActor = Cast<ANiagaraUnitAsActor>(Visualisation);
		
		NiagaraActor->Activate(true);
		
		Visualisation->SetActorLocation(Location);
		//DrawDebugSphere(GetWorld(), Location, 200, 12, FColor::Red, false, 0.f);
		Visualisation->SetActorRotation(Rotation.Rotation());
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> NC = GetNiagaraComponent(HandleAsNumber);
	NC->SetRelativeLocation(Location);
	NC->SetRelativeRotation(Rotation.Rotation());
}

void AAmalgamVisualisationManager::HideItem(FMassEntityHandle EntityHandle)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(ContainsElement(HandleAsNumber), TEXT("Handle is not in map."));
	
	if (!ContainsElement(HandleAsNumber)) return;
	
	if (bUseBPVisualisation)
	{
		AActor* Visualisation = FindElementBP(HandleAsNumber)->Element.Get();
		ANiagaraUnitAsActor* NiagaraActor = Cast<ANiagaraUnitAsActor>(Visualisation);
		NiagaraActor->Activate(false);
	}
}

void AAmalgamVisualisationManager::ShowHideAllItems(bool bShow)
{
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, FString::Printf(TEXT("ShowHideAllItems:%d"), bShow));
	bHideAll = !bShow;
	for (int Index = 0; Index < BPElementsArray.Num(); ++Index)
	{
		if (bUseBPVisualisation)
		{
			AActor* Visualisation = FindElementBP(BPElementsArray[Index].Handle)->Element.Get();
			ANiagaraUnitAsActor* NiagaraActor = Cast<ANiagaraUnitAsActor>(Visualisation);
			NiagaraActor->Activate(!bShow);
		}
	}
}

void AAmalgamVisualisationManager::RemoveFromMap(FMassEntityHandle EntityHandle)
{
	uint64 HandleAsNumber = EntityHandle.AsNumber();
	//checkf(ContainsElement(HandleAsNumber), TEXT("Handle is not in map."));

	if (!ContainsElement(HandleAsNumber)) return;

	if (bUseBPVisualisation)
	{
		FindElementBP(HandleAsNumber)->Element->Destroy();
		int Idx = FindElementIndex(HandleAsNumber);
		BPElementsArray.RemoveAt(Idx);
		return;
	}

	TWeakObjectPtr<UNiagaraComponent> NC = GetNiagaraComponent(HandleAsNumber);
	NC->DestroyComponent();

	int Idx = FindElementIndex(HandleAsNumber);
	ElementArray.RemoveAt(Idx);
}

void AAmalgamVisualisationManager::OnPreLaunchGame()
{
}

void AAmalgamVisualisationManager::ChangeBatchMulticast_Implementation(int Value)
{
	// UnitsByBatch += Value;
	// if (UnitsByBatch < 10) UnitsByBatch = 10;
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("NewVal: %d"), UnitsByBatch));


	Radius += Value;
	if (Radius < 1000) Radius = 1000;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Radius: %d"), Radius));
}

TWeakObjectPtr<UNiagaraComponent> AAmalgamVisualisationManager::GetNiagaraComponent(uint64 ElementHandle)
{
	TWeakObjectPtr<UNiagaraComponent> NC = FindElement(ElementHandle)->NiagaraComponent;
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

FBPVisualElement* AAmalgamVisualisationManager::FindElementBP(uint64 ElementHandle)
{
	FBPVisualElement* FoundElement = BPElementsArray.FindByPredicate([&](const FBPVisualElement& Element)
		{
			return Element.Handle == ElementHandle;
		});

	return FoundElement;
}

int32 AAmalgamVisualisationManager::FindElementIndex(uint64 ElementHandle)
{
	if (bUseBPVisualisation)
	{
		int32 FoundElement = BPElementsArray.IndexOfByPredicate([&](const FBPVisualElement& Element)
			{
				return Element.Handle == ElementHandle;
			});
		return FoundElement;
	}

	int32 FoundElement = ElementArray.IndexOfByPredicate([&](const FNiagaraVisualElement& Element)
		{
			return Element.Handle == ElementHandle;
		});

	return FoundElement;
}

bool AAmalgamVisualisationManager::ContainsElement(uint64 ElementHandle)
{
	if (bUseBPVisualisation)
	{
		FBPVisualElement* FoundElement = BPElementsArray.FindByPredicate([&](const FBPVisualElement& Element)
			{
				return Element.Handle == ElementHandle;
			});
		return FoundElement != nullptr;
	}

	FNiagaraVisualElement* FoundElement = ElementArray.FindByPredicate([&](const FNiagaraVisualElement& Element)
		{
			return Element.Handle == ElementHandle;
		});

	return FoundElement != nullptr;
}

