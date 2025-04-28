// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// Mass includes
#include <MassEntityTypes.h>

// Niagara includes
#include <NiagaraComponent.h>

// Network includes
#include <Net/UnrealNetwork.h>

// IG includes
#include <Interfaces/Ownable.h>

#include "Structs/SimpleStructs.h"
#include "AmalgamVisualisationManager.generated.h"

struct FDataForVisualisation;
enum class EEntityType : uint8;

USTRUCT()
struct FNiagaraVisualElement
{
	GENERATED_USTRUCT_BODY()

	FNiagaraVisualElement() = default;
	FNiagaraVisualElement(uint64 InHandle, UNiagaraComponent* InNiagaraComponent) : Handle(InHandle), NiagaraComponent(InNiagaraComponent)
	{ }

	inline bool operator==(const FNiagaraVisualElement Other) { return Handle == Other.Handle; }

	uint64 Handle;
	UNiagaraComponent* NiagaraComponent;
};

USTRUCT()
struct FBPVisualElement
{
	GENERATED_USTRUCT_BODY()

	FBPVisualElement() = default;
	FBPVisualElement(uint64 InHandle, AActor* InElement) : Handle(InHandle), Element(InElement)
	{
	}

	inline bool operator==(const FNiagaraVisualElement Other) { return Handle == Other.Handle; }

	uint64 Handle;
	TWeakObjectPtr<AActor> Element;
};


USTRUCT()
struct FTruc
{
	GENERATED_USTRUCT_BODY()
public:
	FMassEntityHandle EntityHandle;
	UNiagaraComponent* NiagaraComponent;
};

USTRUCT()
struct FDataForSpawnVisual
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY() FOwner EntityOwner;
	UPROPERTY() UWorld* World;
	UPROPERTY() TSubclassOf<AActor> BPVisualisation;
	UPROPERTY() FVector Location;
	UPROPERTY() float SpeedMultiplier = -1;
	UPROPERTY() EEntityType EntityType;
};

UCLASS()
class INFERNALETESTING_API AAmalgamVisualisationManager : public AActor
{
	GENERATED_BODY()
	
	/* Default AActor Methods */
public:	
	// Sets default values for this actor's properties
	AAmalgamVisualisationManager();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	/* End of Default AActor Methods */


public: /* Public AVM Methods */
	void AddToMapP(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent);
	void AddToMapP(FMassEntityHandle EntityHandle, AActor* BPVisualisation);
	void CreateAndAddToMapP(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location);
	void CreateAndAddToMapP(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual);
	void BatchUpdatePosition(const TArray<FMassEntityHandle>& EntityHandles, const TArray<FDataForVisualisation>& DataForVisualisations);
	void UpdatePositionP(FMassEntityHandle EntityHandle, const FDataForVisualisation DataForVisualisation);
	void RemoveFromMapP(FMassEntityHandle EntityHandle);
	void ChangeBatch(int Value);

	float GetRadius();
	void UpdatePositionOfSpecificUnits(const TArray<FDataForVisualisation>& DataForVisualisations, const TArray<FMassEntityHandle>& EntitiesToHide);

	void ShowHideAllItems(bool bShow);
	

protected: /* Protected AVM Methods */
		   /* These should only be called through replicated methods */
		   /* so that managers are never out of sync */

	void AddToMap(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent);
	void AddToMap(FMassEntityHandle EntityHandle, AActor* BPActor);
	void CreateAndAddToMap(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location);
	void CreateAndAddToMap(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual, TSubclassOf<AActor> BPVisualisation);

	void UpdateItemPosition(FMassEntityHandle EntityHandle, FDataForVisualisation DataForVisualisation);
	void HideItem(FMassEntityHandle EntityHandle);

	void RemoveFromMap(FMassEntityHandle EntityHandle);

	UFUNCTION() void OnPreLaunchGame();

	UFUNCTION(NetMulticast, Reliable) void AddToMapMulticast(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent);
	UFUNCTION(NetMulticast, Reliable) void AddToMapMulticastBP(FMassEntityHandle EntityHandle, AActor* BPVisualisation);
	UFUNCTION(NetMulticast, Reliable) void AddToMapMulticast2();
	UFUNCTION(NetMulticast, Reliable) void CreateAndAddToMapMulticast(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location);
	UFUNCTION(NetMulticast, Reliable) void CreateAndAddToMapMulticastBP(FMassEntityHandle EntityHandle, FDataForSpawnVisual DataForSpawnVisual, TSubclassOf<AActor> BPVisualisation);
	UFUNCTION(NetMulticast, Unreliable) void UpdatePositionMulticast(FMassEntityHandle EntityHandle, const FDataForVisualisation DataForVisualisation);
	UFUNCTION(NetMulticast, Reliable) void BatchUpdatePositionMulticast(const TArray<FMassEntityHandle>& EntityHandles, const TArray<FDataForVisualisation>& DataForVisualisations);
	UFUNCTION(NetMulticast, Reliable) void RemoveFromMapMulticast(FMassEntityHandle EntityHandle);
	UFUNCTION(NetMulticast, Reliable) void ChangeBatchMulticast(int Value);

private: /* Private Methods */

	TWeakObjectPtr<UNiagaraComponent> GetNiagaraComponent(uint64 ElementHandle);
	FNiagaraVisualElement* FindElement(uint64 ElementHandle);
	FBPVisualElement* FindElementBP(uint64 ElementHandle);
	int32 FindElementIndex(uint64 ElementHandle);
	bool ContainsElement(uint64 ElementHandle);
	TArray<FTruc> Trucs = TArray<FTruc>();

public: /* Public Member variables */

private: /* Private Member variables */

	TArray<FNiagaraVisualElement> ElementArray;

	TArray<FBPVisualElement> BPElementsArray;

	UPROPERTY(EditAnywhere) bool bUseBPVisualisation = false;

	bool bDebugReplicationNumbers = false;
	bool bHideAll = true;
	int CurrentBatch = 0;
	int UnitsByBatch = 50;

	AInfernalePawn* InfernalePawn;
	FVector LocalPointLocation;

	
	int Radius = 10000;
	
	/*UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> BPVisualisation;*/
};
