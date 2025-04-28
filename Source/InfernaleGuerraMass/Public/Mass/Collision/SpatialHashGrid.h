// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MassCommonFragments.h"
#include "Mass/Army/AmalgamFragments.h"
#include "LD/Buildings/BuildingParent.h"
#include <LD/LDElement/LDElement.h>
#include <LD/LDElement/SoulBeacon.h>

#include "SpatialHashGrid.generated.h"


class ADataGathererActor;

struct GridCellEntityData
{
	FOwner Owner;
	FVector Location;
	
	EEntityType EntityType;

	float MaxEntityHealth;
	float EntityHealth;
	float TargetableRadius;

	int AggroCount = 0;

	GridCellEntityData(FOwner EntityOwner, FVector EntityLocation, float Health, float Radius, EEntityType Type) : Owner(EntityOwner), Location(EntityLocation), EntityType(Type), MaxEntityHealth(Health), EntityHealth(Health), TargetableRadius(Radius)
	{}

	bool operator== (const GridCellEntityData& Other)
	{
		return Location == Other.Location && Owner.Team == Other.Owner.Team;
	}

	bool operator== (GridCellEntityData* Other)
	{
		return Location == Other->Location && Owner.Team == Other->Owner.Team;
	}

	void operator-- ()
	{
		EntityHealth -= 10.f;
	}

	void SetData(FOwner NewOwner, FVector NewLocation)
	{
		Owner = NewOwner;
		Location = NewLocation;
	}

	void DamageEntity(float Damage);

	static GridCellEntityData None()
	{
		return GridCellEntityData(FOwner(), FVector::ZeroVector, 0.f, 0.f, EEntityType::EntityTypeNone);
	}
};

struct HashGridCell
{
	TMap<FMassEntityHandle, GridCellEntityData> Entities = TMap <FMassEntityHandle, GridCellEntityData>();
	TArray<TWeakObjectPtr<ABuildingParent>> Buildings = TArray<TWeakObjectPtr<ABuildingParent>>();
	TArray<TWeakObjectPtr<ALDElement>> LDElements = TArray<TWeakObjectPtr<ALDElement>>();
	
	TArray<FOwner> PresentOwners;

	TWeakObjectPtr<ASoulBeacon> LinkedBeacon;

	/* Getters */

	GridCellEntityData* GetEntity(FMassEntityHandle Entity);

	const TArray<GridCellEntityData> GetEntities();
	TArray<GridCellEntityData> GetMutableEntities();
	TArray<GridCellEntityData> GetEntitiesByTeamDifference(FOwner Owner);

	TArray<TWeakObjectPtr<ABuildingParent>> GetBuildings();

	TArray<TWeakObjectPtr<ALDElement>> GetLDElements();
	
	TArray<FOwner> GetPresentOwners();

	void UpdatePresentOwners();

	/* Contains methods */

	bool Contains(FMassEntityHandle Entity);
	bool Contains(TWeakObjectPtr<ABuildingParent> Building);
	bool Contains(TWeakObjectPtr<ALDElement> LDElement);


	/* Num methods */
	int GetTotalNum() { return Entities.Num() + Buildings.Num() + LDElements.Num(); }
	int GetEntitiesNum() { return Entities.Num(); }
	int GetBuildingsNum() { return Buildings.Num(); }
	int GetLDElementsNum() { return LDElements.Num(); }

	int GetTotalNumByTeam(FOwner Owner);
	int GetTotalNumByTeamDifference(FOwner Owner);
};

struct FDetectionResult
{
	FMassEntityHandle Entity = FMassEntityHandle(0,0);
	TWeakObjectPtr<ABuildingParent> Building = nullptr;
	TWeakObjectPtr<ALDElement> LD = nullptr;

	float EntityDistance = TNumericLimits<float>::Max();
	float BuildingDistance = TNumericLimits<float>::Max();
	float LDDistance = TNumericLimits<float>::Max();
};

UCLASS()
class INFERNALETESTING_API ASpatialHashGrid : public AActor
{	
	GENERATED_BODY()
public:	
	// Sets default values for this actor's properties
	ASpatialHashGrid();
	ASpatialHashGrid(bool IsPivotCentered);
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	static bool Generate();

	static inline FIntVector2 GetGridSize() { return Instance->GridSize; }

	static inline int32 GetNumEntitiesInCell(FIntVector2 Coordinates) { return Instance->GridCells[Coordinates.X + (Instance->GridSize.X * Coordinates.Y)].GetEntitiesNum(); }


	/* ------ Access Methods ------ */

	/* ----- Entities */

	// Adds Entity to grid and references data properly for quick access
	static bool AddEntityToGrid(FVector WorldCoordinates, FMassEntityHandle Entity, FOwner Owner, FTransformFragment* Transform, float Health, float TargetableRadius, EEntityType Type);

	// Removes Entity from grid and ensures all references are cleaned up
	static bool RemoveEntityFromGrid(FMassEntityHandle Entity);

	// Moves the Entity's data from its current cell to another
	static bool MoveEntityToCell(FMassEntityHandle Entity, FVector WorldCoordinates);

	// Sets the Entity's transform
	static void UpdateCellTransform(FMassEntityHandle Entity, FTransform Transform);

	static void DamageEntity(FMassEntityHandle Entity, float Damage);

	/* ----- Buildings */
	
	// Adds Building to grid
	static bool AddBuildingToGrid(FVector WorldCoordinates, ABuildingParent* Building);
	
	// Removes Building from grid
	static bool RemoveBuildingFromGrid(FVector WorldCoordinates, ABuildingParent* Building);

	/* LD Elements */

	// Adds LDElement to grid
	static bool AddLDElementToGrid(FVector WorldCoordinates, ALDElement* LDElement);

	// Removes LDElement from grid
	static bool RemoveLDElementFromGrid(FVector WorldCoordinates, ALDElement* LDElement);
	
	static bool AddSoulBeaconToGrid(ASoulBeacon* SoulBeacon);

	/* ----- Utility Methods */

	// Checks the presence array to see if Entity is already known
	static bool Contains(FMassEntityHandle Entity);
	
	static bool Contains(FVector Location, TWeakObjectPtr<ABuildingParent> Building);
	
	static bool Contains(FVector Location, TWeakObjectPtr<ALDElement> LDElem);

	// Returns the Entity's cell coordinates from its Handle
	static FIntVector2 CoordsFromHandle(FMassEntityHandle Entity);

	// Returns the index of the cell at passed coordinates
	static int CoordsToIndex(FIntVector2 Coords);

	// Sets the presence array as the list of keys extracted from HandleToCoordsMap
	static void RefreshPresent();

	// Returns a const reference to the entity's data
	static const GridCellEntityData GetEntityData(FMassEntityHandle Entity);

	// Returns a reference to the entity's data
	static GridCellEntityData* GetMutableEntityData(FMassEntityHandle Entity);
	
	// Checks if the passed coordinates are inside of the grid boundaries
	static bool IsInGrid(FVector WorldCoordinates);
	
	// Checks if the passed coordinates are inside of the grid boundaries
	static bool IsInGrid(FIntVector2 GridCoordinates);
	
	// Returns all entities in a given cell
	static TArray<GridCellEntityData> GetEntitiesInCell(FIntVector2 Coordinates);

	// Returns a ref to a cell
	static HashGridCell* GetCellRef(FMassEntityHandle Handle);
	static HashGridCell* GetCellRef(FIntVector2 GridCoords);
	static HashGridCell* GetCellRef(FVector WorldCoords);

	// Converts passed coordinates as in-grid coordinates
	static FIntVector2 WorldToGridCoords(FVector WorldCoordinates);

	// Checks if the grid was generated properly
	static bool IsValid();
	
	// Returns the number of currently referenced entities
	static int32 GetEntitiesCount();

	static int32 GetMaxEntityAggroCount() { return Instance->MaxEntityAggroCount; }

	static TArray<FVector2D> GetAllEntityOfTypeOfTeam(EEntityType Type, ETeam Team);
	
	/* ----- Detection Methods ------ */

	static FDetectionResult FindClosestElementsInRange(FVector WorldCoordinates, float Range, float Angle = 360.f, FVector EntityForwardVector = FVector::ZeroVector, FMassEntityHandle Entity = FMassEntityHandle(0, 0));

	static TMap<FMassEntityHandle, GridCellEntityData> FindEntitiesInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team = ETeam::NatureTeam);
	static TArray<TWeakObjectPtr<ABuildingParent>> FindBuildingsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity);
	static TArray<TWeakObjectPtr<ALDElement>> FindLDElementsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity);
	static TArray<HashGridCell*> FindCellsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity);
	
	static TMap<FMassEntityHandle, GridCellEntityData> FindEntitiesAroundCell(FVector WorldCoordinates, int32 Range);

	static FMassEntityHandle FindClosestEntity(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team);
	static TWeakObjectPtr<ABuildingParent> FindClosestBuilding(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team);
	static TWeakObjectPtr<ALDElement> FindClosestLDElement(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team);

	static TWeakObjectPtr<ASoulBeacon> IsInSoulBeaconRange(FMassEntityHandle Entity);
	static TWeakObjectPtr<ASoulBeacon> IsInSoulBeaconRangeByCell(HashGridCell* Cell);
	static TWeakObjectPtr<ASoulBeacon> IsInSoulBeaconRange(FVector WorldCoordinates);

	/* ----- Debug Methods ----- */
	static void DebugAmalgamDetection(FVector WorldCoordinates, float Range, float Angle, FVector ForwardVector, bool Detected, FVector TargetLocation);

	static void DebugDetectionRange(FVector WorldCoordinates, float Range);
	static void DebugDetectionCheck(FVector WorldCoordinates, FVector TargetPosition, float Range, float Angle, FVector ForwardVector, bool DetectionCheckValue);
	static void DebugDetectionCone(FVector WorldCoordinates, float Range, float Angle, FVector Forward, bool Detected);
	static void DebugDetectionCell(FVector WorldCoordinates, float Range);
	static void DebugSingleDetectionCell(FVector WorldCoordinates, int XOffset, int YOffset, FColor CellColor);

	static bool DebugCheckExpr(bool Expr, const char* Msg, bool bUseAsserts);

	UFUNCTION(CallInEditor)
	void DebugAllBuildingRanges();
	UFUNCTION(CallInEditor)
	void DebugAllLDRanges();
	UFUNCTION(CallInEditor)
	void DebugGridContent();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void InitGrid();

	UFUNCTION(CallInEditor)
	void DebugGridSize();

	UFUNCTION(CallInEditor)
	void DebugGridVisuals();

private:
	static bool GenerateGrid();
	static bool GenerateGridFromCenter();

public:	
	static ASpatialHashGrid* Instance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// Defines if the grid's pivot is at it's center or top left
	bool bIsPivotCentered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CellSizeX = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CellSizeY = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int GridSizeX = 400;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int GridSizeY = 400;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxEntityAggroCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxDetectableEntities = 10; // Max number of entities detectable per cell


protected:
	// Single cell width & height in unreal units
	FIntVector2 CellSize;

	// Number of cells on grid rows & columns
	FIntVector2 GridSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//Number of entities allowed in a cell before ignoring new entrances
	int32 MaxEntitiesPerCell;

private:

	TMap<FMassEntityHandle, FIntVector2> HandleToCoordsMap;
	TArray<HashGridCell> GridCells;
	TArray<TWeakObjectPtr<ASoulBeacon>> AllSoulBeacons;
	FVector GridLocation;

	TArray<FMassEntityHandle> PresentEntities;

	UPROPERTY(EditAnywhere, Category="Grid Debug")
	bool bDebugAmalgamDetectionCone = false;

	UPROPERTY(EditAnywhere, Category="Grid Debug")
	bool bDebugAmalgamDetectionCheck = false;

	UPROPERTY(EditAnywhere, Category="Grid Debug")
	bool bDebugDetectionRange = false;

	UPROPERTY(EditAnywhere, Category="Grid Debug")
	bool bUseAsserts = false;
};
