// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Collision/SpatialHashGrid.h"

ASpatialHashGrid* ASpatialHashGrid::Instance;

// Sets default values
ASpatialHashGrid::ASpatialHashGrid()
{
	Instance = this;

	InitGrid();

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

ASpatialHashGrid::ASpatialHashGrid(bool IsPivotCentered) : bIsPivotCentered(IsPivotCentered)
{
	if(IsPivotCentered)
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

/*
* Adds the Mass Handle of an entity to the grid and references its data.
*
* @param WorldCoordinates : World Location of the entity
* @param Entity : Entity added to the grid
* @param Owner : The entity owner
* @param Transform : A pointer to the entity's transform, so that it can be referenced
* @param Health : The entity's health at the time of addition
* @return False if the adding conditions aren't met, True otherwise.
*/
bool ASpatialHashGrid::AddEntityToGrid(FVector WorldCoordinates, FMassEntityHandle Entity, FOwner Owner, FTransformFragment* Transform, float Health)
{
	FIntVector2 GridCoordinates = Instance->WorldToGridCoords(WorldCoordinates);
	checkf(IsInGrid(GridCoordinates), TEXT("SpatialHashGrid Error : Coordinates out of grid"));

	if (Contains(Entity)) return false;
	
	int CoordsIndex = CoordsToIndex(GridCoordinates);
	Instance->GridCells[CoordsIndex].Entities.Add(Entity, GridCellEntityData(Owner, Transform->GetTransform().GetLocation(), Health));
	Instance->HandleToCoordsMap.Add(Entity, GridCoordinates);

	Instance->RefreshPresent();

	return true;
}

/*
* @param Entity : Entity removed from the cell
* 
* @return False if the removing conditions aren't met, True otherwise.
*/
bool ASpatialHashGrid::RemoveEntityFromGrid(FMassEntityHandle Entity)
{
	checkf(Contains(Entity), TEXT("SpatialHashGrid Error : Trying to remove unknown entity"));

	FIntVector2 CellCoordinates = CoordsFromHandle(Entity);
	if (!IsInGrid(CellCoordinates)) return false;

	int CellIndex = CoordsToIndex(CellCoordinates);
	if (Instance->GridCells[CellIndex].Entities.IsEmpty()) return false;

	Instance->GridCells[CellIndex].Entities.Remove(Entity);
	Instance->HandleToCoordsMap.Remove(Entity);

	RefreshPresent();

	return true;
}

/*
* @param Entity : Entity to be moved
* @param WorldCoordinates : The Entity's current world coordinates
* 
* @return False if the coordinates are outside the grid, True otherwise
*/

bool ASpatialHashGrid::MoveEntityToCell(FMassEntityHandle Entity, FVector WorldCoordinates)
{
	checkf(IsInGrid(WorldCoordinates), TEXT("SpatialHashGrid Error : Coordinates out of grid"));

	FIntVector2 CurrentCellCoords = CoordsFromHandle(Entity);
	FIntVector2 NewCellCoords = Instance->WorldToGridCoords(WorldCoordinates);

	if (NewCellCoords == CurrentCellCoords) return true;

	GridCellEntityData DataToMove = Instance->GetEntityData(Entity);

	int CurrentCellIndex = CoordsToIndex(CurrentCellCoords);
	int NewCellIndex = CoordsToIndex(NewCellCoords);

	Instance->GridCells[NewCellIndex].Entities.Add(Entity, DataToMove);
	Instance->GridCells[CurrentCellIndex].Entities.Remove(Entity);

	Instance->HandleToCoordsMap[Entity] = NewCellCoords;

	return true;
}

void ASpatialHashGrid::UpdateCellTransform(FMassEntityHandle Entity, FTransform Transform)
{
	FIntVector2 CellCoords = CoordsFromHandle(Entity);
	int CellIndex = CoordsToIndex(CellCoords);
	auto GridCell = Instance->GridCells[CellIndex];
	
	checkf(GridCell.Entities.Contains(Entity), TEXT("SpatialHashGrid Error : Entity not in cell"));
	
	Instance->GridCells[CellIndex].Entities[Entity].Location = Transform.GetLocation();
}

void ASpatialHashGrid::DamageEntity(FMassEntityHandle Entity, float Damage)
{
	Instance->GetMutableEntityData(Entity)->DamageEntity(Damage);
}

bool ASpatialHashGrid::AddBuildingToGrid(FVector WorldCoordinates, ABuildingParent* Building)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Adding building out of grid"));

	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].Buildings.Contains(Building)) return false;

	Instance->GridCells[CellIndex].Buildings.Add(Building);
	return true;
}

bool ASpatialHashGrid::RemoveBuildingFromGrid(FVector WorldCoordinates, ABuildingParent* Building)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
	
	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].Buildings.IsEmpty()) return false;

	Instance->GridCells[CellIndex].Buildings.Remove(Building);
	return true;
}

bool ASpatialHashGrid::AddLDElementToGrid(FVector WorldCoordinates, ALDElement* LDElement)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Adding LD Element out of grid"));

	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].LDElements.Contains(LDElement)) return false;

	Instance->GridCells[CellIndex].LDElements.Add(LDElement);
	return true;
}

bool ASpatialHashGrid::RemoveLDElementFromGrid(FVector WorldCoordinates, ALDElement* LDElement)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));

	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].LDElements.IsEmpty()) return false;

	Instance->GridCells[CellIndex].LDElements.Remove(LDElement);
	return true;
}

bool ASpatialHashGrid::Contains(FMassEntityHandle Entity)
{
	return Instance->PresentEntities.Contains(Entity);
}

bool ASpatialHashGrid::Contains(FVector Location, ABuildingParent* Building)
{
	FIntVector2 GridCoords = WorldToGridCoords(Location);
	int Index = CoordsToIndex(GridCoords);
	return Instance->GridCells[Index].Contains(Building);
}

bool ASpatialHashGrid::Contains(FVector Location, ALDElement* LDElem)
{
	FIntVector2 GridCoords = WorldToGridCoords(Location);
	int Index = CoordsToIndex(GridCoords);
	return Instance->GridCells[Index].Contains(LDElem);
}

FIntVector2 ASpatialHashGrid::CoordsFromHandle(FMassEntityHandle Entity)
{
	return Instance->HandleToCoordsMap[Entity];
}

int ASpatialHashGrid::CoordsToIndex(FIntVector2 Coords)
{
	return Coords.X + Coords.Y * Instance->GridSize.X;
}

void ASpatialHashGrid::RefreshPresent()
{
	Instance->HandleToCoordsMap.GenerateKeyArray(Instance->PresentEntities);
}

const GridCellEntityData ASpatialHashGrid::GetEntityData(FMassEntityHandle Entity)
{
	checkf(Contains(Entity), TEXT("SpatialHashGrid Error : Accessing unknown Entity"));

	FIntVector2 EntityCoords = Instance->HandleToCoordsMap[Entity];
	checkf(IsInGrid(EntityCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
	
	int CoordsIndex = CoordsToIndex(EntityCoords);

	GridCellEntityData* ToReturn = new GridCellEntityData(FOwner(), FVector::ZeroVector, 0.f);

	if (Instance->GridCells[CoordsIndex].Contains(Entity))
	{
		ToReturn = Instance->GridCells[CoordsIndex].GetEntity(Entity);
	}

	return *ToReturn;
}

GridCellEntityData* ASpatialHashGrid::GetMutableEntityData(FMassEntityHandle Entity)
{
	FIntVector2 EntityCoords = Instance->HandleToCoordsMap[Entity];
	checkf(IsInGrid(EntityCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));

	int CoordsIndex = CoordsToIndex(EntityCoords);

	GridCellEntityData* ToReturn = new GridCellEntityData(FOwner(), FVector::ZeroVector, 0.f);

	if (Instance->GridCells[CoordsIndex].Contains(Entity))
	{
		ToReturn = Instance->GridCells[CoordsIndex].GetEntity(Entity);
	}

	return ToReturn;
}

bool ASpatialHashGrid::IsInGrid(FVector WorldCoordinates)
{
	FVector TLCellCorner = Instance->GridLocation;

	if (Instance->bIsPivotCentered)
	{
		//TLCellCorner.X -= Instance->CellSize.X * (GetGridSize().X / 2 + GetGridSize().Y % 2 * .5f);
		//TLCellCorner.Y -= Instance->CellSize.Y * (GetGridSize().Y / 2 + GetGridSize().Y % 2 * .5f);
	}

	FVector BRCellCorner = TLCellCorner + FVector(GetGridSize().X * Instance->CellSize.X, GetGridSize().Y * Instance->CellSize.Y, .0f);;

	bool bIsOutGrid = (WorldCoordinates.X < TLCellCorner.X
		|| WorldCoordinates.X > BRCellCorner.X
		|| WorldCoordinates.Y < TLCellCorner.Y
		|| WorldCoordinates.Y > BRCellCorner.Y);

	return !bIsOutGrid;
}

bool ASpatialHashGrid::IsInGrid(FIntVector2 GridCoordinates)
{
	if (GridCoordinates.X < 0 || GridCoordinates.Y < 0 || GridCoordinates.X >= Instance->GridSize.X || GridCoordinates.Y >= Instance->GridSize.Y)
		return false;

	return true;
}

TArray<GridCellEntityData> ASpatialHashGrid::GetEntitiesInCell(FIntVector2 Coordinates)
{
	int CellIndex = CoordsToIndex(Coordinates);
	TArray<GridCellEntityData> Entities = Instance->GridCells[CellIndex].GetEntities();
	return Entities;
}

FIntVector2 ASpatialHashGrid::WorldToGridCoords(FVector WorldCoordinates)
{
	checkf(IsInGrid(WorldCoordinates), TEXT("SpatialHashGrid : Entity Coordinates out of grid"));

	FVector TopLeftCellCoordinates = Instance->GridLocation;

	if (Instance->bIsPivotCentered)
	{
		//TopLeftCellCoordinates.X -= Instance->CellSize.X * (GetGridSize().X / 2 + GetGridSize().X % 2 * .5f);
		//TopLeftCellCoordinates.Y -= Instance->CellSize.Y * (GetGridSize().Y / 2 + GetGridSize().Y % 2 * .5f);
	}

	FVector InGridCoordinates = WorldCoordinates - TopLeftCellCoordinates;

	int32 XCoordinates = FMath::FloorToInt(InGridCoordinates.X / Instance->CellSize.X);
	int32 YCoordinates = FMath::FloorToInt(InGridCoordinates.Y / Instance->CellSize.Y);

	return FIntVector2(XCoordinates, YCoordinates);
}

bool ASpatialHashGrid::IsValid()
{
	return !Instance->GridCells.IsEmpty();
}

int32 ASpatialHashGrid::GetEntitiesCount()
{
	return Instance->PresentEntities.Num();
}

/*
* Returns an array of entites gathered from cells up to a Range distance from the center cell
* @param WorldCoordinates Position of the entity in the center cell
* @param Range Distance to the farthest cell in which to search
* 
* @return Returns the data linked to the found entities
*/
TMap<FMassEntityHandle, GridCellEntityData> ASpatialHashGrid::FindEntitiesInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity)
{
	TMap<FMassEntityHandle, GridCellEntityData> FoundEntities;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	FVector Location = Instance->GetActorLocation();

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			TArray<FMassEntityHandle> Keys;
			Instance->GridCells[Index].Entities.GenerateKeyArray(Keys);

			for (auto Key : Keys)
			{
				GridCellEntityData Data = Instance->GridCells[Index].Entities[Key];
				
				//DrawDebugLine(Instance->GetWorld(), DetectionCenter, Data.Location, FColor::Blue, false, 3.f, 0, 100);
				
				auto Direction = (Data.Location - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector)));
				if (AngleBetween <= Angle) FoundEntities.Add(Key, Data);
			}
		}
	}
	FoundEntities.Remove(Entity); // Prevents entity finding itself

	return FoundEntities;
}



TArray<ABuildingParent*> ASpatialHashGrid::FindBuildingsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity)
{
	TArray<ABuildingParent*> FoundBuildings;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			for (auto Building : Instance->GridCells[Index].GetBuildings())
			{
				auto Direction = (Building->GetActorLocation() - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector)));
				if (AngleBetween <= Angle) FoundBuildings.Add(Building);
			}
		}
	}
	
	return FoundBuildings;
}

TArray<ALDElement*> ASpatialHashGrid::FindLDElementsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity)
{
	TArray<ALDElement*> FoundBuildings;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			for (auto Elem : Instance->GridCells[Index].GetLDElements())
			{
				auto Direction = (Elem->GetActorLocation() - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector)));
				if (AngleBetween <= Angle) FoundBuildings.Add(Elem);
			}
		}
	}

	return FoundBuildings;
}

TMap<FMassEntityHandle, GridCellEntityData> ASpatialHashGrid::FindEntitiesAroundCell(FVector WorldCoordinates, int32 Range)
{
	TMap<FMassEntityHandle, GridCellEntityData> FoundEntities;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	for (int x = -Range; x <= Range; ++x)
	{
		for (int y = -Range; y <= Range; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;


			FoundEntities.Append(Instance->GridCells[Index].Entities);
		}
	}

	return FoundEntities;
}

FMassEntityHandle ASpatialHashGrid::FindClosestEntity(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team)
{
	TMap<FMassEntityHandle, GridCellEntityData> FoundEntities = Instance->FindEntitiesInRange(WorldCoordinates, Range, Angle, EntityForwardVector, Entity);

	if (FoundEntities.Num() == 0) return FMassEntityHandle(0,0); // Returns unset handle

	TArray<FMassEntityHandle> KeyArray;
	FoundEntities.GenerateKeyArray(KeyArray);


	FMassEntityHandle Closest = FMassEntityHandle(0, 0); //KeyArray[0];
	GridCellEntityData* ClosestData = nullptr; //&FoundEntities[Closest];
	float ClosestDistance = std::numeric_limits<float>::max(); //(ClosestData->Location - WorldCoordinates).Length();

	for (auto Key : KeyArray)
	{
		GridCellEntityData* Next = &FoundEntities[Key];
		float NextDistance = (Next->Location - WorldCoordinates).Length();

		if (Next->AggroCount >= GetMaxEntityAggroCount()) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("FindClosestEntity \n\t Entity skipped > Max Aggro Reached"));

		if (NextDistance > ClosestDistance || Next->Owner.Team == Team || Next->AggroCount >= GetMaxEntityAggroCount()) continue;

		Closest = Key;
		ClosestData = Next;
		ClosestDistance = NextDistance;
	}

	return Closest;
}

ABuildingParent* ASpatialHashGrid::FindClosestBuilding(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team)
{
	TArray<ABuildingParent*> FoundBuildings = Instance->FindBuildingsInRange(WorldCoordinates, Range, Angle, EntityForwardVector, Entity);

	if (FoundBuildings.Num() == 0) return nullptr;

	ABuildingParent* Closest = nullptr;
	float ClosestDistance = std::numeric_limits<float>::max();
	
	for (auto Building : FoundBuildings)
	{
		float NextDistance = (Building->GetActorLocation() - WorldCoordinates).Length();
		if (NextDistance > ClosestDistance || Building->GetOwner().Team == Team) continue;

		Closest = Building;
		ClosestDistance = NextDistance;
	}

	return Closest;
}

/*
* @todo : Remake evaluation after LDElements have teams (if they ever have)
*/
ALDElement* ASpatialHashGrid::FindClosestLDElement(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team)
{
	TArray<ALDElement*> FoundElems = Instance->FindLDElementsInRange(WorldCoordinates, Range, Angle, EntityForwardVector, Entity);

	if (FoundElems.Num() == 0) return nullptr;

	float ClosestDistance = (FoundElems[0]->GetActorLocation() - WorldCoordinates).Length();
	ALDElement* Closest = FoundElems[0];

	for (auto Elem : FoundElems)
	{
		float NextDistance = (Elem->GetActorLocation() - WorldCoordinates).Length();
		if (NextDistance > ClosestDistance ) continue;

		Closest = Elem;
		ClosestDistance = NextDistance;
	}

	return Closest;
}

void ASpatialHashGrid::DebugDetectionRange(FVector WorldCoordinates, float Range)
{
	DrawDebugSphere(Instance->GetWorld(), WorldCoordinates, Range, 10, FColor::Orange, false, 10.f);
}

void ASpatialHashGrid::DebugDetectionCheck(FVector WorldCoordinates, FVector TargetPosition, float Range, float Angle, FVector ForwardVector, bool DetectionCheckValue)
{
	FColor DebugColor;
	if (DetectionCheckValue)
		DebugColor = FColor::Green;
	else
		DebugColor = FColor::Red;

	DrawDebugLine(Instance->GetWorld(), WorldCoordinates, TargetPosition, FColor::Red, false, 10.f);
}

// Called when the game starts or when spawned
void ASpatialHashGrid::BeginPlay()
{
	Super::BeginPlay();
	//Instance->GridCells.Empty();
	//Generate();
	Instance->GridLocation = GetActorLocation();
	InitGrid();
}

void ASpatialHashGrid::InitGrid()
{
	Instance->GridLocation = GetActorLocation();
	CellSize = FIntVector2(CellSizeX, CellSizeY);
	GridSize = FIntVector2(GridSizeX, GridSizeY);
	UE_LOG(LogTemp, Warning, TEXT("Grid Size : %d ; %d"), GridSize.X, GridSize.Y);
	UE_LOG(LogTemp, Warning, TEXT("Cell Size : %d ; %d"), CellSize.X, CellSize.Y);
	//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Grid Size : %d ; %d"), GridSize.X, GridSize.Y));
	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("yellow hello"));

	Generate();
}

void ASpatialHashGrid::DebugGridSize()
{
	UE_LOG(LogTemp, Warning, TEXT("Grid Size : %d ; %d"), GridSize.X, GridSize.Y);
	UE_LOG(LogTemp, Warning, TEXT("Cell Size : %d ; %d"), CellSize.X, CellSize.Y);
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Grid Size : %d ; %d"), GridSize.X, GridSize.Y));
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Cell Grid Size : %d ; %d"), CellSize.X, CellSize.Y));
}

void ASpatialHashGrid::DebugGridVisuals()
{
	//Debug draw line
	FVector GridBase = Instance->GridLocation;
	if (bIsPivotCentered) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Grid is Centered, will be debuged latter")));

	for (int i = 0; i < GridSizeY; ++i)
	{
		FVector Start = GridBase + FVector(0, i * CellSizeY, 50);
		FVector End = Start + FVector(GridSizeX * CellSizeX, i * CellSizeY, 50);
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 10.0f, 0, 100.f);
	}
	for (int i = 0; i < GridSizeX; ++i)
	{
		FVector Start = GridBase + FVector(i * CellSizeX, 0, 50);
		FVector End = Start + FVector(i * CellSizeX, GridSizeY * CellSizeY, 50);
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 10.0f, 0, 100.f);
	}
}

/*
* Differentiation between GenerateFromCenter & Generate is only useful if grid has a Gizmo
*/
bool ASpatialHashGrid::Generate()
{
	if(Instance->bIsPivotCentered)
		return GenerateGridFromCenter();

	return GenerateGrid();
}

bool ASpatialHashGrid::GenerateGrid()
{
	Instance->GridCells = TArray<HashGridCell>();

	for (int32 Index = 0; Index < Instance->GridSize.X * Instance->GridSize.Y; ++Index)
	{
		Instance->GridCells.Add(HashGridCell());
	}

	//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, FString::Printf(TEXT("SHG / Generate Grid : %d cells in grid"), Instance->GridCells.Num()));
	return true;
}

bool ASpatialHashGrid::GenerateGridFromCenter()
{
	Instance->GridCells = TArray<HashGridCell>();

	for (int32 Index = 0; Index < Instance->GridSize.X * Instance->GridSize.Y; ++Index)
	{
		Instance->GridCells.Add(HashGridCell());
	}

	return true;
}

// Called every frame
void ASpatialHashGrid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void GridCellEntityData::DamageEntity(float Damage)
{
	EntityHealth -= Damage;
	GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("Damage inflicted : %f \n\t New Health : %f"), Damage, EntityHealth));
}

GridCellEntityData* HashGridCell::GetEntity(FMassEntityHandle Entity)
{
	return &Entities[Entity];
}

const TArray<GridCellEntityData> HashGridCell::GetEntities()
{
	TArray<GridCellEntityData> ValueArray;
	Entities.GenerateValueArray(ValueArray);
	const TArray<GridCellEntityData> ConstCopy = ValueArray;
	
	return ConstCopy;
}

TArray<GridCellEntityData> HashGridCell::GetMutableEntities()
{
	TArray<GridCellEntityData> ValueArray;
	Entities.GenerateValueArray(ValueArray);
	return ValueArray;
}

TArray<ABuildingParent*> HashGridCell::GetBuildings()
{
	TArray<ABuildingParent*> OutBuildings = Buildings;
	return OutBuildings;
}

TArray<ALDElement*> HashGridCell::GetLDElements()
{
	TArray<ALDElement*> OutLDElems = LDElements;
	return OutLDElems;
}

bool HashGridCell::Contains(FMassEntityHandle Entity)
{
	return Entities.Contains(Entity);
}

bool HashGridCell::Contains(ABuildingParent* Building)
{
	return Buildings.Contains(Building);
}

bool HashGridCell::Contains(ALDElement* LDElement)
{
	return LDElements.Contains(LDElement);
}

int HashGridCell::GetTotalNumByTeam(FOwner Owner)
{
	ETeam Team = Owner.Team;

	TArray<GridCellEntityData> EntityValues;
	Entities.GenerateValueArray(EntityValues);

	int NumEntities = EntityValues.FilterByPredicate([&Team](GridCellEntityData EntityData)
		{
			return EntityData.Owner.Team == Team;
		}).Num();

	int NumBuildings = Buildings.FilterByPredicate([&Team](ABuildingParent* Building)
		{
			return Building->GetOwner().Team == Team;
		}).Num();

	return NumEntities + NumBuildings;
}

int HashGridCell::GetTotalNumByTeamDifference(FOwner Owner)
{
	return GetTotalNum() - GetTotalNumByTeam(Owner);
}