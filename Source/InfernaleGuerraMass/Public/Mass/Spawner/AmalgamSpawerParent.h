// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassSpawner.h"
#include <Mass/Amalgam/Data/AmalgamDataStruct.h>
#include "Interfaces/Ownable.h"

#include "AmalgamSpawerParent.generated.h"

class UNiagaraSystem;
class USpawnerComponent;

struct FOwner;

USTRUCT(BlueprintType)
struct FAmalgamSpawnerData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite)
	float SpawnDelay;

	// TODO : 
	// Add data for spawned amalgam
};

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API AAmalgamSpawnerParent : public AMassSpawner
{
	GENERATED_BODY()
public:
	AAmalgamSpawnerParent();

	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void DoAmalgamSpawning(int Index);

	static TArray<AAmalgamSpawnerParent*> Spawners;
	
	void Initialize(USpawnerComponent* NewSpawnerComponent, int32 EntitySpawnCount);
	TWeakObjectPtr<AFlux> GetFlux();

	inline FOwner GetOwner() { return Owner; }
	inline FAmalgamSpawnData GetAmalgamData() { return AmalgamData; }
	inline void ReplaceWithFluxes(TArray<TWeakObjectPtr<AFlux>> NewFluxes) { Fluxes = NewFluxes; }
	inline bool CanDequeueSpawnIndex() { return !SpawnQueue.IsEmpty(); }
	inline UNiagaraSystem* GetNiagaraSystem() { return NiagaraSystem; }
	inline UNiagaraComponent* GetNiagaraComponent() { return NiagaraComponent; }
	inline void SetNiagaraComponent(UNiagaraComponent* InNiagaraComponent) { NiagaraComponent = InNiagaraComponent; }
	
	USpawnerComponent* GetSpawnerComponent() { return SpawnerComponent; }
	void SetSpawnerComponent(USpawnerComponent* NewSpawnerComponent);
	
	void UpdateOwner();
	void PostRegister();
	bool IsEnabled() { return bIsEnabled; }

	float GetStrengthMultiplier();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostLoad() override;
	virtual void PostRegisterAllComponents() override;
	virtual void BeginDestroy() override;

	inline int DequeueSpawnIndex() { int Index; SpawnQueue.Dequeue(Index); return Index; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNiagaraSystem* NiagaraSystem;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAmalgamSpawnData AmalgamData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LocalSpawnDelay;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LocalSpawnTimer;
	
	FOwner Owner;

	bool bIsEnabled = true; // Check if portal should spawn units or not

	TArray<TWeakObjectPtr<AFlux>> Fluxes = TArray<TWeakObjectPtr<AFlux>>();
	TQueue<int> SpawnQueue = TQueue<int>();

	USpawnerComponent* SpawnerComponent;

	UNiagaraComponent* NiagaraComponent;

private:

	bool bDebug = false;
};
