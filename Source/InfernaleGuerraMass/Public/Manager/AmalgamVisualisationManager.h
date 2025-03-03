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

#include "AmalgamVisualisationManager.generated.h"

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
struct FTruc
{
	GENERATED_USTRUCT_BODY()
public:
	FMassEntityHandle EntityHandle;
	UNiagaraComponent* NiagaraComponent;
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
	void CreateAndAddToMapP(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location);
	void UpdatePositionP(FMassEntityHandle EntityHandle, FVector const Location, FVector const Rotation);
	void RemoveFromMapP(FMassEntityHandle EntityHandle);
	

protected: /* Protected AVM Methods */
		   /* These should only be called through replicated methods */
		   /* so that managers are never out of sync */

	void AddToMap(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent);
	void CreateAndAddToMap(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location);

	void UpdateItemPosition(FMassEntityHandle EntityHandle, FVector const Location, FVector const Rotation);

	void RemoveFromMap(FMassEntityHandle EntityHandle);

	UFUNCTION(NetMulticast, Reliable) void AddToMapMulticast(FMassEntityHandle EntityHandle, UNiagaraComponent* NiagaraComponent);
	UFUNCTION(NetMulticast, Reliable) void AddToMapMulticast2();
	UFUNCTION(NetMulticast, Reliable) void CreateAndAddToMapMulticast(FMassEntityHandle EntityHandle, FOwner EntityOwner, const UWorld* World, UNiagaraSystem* NiagaraSystem, const FVector Location);
	UFUNCTION(NetMulticast, Reliable) void UpdatePositionMulticast(FMassEntityHandle EntityHandle, FVector const Location, FVector const Rotation);
	UFUNCTION(NetMulticast, Reliable) void RemoveFromMapMulticast(FMassEntityHandle EntityHandle);

private: /* Private Methods */

	UNiagaraComponent* GetNiagaraComponent(uint64 ElementHandle);
	FNiagaraVisualElement* FindElement(uint64 ElementHandle);
	int32 FindElementIndex(uint64 ElementHandle);
	bool ContainsElement(uint64 ElementHandle);
	TArray<FTruc> Trucs = TArray<FTruc>();

public: /* Public Member variables */

private: /* Private Member variables */

	TArray<FNiagaraVisualElement> ElementArray;
};
