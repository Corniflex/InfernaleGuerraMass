// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MassCommonFragments.h"
#include "Mass/Army/AmalgamFragments.h"
#include "LD/Buildings/BuildingParent.h"
#include <LD/LDElement/LDElement.h>

#include "SpatialHashGrid.generated.h"

struct GridCellEntityData
{
	FOwner Owner;
	FVector Location;
	
	float MaxEntityHealth;
	float EntityHealth;

	int AggroCount = 0;

	GridCellEntityData(FOwner EntityOwner, FVector EntityLocation, float Health) : Owner(EntityOwner), Location(EntityLocation), MaxEntityHealth(Health), EntityHealth(Health)
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
		return GridCellEntityData(FOwner(), FVector::ZeroVector, 0.f);
	}
};

struct HashGridCell
{
	TMap<FMassEntityHandle, GridCellEntityData> Entities = TMap <FMassEntityHandle, GridCellEntityData>();
	TArray<ABuildingParent*> Buildings = TArray<ABuildingParent*>();
	TArray<ALDElement*> LDElements = TArray<ALDElement*>();

	/* Getters */

	GridCellEntityData* GetEntity(FMassEntityHandle Entity);

	const TArray<GridCellEntityData> GetEntities();
	TArray<GridCellEntityData> GetMutableEntities();
	TArray<GridCellEntityData> GetEntitiesByTeamDifference(FOwner Owner);

	TArray<ABuildingParent*> GetBuildings();

	TArray<ALDElement*> GetLDElements();

	/* Contains methods */

	bool Contains(FMassEntityHandle Entity);
	bool Contains(ABuildingParent* Building);
	bool Contains(ALDElement* LDElement);

	/* Num methods */
	int GetTotalNum() { return Entities.Num() + Buildings.Num() + LDElements.Num(); }
	int GetEntitiesNum() { return Entities.Num(); }
	int GetBuildingsNum() { return Buildings.Num(); }
	int GetLDElementsNum() { return LDElements.Num(); }

	int GetTotalNumByTeam(FOwner Owner);
	int GetTotalNumByTeamDifference(FOwner Owner);
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
	static bool AddEntityToGrid(FVector WorldCoordinates, FMassEntityHandle Entity, FOwner Owner, FTransformFragment* Transform, float Health);

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

	/* ----- Utility Methods */

	// Checks the presence array to see if Entity is already known
	static bool Contains(FMassEntityHandle Entity);
	
	static bool Contains(FVector Location, ABuildingParent* Building);
	
	static bool Contains(FVector Location, ALDElement* LDElem);

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

	// Converts passed coordinates as in-grid coordinates
	static FIntVector2 WorldToGridCoords(FVector WorldCoordinates);

	// Checks if the grid was generated properly
	static bool IsValid();
	
	// Returns the number of currently referenced entities
	static int32 GetEntitiesCount();

	static int32 GetMaxEntityAggroCount() { return Instance->MaxEntityAggroCount; }
	
	/* ----- Detection Methods ------ */

	static TMap<FMassEntityHandle, GridCellEntityData> FindEntitiesInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity);
	static TArray<ABuildingParent*> FindBuildingsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity);
	static TArray<ALDElement*> FindLDElementsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity);
	
	static TMap<FMassEntityHandle, GridCellEntityData> FindEntitiesAroundCell(FVector WorldCoordinates, int32 Range);

	static FMassEntityHandle FindClosestEntity(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team);
	static ABuildingParent* FindClosestBuilding(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team);
	static ALDElement* FindClosestLDElement(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team);

	/* ----- Debug Methods ----- */
	static void DebugDetectionRange(FVector WorldCoordinates, float Range);
	static void DebugDetectionCheck(FVector WorldCoordinates, FVector TargetPosition, float Range, float Angle, FVector ForwardVector, bool DetectionCheckValue);

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
	FVector GridLocation;

	/*Old*/
	TArray<FMassEntityHandle> PresentEntities;
};
