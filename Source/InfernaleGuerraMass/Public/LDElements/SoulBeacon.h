// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/Damageable.h"
#include "Interfaces/FluxTarget.h"
#include "Interfaces/Ownable.h"
#include "Interfaces/UnitTargetable.h"
#include "Interfaces/VisuallyUpdatedByOwner.h"
#include "LD/LDElement/LDElement.h"
#include "GameMode/Infernale/GameModeInfernale.h"

#include "SoulBeacon.generated.h"

/**
 * 
 */

struct FOwner;
struct HashGridCell;
class UEffectAfterDelayComponent;
class UDamageableComponent;
class USoulBeaconRewardAsset;
class ABuildingParent;

UENUM(BlueprintType)
enum ESoulBeaconRewardType
{
	RewardAmalgam,
	RewardNeutral,
	RewardBoss,
	RewardBuilding,
	RewardMainBuilding
};

UENUM()
enum ESoulBeaconCaptureState
{
	Capturing,
	Stalemate,
	Uncontested
};

USTRUCT(BlueprintType)
struct FDetectionRange
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* DetectionPivot;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Radius;

	const FVector GetPosition() const
	{
		if (!DetectionPivot) return FVector::ZeroVector;

		FVector Position = DetectionPivot->GetActorLocation(); 
		Position.Z = 0.f;
		return Position; 
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnContesterAddOrRemove);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCaptured, FOwner, Capturer);

UCLASS()
class INFERNALETESTING_API ASoulBeacon : public ALDElement, public IOwnable, public IVisuallyUpdatedByOwner, public IDamageable, public IFluxTarget, public IUnitTargetable
{
	GENERATED_BODY()
	
public:
	ASoulBeacon();

	virtual void Tick(float DeltaTime) override;

	TWeakObjectPtr<UDamageableComponent> GetDamageableComponent() const;

	virtual FOwner GetOwner() override;
	virtual void SetOwner(FOwner NewOwner) override;
	virtual void ChangeOwner(FOwner NewOwner) override;
	
	void TryGetGameModeInfernale();
	UFUNCTION() void OnPreLaunchGame();

	virtual float DamageHealthOwner(const float DamageAmount, const bool bDamagePercent, const FOwner DamageOwner) override;
	virtual float GetTargetableRange() override;
	virtual EEntityType GetEntityType() const override;
	
	TArray<FDetectionRange> GetCaptureRanges();
	TArray<FDetectionRange> GetEffectRanges();
	
	void Reward(ESoulBeaconRewardType RewardType);
	void RewardMultiple(ESoulBeaconRewardType RewardType, int RewardCount);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RewardBP(ESoulBeaconRewardType RewardType);

	void AddContester(FOwner Capturer);
	void RemoveContester(FOwner Capturer);

	void AddCells(TArray<HashGridCell*> AddedCells);

	void AddCell(HashGridCell* Cell);
	void CheckCells();
	bool IsCellInRange(HashGridCell* Cell);

	bool IsValidRewarder();

	UFUNCTION(BlueprintCallable)
	void ActivateInstant();

	UFUNCTION(BlueprintCallable)
	bool CanActivate();

protected:
	virtual void BeginPlay() override;
	virtual void RegisterLDElement() override;

	void InteractStartMain(APlayerControllerInfernale* Interactor);
	void InteractEndMain(APlayerControllerInfernale* Interactor);

	void SyncDataAsset();

	UFUNCTION() void OnDamaged(TWeakObjectPtr<AActor> _, float NewHealth, float DamageAmount);

	UFUNCTION() void UpdateOwnerVisuals(TWeakObjectPtr<ASoulBeacon> Actor, FOwner OldOwner, FOwner NewOwner);

	UFUNCTION(NetMulticast, Reliable)
	void OnOwnerChangedMulticast(FOwner NewOwner);

	void Capture(float DeltaTime);
	
	UFUNCTION() void UpdateState();

	UFUNCTION() void Captured(FOwner Capturer);

	UFUNCTION(CallInEditor)
	void DebugEffectRange();

	UFUNCTION(CallInEditor)
	void DebugCaptureRange();

public:
	UPROPERTY(BlueprintAssignable) FOnContesterAddOrRemove OnContesterAddOrRemove;
	UPROPERTY(BlueprintAssignable) FOnCaptured OnCaptured;
	
protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main Building")
	AGameModeInfernale* GameModeInfernale;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UEffectAfterDelayComponent* EffectAfterDelayComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UDamageableComponent* DamageableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoulBeaconRewardAsset* RewardDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FOwner OwnerWithTeam;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data asset")
	bool bUseDataAsset = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MaxHealth = 100.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float EffectRange = 200.f; // For now used for capture and effect

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float CapturePerSecond = 15.f; // based on the fact that total capture needed is 100
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float CaptureDecreasePerSecond = 15.f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float ContesterChangeThreshold = 0.0001f;
	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<FDetectionRange> CaptureRanges;
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<FDetectionRange> EffectRanges;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Beacon")
	bool bIsActive = false;

	FTimerHandle CooldownHandle;

	ESoulBeaconCaptureState CaptureState = ESoulBeaconCaptureState::Uncontested;

	TArray<HashGridCell*> CaptureCells = TArray<HashGridCell*>();
	float CaptureAmount = 0.f;
	TArray<FOwner> Contesters = TArray<FOwner>();
	
	FOwner MainContester = FOwner();

	bool GameModeInfernaleWasInitialized = false;

private:
	bool bDebug = false;
};