// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Collision/SpatialHashGrid.h"
#include <LD/LDElement/SoulBeacon.h>
#include <LD/LDElement/NeutralCamp.h>

#include "LeData/DataGathererActor.h"

ASpatialHashGrid* ASpatialHashGrid::Instance;

// Sets default values
ASpatialHashGrid::ASpatialHashGrid()
{
	Instance = this;

	PrimaryActorTick.bCanEverTick = false;
}

ASpatialHashGrid::ASpatialHashGrid(bool IsPivotCentered) : bIsPivotCentered(IsPivotCentered)
{
	Instance = this;
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
bool ASpatialHashGrid::AddEntityToGrid(FVector WorldCoordinates, FMassEntityHandle Entity, FOwner Owner, FTransformFragment* Transform, float Health, float TargetableRadius, EEntityType Type)
{
	FIntVector2 GridCoordinates = Instance->WorldToGridCoords(WorldCoordinates);
	//checkf(IsInGrid(GridCoordinates), TEXT("SpatialHashGrid Error : Coordinates out of grid"));
	if (!DebugCheckExpr(IsInGrid(GridCoordinates), "SpatialHashGrid Error : Coordinates out of grid", Instance->bUseAsserts)) return false;

	if (Contains(Entity)) return false;
	
	int CoordsIndex = CoordsToIndex(GridCoordinates);
	Instance->GridCells[CoordsIndex].Entities.Add(Entity, GridCellEntityData(Owner, Transform->GetTransform().GetLocation(), Health, TargetableRadius, Type));
	Instance->HandleToCoordsMap.Add(Entity, GridCoordinates);

	Instance->GridCells[CoordsIndex].UpdatePresentOwners();

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
	//checkf(Contains(Entity), TEXT("SpatialHashGrid Error : Trying to remove unknown entity"));

	if(!DebugCheckExpr(Contains(Entity), "SpatialHashGrid Error : Trying to remove unknown entity", Instance->bUseAsserts)) return false;

	FIntVector2 CellCoordinates = CoordsFromHandle(Entity);
	if (!IsInGrid(CellCoordinates)) return false;

	int CellIndex = CoordsToIndex(CellCoordinates);
	if (Instance->GridCells[CellIndex].Entities.IsEmpty()) return false;

	Instance->GridCells[CellIndex].Entities.Remove(Entity);
	Instance->HandleToCoordsMap.Remove(Entity);

	Instance->GridCells[CellIndex].UpdatePresentOwners();

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
	//checkf(IsInGrid(WorldCoordinates), TEXT("SpatialHashGrid Error : Coordinates out of grid"));

	if (!DebugCheckExpr(IsInGrid(WorldCoordinates), "SpatialHashGrid Error : Coordinates out of grid", Instance->bUseAsserts)) return false;

	FIntVector2 CurrentCellCoords = CoordsFromHandle(Entity);
	FIntVector2 NewCellCoords = Instance->WorldToGridCoords(WorldCoordinates);

	if (NewCellCoords == CurrentCellCoords) return true;

	GridCellEntityData DataToMove = Instance->GetEntityData(Entity);

	int CurrentCellIndex = CoordsToIndex(CurrentCellCoords);
	int NewCellIndex = CoordsToIndex(NewCellCoords);

	Instance->GridCells[NewCellIndex].Entities.Add(Entity, DataToMove);
	Instance->GridCells[CurrentCellIndex].Entities.Remove(Entity);

	Instance->GridCells[NewCellIndex].UpdatePresentOwners();
	Instance->GridCells[CurrentCellIndex].UpdatePresentOwners();

	Instance->HandleToCoordsMap[Entity] = NewCellCoords;

	return true;
}

void ASpatialHashGrid::UpdateCellTransform(FMassEntityHandle Entity, FTransform Transform)
{
	FIntVector2 CellCoords = CoordsFromHandle(Entity);
	int CellIndex = CoordsToIndex(CellCoords);
	auto GridCell = Instance->GridCells[CellIndex];
	
	//checkf(GridCell.Entities.Contains(Entity), TEXT("SpatialHashGrid Error : Entity not in cell"));
	if(!DebugCheckExpr(GridCell.Entities.Contains(Entity), "SpatialHashGrid Error : Entity not in cell", Instance->bUseAsserts)) return;

	Instance->GridCells[CellIndex].Entities[Entity].Location = Transform.GetLocation();
}

void ASpatialHashGrid::DamageEntity(FMassEntityHandle Entity, float Damage)
{
	Instance->GetMutableEntityData(Entity)->DamageEntity(Damage);
}

bool ASpatialHashGrid::AddBuildingToGrid(FVector WorldCoordinates, ABuildingParent* Building)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	//checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Adding building out of grid"));
	if (!DebugCheckExpr(IsInGrid(GridCoords), "SpatialHashGrid Error : Adding building out of grid", Instance->bUseAsserts)) return false;;

	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].Buildings.Contains(Building)) return false;

	// Reference to initial Cell
	Instance->GridCells[CellIndex].Buildings.Add(Building);

	// Reference to cells in range
	TArray<HashGridCell*> CellsInRange = FindCellsInRange(WorldCoordinates, Building->GetTargetableRange(), 360.f, Building->GetActorForwardVector(), FMassEntityHandle(0, 0));
	for (auto Cell : CellsInRange)
		if (!Cell->Buildings.Contains(Building))
			Cell->Buildings.Add(Building);

	return true;
}

bool ASpatialHashGrid::RemoveBuildingFromGrid(FVector WorldCoordinates, ABuildingParent* Building)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	//checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
	if(!DebugCheckExpr(IsInGrid(GridCoords), "SpatialHashGrid Error : Accessing coords out of grid", Instance->bUseAsserts)) return false;

	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].Buildings.IsEmpty()) return false;

	Instance->GridCells[CellIndex].Buildings.Remove(Building);
	return true;
}

bool ASpatialHashGrid::AddLDElementToGrid(FVector WorldCoordinates, ALDElement* LDElement)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	//checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Adding LD Element out of grid"));
	if(!DebugCheckExpr(IsInGrid(GridCoords), "SpatialHashGrid Error : Adding LD Element out of grid", Instance->bUseAsserts))
	{
		// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("SpatialHashGrid Error : Adding LD Element out of grid"));
		return false;
	}

	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].LDElements.Contains(LDElement))
	{
		// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("SpatialHashGrid Error : LDElement already in grid"));
		return false;
	}

	Instance->GridCells[CellIndex].LDElements.Add(LDElement);
	// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("LDElement added to grid at %s"), *WorldCoordinates.ToString()));
	return true;
}

bool ASpatialHashGrid::RemoveLDElementFromGrid(FVector WorldCoordinates, ALDElement* LDElement)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	//checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
	if(!DebugCheckExpr(IsInGrid(GridCoords), "SpatialHashGrid Error : Accessing coords out of grid", Instance->bUseAsserts))
	{
		// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
		return false;
	}
	int CellIndex = CoordsToIndex(GridCoords);
	if (Instance->GridCells[CellIndex].LDElements.IsEmpty())
	{
		// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("SpatialHashGrid Error : LDElement not in grid"));
		return false;
	}

	Instance->GridCells[CellIndex].LDElements.Remove(LDElement);
	// GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("LDElement removed from grid"));
	return true;
}

bool ASpatialHashGrid::AddSoulBeaconToGrid(ASoulBeacon* SoulBeacon)
{
	if (Instance->AllSoulBeacons.Contains(SoulBeacon)) return false;
	const FVector Location = SoulBeacon->GetActorLocation();
	FIntVector2 GridCoords = Instance->WorldToGridCoords(Location);
	
	//checkf(IsInGrid(GridCoords), TEXT("SpatialHashGrid Error : Adding Soul Beacon out of grid"));
	if (!DebugCheckExpr(IsInGrid(GridCoords), "SpatialHashGrid Error : Adding Soul Beacon out of grid", Instance->bUseAsserts)) return false;

	int CellIndex = CoordsToIndex(GridCoords);
	Instance->AllSoulBeacons.Add(SoulBeacon);
	
	TArray<HashGridCell*> CaptureCells = TArray<HashGridCell*>();
	TArray<HashGridCell*> EffectCells = TArray<HashGridCell*>();
	
	for(const auto& Pt : SoulBeacon->GetCaptureRanges())
		CaptureCells.Append(FindCellsInRange(Pt.GetPosition(), Pt.Radius, 360.f, SoulBeacon->GetActorForwardVector(), FMassEntityHandle(0, 0)));

	for(const auto& Pt : SoulBeacon->GetEffectRanges())
		EffectCells.Append(FindCellsInRange(Pt.GetPosition(), Pt.Radius, 360.f, SoulBeacon->GetActorForwardVector(), FMassEntityHandle(0, 0)));

	TArray<HashGridCell*> CaptureCellsCleaned = TArray<HashGridCell*>();
	TArray<HashGridCell*> EffectCellsCleaned = TArray<HashGridCell*>();

	for (const auto& Elem : CaptureCells)
		CaptureCellsCleaned.AddUnique(Elem);
	for (const auto& Elem : EffectCells)
		EffectCellsCleaned.AddUnique(Elem);

	SoulBeacon->AddCells(CaptureCells);
	for (auto& Cell : CaptureCellsCleaned)
		Cell->LinkedBeacon = SoulBeacon;

	for (auto& Cell : EffectCellsCleaned)
		Cell->LinkedBeacon = SoulBeacon;

	return true;
}

bool ASpatialHashGrid::Contains(FMassEntityHandle Entity)
{
	return Instance->PresentEntities.Contains(Entity);
}

bool ASpatialHashGrid::Contains(FVector Location, TWeakObjectPtr<ABuildingParent> Building)
{
	FIntVector2 GridCoords = WorldToGridCoords(Location);
	int Index = CoordsToIndex(GridCoords);
	return Instance->GridCells[Index].Contains(Building);
}

bool ASpatialHashGrid::Contains(FVector Location, TWeakObjectPtr<ALDElement> LDElem)
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
	if (!Contains(Entity)) return GridCellEntityData::None();
	
	//checkf(Contains(Entity), TEXT("SpatialHashGrid Error : Accessing unknown Entity"));

	FIntVector2 EntityCoords = Instance->HandleToCoordsMap[Entity];
	//checkf(IsInGrid(EntityCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
	if (!IsInGrid(EntityCoords)) return GridCellEntityData::None();
	
	int CoordsIndex = CoordsToIndex(EntityCoords);

	GridCellEntityData* ToReturn = new GridCellEntityData(FOwner(), FVector::ZeroVector, 0.f, 0.f, EEntityType::EntityTypeNone);

	if (Instance->GridCells[CoordsIndex].Contains(Entity))
	{
		ToReturn = Instance->GridCells[CoordsIndex].GetEntity(Entity);
	}

	return *ToReturn;
}

GridCellEntityData* ASpatialHashGrid::GetMutableEntityData(FMassEntityHandle Entity)
{
	FIntVector2 EntityCoords = Instance->HandleToCoordsMap[Entity];
	//checkf(IsInGrid(EntityCoords), TEXT("SpatialHashGrid Error : Accessing coords out of grid"));
	if (!DebugCheckExpr(IsInGrid(EntityCoords), "SpatialHashGrid Error : Accessing coords out of grid", Instance->bUseAsserts)) return nullptr;

	int CoordsIndex = CoordsToIndex(EntityCoords);

	GridCellEntityData* ToReturn = new GridCellEntityData(FOwner(), FVector::ZeroVector, 0.f, 0.f, EEntityType::EntityTypeNone);

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

HashGridCell* ASpatialHashGrid::GetCellRef(FMassEntityHandle Handle)
{
	int32 Index = CoordsToIndex(CoordsFromHandle(Handle));
	return &Instance->GridCells[Index];
}

HashGridCell* ASpatialHashGrid::GetCellRef(FIntVector2 GridCoords)
{
	int32 Index = CoordsToIndex(GridCoords);
	return &Instance->GridCells[Index];
}

HashGridCell* ASpatialHashGrid::GetCellRef(FVector WorldCoords)
{
	int32 Index = CoordsToIndex(WorldToGridCoords(WorldCoords));
	return &Instance->GridCells[Index];
}

FIntVector2 ASpatialHashGrid::WorldToGridCoords(FVector WorldCoordinates)
{
	//checkf(IsInGrid(WorldCoordinates), TEXT("SpatialHashGrid : Entity Coordinates out of grid"));
	if(!DebugCheckExpr(IsInGrid(WorldCoordinates), "SpatialHashGrid Error : Entity Coordinates out of grid", Instance->bUseAsserts)) return FIntVector2(0,0);

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


TArray<FVector2D> ASpatialHashGrid::GetAllEntityOfTypeOfTeam(EEntityType Type, ETeam Team)
{
	TArray<FVector2D> Entities;
	FCriticalSection Mutex;
	Entities.Reserve(1500); // estimation

	ParallelFor(Instance->GridCells.Num(), [&](int32 Index)
	{
		const auto& Cell = Instance->GridCells[Index];
		TArray<FVector2D> LocalEntities;

		for (const auto& Pair : Cell.Entities)
		{
			if (const auto& EntityData = Pair.Value; EntityData.EntityType == Type && EntityData.Owner.Team == Team)
			{
				LocalEntities.Add(FVector2D(EntityData.Location.X, EntityData.Location.Y));
			}
		}

		if (LocalEntities.Num() > 0)
		{
			FScopeLock Lock(&Mutex);
			Entities.Append(LocalEntities);
		}
	});
	return Entities;
}

FDetectionResult ASpatialHashGrid::FindClosestElementsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity)
{
	FDetectionResult Result;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	int32 DetectedEntities = 0;

	ETeam CallerTeam = GetEntityData(Entity).Owner.Team;

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			TArray<FMassEntityHandle> Keys;
			Instance->GridCells[Index].Entities.GenerateKeyArray(Keys);

			#if WITH_EDITOR
			int FoundCounter = 0;
			bool FullDetected = false;
			#endif

			for (auto Key : Keys)
			{
				GridCellEntityData Data = Instance->GridCells[Index].Entities[Key];
				if (CallerTeam == Data.Owner.Team) continue;

				#if WITH_EDITOR
					++FoundCounter;
				#endif
				
				auto Direction = (Data.Location - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector)));
				if (AngleBetween >= Angle) continue;
				
				auto Distance = (Data.Location - DetectionCenter).Length() - Data.TargetableRadius;
				if (Distance > Range || Distance > Result.EntityDistance) continue;

				#if WITH_EDITOR
					FullDetected = true;
				#endif

				Result.EntityDistance = Distance;
				Result.Entity = Key;

				/*FColor DebugColor = AngleBetween <= Angle ? FColor::Green : FColor::Red;
				DrawDebugLine(Instance->GetWorld(), DetectionCenter, Data.Location, DebugColor, false, 3.f, 0, 100);*/
			}
			
			for (auto Building : Instance->GridCells[Index].GetBuildings())
			{
				if (!Building.IsValid()) continue;
				if (Building->GetOwner().Team == CallerTeam) continue;

				#if WITH_EDITOR
					++FoundCounter;
				#endif

				auto Direction = (Building->GetActorLocation() - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector.GetSafeNormal())));
				if (AngleBetween >= Angle) continue;
				
				float Distance = (Building->GetActorLocation() - DetectionCenter).Length() - Building->GetTargetableRange();
				if (Distance > Range || Distance > Result.BuildingDistance) continue;

				#if WITH_EDITOR
				FullDetected = true;
				#endif
				
				Result.BuildingDistance = Distance;
				Result.Building = Building;
			}

			for (auto LD : Instance->GridCells[Index].GetLDElements())
			{
				#if WITH_EDITOR
					++FoundCounter;
				#endif
				
				float LDTargetableRange = 0.f;
				if (LD->GetLDElementType() == ELDElementType::LDElementNeutralCampType)
					LDTargetableRange = Cast<ANeutralCamp>(LD)->GetTargetableRange();

				auto Direction = (LD->GetActorLocation() - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector.GetSafeNormal())));
				if (AngleBetween >= Angle) continue;

				float Distance = (LD->GetActorLocation() - DetectionCenter).Length() - LDTargetableRange;
				if (Distance > Range || Distance > Result.LDDistance) continue;

				#if WITH_EDITOR
				FullDetected = true;
				#endif

				Result.LDDistance = Distance;
				Result.LD = LD;
			}

			//#if WITH_EDITOR
			//FColor Color = FullDetected ? FColor::Green : (FoundCounter != 0 ? FColor::Yellow : FColor::Red);
			//DebugSingleDetectionCell(WorldCoordinates, x, y, Color);
			//#endif // WITH_EDITOR

		}
	}

	return Result;
}

/*
* Returns an array of entites gathered from cells up to a Range distance from the center cell
* @param WorldCoordinates Position of the entity in the center cell
* @param Range Distance to the farthest cell in which to search
* 
* @return Returns the data linked to the found entities
*/
TMap<FMassEntityHandle, GridCellEntityData> ASpatialHashGrid::FindEntitiesInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team)
{
	TMap<FMassEntityHandle, GridCellEntityData> FoundEntities;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	int32 DetectedEntities = 0;

	ETeam CallerTeam =  Contains(Entity) ? GetEntityData(Entity).Owner.Team : Team;

	//DrawDebugLine(Instance->GetWorld(), DetectionCenter, DetectionCenter + EntityForwardVector * 1000000, FColor::Purple, false, 10.f, 10U);

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
				
				
				auto Distance = (Data.Location - DetectionCenter).Length() - Data.TargetableRadius;
				if (Distance > Range) continue;
				auto Direction = (Data.Location - DetectionCenter).GetSafeNormal();

				auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector)));
				
				// DebugAmalgamDetection(DetectionCenter, Range, Angle, EntityForwardVector, AngleBetween <= Angle, Data.Location);
				
				if (AngleBetween <= Angle) FoundEntities.Add(Key, Data);
				if (CallerTeam != Data.Owner.Team)
				{
					FColor DebugColor = AngleBetween <= Angle ? FColor::Green : FColor::Red;
					//DrawDebugLine(Instance->GetWorld(), DetectionCenter, Data.Location, DebugColor, false, 3.f, 0, 100);
				}

				if(Key != Entity)
					++DetectedEntities;

				if (DetectedEntities >= Instance->MaxDetectableEntities)
				{
					FoundEntities.Remove(Entity);
					return FoundEntities;
				}
			}
		}
	}
	FoundEntities.Remove(Entity); // Prevents entity finding itself

	return FoundEntities;
}



TArray<TWeakObjectPtr<ABuildingParent>> ASpatialHashGrid::FindBuildingsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity)
{
	TArray<TWeakObjectPtr<ABuildingParent>> FoundBuildings;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	int32 DetectableEntities = 0;
	float DetectorTargetableRange = 62.5f;

	if (Instance->PresentEntities.Contains(Entity))
		DetectorTargetableRange = Instance->GetEntityData(Entity).TargetableRadius;

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			for (auto Building : Instance->GridCells[Index].GetBuildings())
			{
				//DrawDebugSphere(Instance->GetWorld(), Building->GetActorLocation(), Building->GetTargetableRange(), 10, FColor::Orange, false, 2.f);
				if (!Building.IsValid()) continue;
				float Distance = (Building->GetActorLocation() - DetectionCenter).Length() - Building->GetTargetableRange() - DetectorTargetableRange;
				if (Distance > Range)
				{
					//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, FString::Printf(TEXT("Building out of range")));
					continue;
				}

				auto Direction = (Building->GetActorLocation() - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector.GetSafeNormal())));

				//DebugAmalgamDetection(DetectionCenter, Range, Angle, EntityForwardVector, AngleBetween <= Angle, Building->GetActorLocation());
				if (AngleBetween <= Angle && !FoundBuildings.Contains(Building))	FoundBuildings.Add(Building);
				
				FColor DebugColor = AngleBetween <= Angle ? FColor::Green : FColor::Red;

				//DrawDebugLine(Instance->GetWorld(), DetectionCenter, Building->GetActorLocation(), DebugColor, false, 2.f);
				
				++DetectableEntities;

				if (DetectableEntities >= Instance->MaxDetectableEntities) 
					return FoundBuildings;
			}
		}
	}
	
	return FoundBuildings;
}

TArray<TWeakObjectPtr<ALDElement>> ASpatialHashGrid::FindLDElementsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity)
{
	TArray<TWeakObjectPtr<ALDElement>> FoundLD;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	int32 DetectableEntities = 0;
	float DetectorTargetableRange = 62.5f;

	if (Instance->PresentEntities.Contains(Entity))
		DetectorTargetableRange = Instance->GetEntityData(Entity).TargetableRadius;

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			for (auto Elem : Instance->GridCells[Index].GetLDElements())
			{
				if (!Elem.IsValid()) continue;

				float Distance = (Elem->GetActorLocation() - DetectionCenter).Length() - DetectorTargetableRange;
				if (Distance > Range) continue;

				auto Direction = (Elem->GetActorLocation() - DetectionCenter).GetSafeNormal();
				auto AngleBetween = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, EntityForwardVector)));
				if (AngleBetween <= Angle) FoundLD.Add(Elem);
				
				//DebugAmalgamDetection(DetectionCenter, Range, Angle, EntityForwardVector, AngleBetween <= Angle, Elem->GetActorLocation());

				++DetectableEntities;
				if (DetectableEntities >= Instance->MaxDetectableEntities) 
					return FoundLD;
			}
		}
	}

	return FoundLD;
}

TArray<HashGridCell*> ASpatialHashGrid::FindCellsInRange(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity)
{
	TArray<HashGridCell*> FoundCells;

	const FVector DetectionCenter = WorldCoordinates;
	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	int32 DetectableEntities = 0;

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			int Index = (GridCoords.X + x) + ((GridCoords.Y + y) * Instance->GridSize.X);
			if (Index < 0 || Index >= Instance->GridCells.Num())
				continue;

			HashGridCell* ToAdd = &Instance->GridCells[Index];
			FoundCells.Add(ToAdd);
		}
	}

	return FoundCells;
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
		float NextDistance = (Next->Location - WorldCoordinates).Length() - Next->TargetableRadius;

		//if (Next->AggroCount >= GetMaxEntityAggroCount()) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Purple, TEXT("FindClosestEntity \n\t Entity skipped > Max Aggro Reached"));

		if (NextDistance > ClosestDistance || Next->Owner.Team == Team || Next->AggroCount >= GetMaxEntityAggroCount()) continue;

		Closest = Key;
		ClosestData = Next;
		ClosestDistance = NextDistance;
	}

	return Closest;
}

TWeakObjectPtr<ABuildingParent> ASpatialHashGrid::FindClosestBuilding(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team)
{
	TArray<TWeakObjectPtr<ABuildingParent>> FoundBuildings = Instance->FindBuildingsInRange(WorldCoordinates, Range, Angle, EntityForwardVector, Entity);

	if (FoundBuildings.Num() == 0) return nullptr;

	TWeakObjectPtr<ABuildingParent> Closest = nullptr;
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
TWeakObjectPtr<ALDElement> ASpatialHashGrid::FindClosestLDElement(FVector WorldCoordinates, float Range, float Angle, FVector EntityForwardVector, FMassEntityHandle Entity, ETeam Team)
{
	TArray<TWeakObjectPtr<ALDElement>> FoundElems = Instance->FindLDElementsInRange(WorldCoordinates, Range, Angle, EntityForwardVector, Entity);

	if (FoundElems.Num() == 0) return nullptr;

	float ClosestDistance = std::numeric_limits<float>::max();
	TWeakObjectPtr<ALDElement> Closest = nullptr;

	for (auto Elem : FoundElems)
	{
		if (!Elem.IsValid()) continue;
		if (!Elem->IsAttackableBy(Team)) continue;
		float NextDistance = (Elem->GetActorLocation() - WorldCoordinates).Length();
		if (NextDistance > ClosestDistance ) continue;

		Closest = Elem;
		ClosestDistance = NextDistance;
	}

	return Closest;
}

TWeakObjectPtr<ASoulBeacon> ASpatialHashGrid::IsInSoulBeaconRange(FMassEntityHandle Entity)
{
	if (!ASpatialHashGrid::Contains(Entity)) return nullptr;

	int Index = CoordsToIndex(CoordsFromHandle(Entity));
	FVector Location = Instance->GridCells[Index].Entities[Entity].Location;

	for (auto& CheckedBeacon : Instance->AllSoulBeacons)
	{
		if (!CheckedBeacon->IsValidRewarder()) continue;
		for (auto& Range : CheckedBeacon->GetEffectRanges())
		{
			//DebugDetectionRange(Range.Center, Range.Radius);
			float Distance = (Range.GetPosition() - Location).Length();
			if (Distance < Range.Radius)
			{
				return CheckedBeacon;
			}
		}
	}

	return nullptr;
}

TWeakObjectPtr<ASoulBeacon> ASpatialHashGrid::IsInSoulBeaconRangeByCell(HashGridCell* Cell)
{
	for (auto& CheckedBeacon : Instance->AllSoulBeacons)
	{
		if (CheckedBeacon->IsCellInRange(Cell)) return CheckedBeacon;
	}

	return nullptr;
}

TWeakObjectPtr<ASoulBeacon> ASpatialHashGrid::IsInSoulBeaconRange(FVector WorldCoordinates)
{
	for (auto& CheckedBeacon : Instance->AllSoulBeacons)
		for (auto& Range : CheckedBeacon->GetEffectRanges())
		{
			float Distance = (Range.GetPosition() - WorldCoordinates).Length();
			if (Distance < Range.Radius)
			{
				return CheckedBeacon;
			}
		}

	return nullptr;
}

void ASpatialHashGrid::DebugAmalgamDetection(FVector WorldCoordinates, float Range, float Angle, FVector ForwardVector, bool Detected, FVector TargetLocation)
{
	if (Instance->bDebugAmalgamDetectionCone) DebugDetectionCone(WorldCoordinates, Range, Angle, ForwardVector, Detected);
	if (Instance->bDebugAmalgamDetectionCheck) DebugDetectionCheck(WorldCoordinates, TargetLocation, Range, Angle, ForwardVector, Detected);
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

	DrawDebugLine(Instance->GetWorld(), WorldCoordinates, TargetPosition, DebugColor, false, 10.f);
}

void ASpatialHashGrid::DebugDetectionCone(FVector WorldCoordinates, float Range, float Angle, FVector Forward, bool Detected)
{
	FColor Color = Detected ? FColor::Green : FColor::Red;
	DrawDebugCone(Instance->GetWorld(), WorldCoordinates, Forward, Range, FMath::DegreesToRadians(Angle), FMath::DegreesToRadians(Angle), 10, Color, false, 1.f);
}

void ASpatialHashGrid::DebugDetectionCell(FVector WorldCoordinates, float Range)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);

	const int DetectionRangeX = Range / Instance->CellSize.X;
	const int DetectionRangeY = Range / Instance->CellSize.Y;

	for (int x = -DetectionRangeX; x <= DetectionRangeX; ++x)
	{
		for (int y = -DetectionRangeY; y <= DetectionRangeY; ++y)
		{
			float GridXPos = Instance->GetActorLocation().X + (GridCoords.X + x) * Instance->CellSize.X;
			float GridYPos = Instance->GetActorLocation().Y + (GridCoords.Y + y) * Instance->CellSize.Y;

			FVector TL = FVector(GridXPos, GridYPos, 0.f);
			FVector TR = FVector(GridXPos + Instance->CellSize.X, GridYPos, 0.f);
			FVector BL = FVector(GridXPos, GridYPos + Instance->CellSize.Y, 0.f);
			FVector BR = FVector(GridXPos + Instance->CellSize.X, GridYPos + Instance->CellSize.Y, 0.f);

			DrawDebugLine(Instance->GetWorld(), TL, TR, FColor::Orange, false, 1.0f);
			DrawDebugLine(Instance->GetWorld(), TL, BL, FColor::Orange, false, 1.0f);
			DrawDebugLine(Instance->GetWorld(), BL, BR, FColor::Orange, false, 1.0f);
			DrawDebugLine(Instance->GetWorld(), TR, BR, FColor::Orange, false, 1.0f);
		}
	}
}

void ASpatialHashGrid::DebugSingleDetectionCell(FVector WorldCoordinates, int XOffset, int YOffset, FColor CellColor)
{
	FIntVector2 GridCoords = Instance->WorldToGridCoords(WorldCoordinates);
	
	float GridXPos = Instance->GetActorLocation().X + (GridCoords.X + XOffset) * Instance->CellSize.X;
	float GridYPos = Instance->GetActorLocation().Y + (GridCoords.Y + YOffset) * Instance->CellSize.Y;

	FVector TL = FVector(GridXPos, GridYPos, 0.f);
	FVector TR = FVector(GridXPos + Instance->CellSize.X, GridYPos, 0.f);
	FVector BL = FVector(GridXPos, GridYPos + Instance->CellSize.Y, 0.f);
	FVector BR = FVector(GridXPos + Instance->CellSize.X, GridYPos + Instance->CellSize.Y, 0.f);

	DrawDebugLine(Instance->GetWorld(), TL, TR, CellColor, false, 2.5f, 10U);
	DrawDebugLine(Instance->GetWorld(), TL, BL, CellColor, false, 2.5f, 10U);
	DrawDebugLine(Instance->GetWorld(), BL, BR, CellColor, false, 2.5f, 10U);
	DrawDebugLine(Instance->GetWorld(), TR, BR, CellColor, false, 2.5f, 10U);
}



bool ASpatialHashGrid::DebugCheckExpr(bool Expr, const char* Msg, bool bUseAsserts)
{
	if (Instance->bUseAsserts)
	{
		checkf(Expr, TEXT("checkf triggered."));
		return true;
	}

	return Expr;
}

void ASpatialHashGrid::DebugAllBuildingRanges()
{
	for (auto& Cell : Instance->GridCells)
	{
		for (auto& Building : Cell.Buildings)
		{
			DrawDebugSphere(Instance->GetWorld(), Building->GetActorLocation(), Building->GetTargetableRange(), 10, FColor::Orange, false, 10.f, 10U);
		}
	}
}

void ASpatialHashGrid::DebugAllLDRanges()
{
	for (auto& Cell : Instance->GridCells)
	{
		for (auto& Elem : Cell.LDElements)
		{
			DrawDebugSphere(Instance->GetWorld(), Elem->GetActorLocation(), 1500.f /*Placeholder value*/, 10, FColor::Orange, false, 10.f, 10U);
		}
	}
}

void ASpatialHashGrid::DebugGridContent()
{
	int LDCount = 0;
	int BuildingCount = 0;
	for (auto& Cell : Instance->GridCells)
	{
		LDCount += Cell.GetLDElementsNum();
		BuildingCount += Cell.GetBuildingsNum();
	}

	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, FString::Printf(TEXT("%d LD Elements \n %d Buildings"), LDCount, BuildingCount));
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
	Instance->GridLocation = Instance->GetActorLocation();
	CellSize = FIntVector2(CellSizeX, CellSizeY);
	GridSize = FIntVector2(GridSizeX, GridSizeY);
	UE_LOG(LogTemp, Warning, TEXT("Grid Size : %d ; %d"), GridSizeX, GridSizeY);
	UE_LOG(LogTemp, Warning, TEXT("Cell Size : %f ; %f"), CellSizeX, CellSizeY);
	//GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Grid Size : %d ; %d"), GridSize.X, GridSize.Y));
	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("yellow hello"));

	Generate();
}

void ASpatialHashGrid::DebugGridSize()
{
	UE_LOG(LogTemp, Warning, TEXT("Grid Size : %d ; %d"), GridSizeX, GridSizeY);
	UE_LOG(LogTemp, Warning, TEXT("Cell Size : %f ; %f"), CellSizeX, CellSizeY);
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Grid Size : %d ; %d"), GridSizeX, GridSizeY));
	GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow, FString::Printf(TEXT("Cell Grid Size : %d ; %d"), CellSizeX, CellSizeY));
}

void ASpatialHashGrid::DebugGridVisuals()
{
	//Debug draw line
	FVector GridBase = Instance->GetActorLocation();
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
	//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("Damage inflicted : %f \n\t New Health : %f"), Damage, EntityHealth));
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

TArray<TWeakObjectPtr<ABuildingParent>> HashGridCell::GetBuildings()
{
	TArray<TWeakObjectPtr<ABuildingParent>> OutBuildings = Buildings;
	return OutBuildings;
}

TArray<TWeakObjectPtr<ALDElement>> HashGridCell::GetLDElements()
{
	TArray<TWeakObjectPtr<ALDElement>> OutLDElems = LDElements;
	return OutLDElems;
}

TArray<FOwner> HashGridCell::GetPresentOwners()
{
	return PresentOwners;
}

void HashGridCell::UpdatePresentOwners()
{
	PresentOwners.Empty();

	for (auto& Entity : GetEntities())
	{
		if (!PresentOwners.Contains(Entity.Owner))
		{
			PresentOwners.AddUnique(Entity.Owner);
		}
	}

	if(LinkedBeacon.IsValid())
		LinkedBeacon->CheckCells();
}

bool HashGridCell::Contains(FMassEntityHandle Entity)
{
	return Entities.Contains(Entity);
}

bool HashGridCell::Contains(TWeakObjectPtr<ABuildingParent> Building)
{
	return Buildings.Contains(Building);
}

bool HashGridCell::Contains(TWeakObjectPtr<ALDElement> LDElement)
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

	int NumBuildings = Buildings.FilterByPredicate([&Team](TWeakObjectPtr<ABuildingParent> Building)
		{
			return Building->GetOwner().Team == Team;
		}).Num();

	return NumEntities + NumBuildings;
}

int HashGridCell::GetTotalNumByTeamDifference(FOwner Owner)
{
	return GetTotalNum() - GetTotalNumByTeam(Owner);
}