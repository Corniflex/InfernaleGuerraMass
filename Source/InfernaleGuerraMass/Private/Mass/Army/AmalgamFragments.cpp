
#include "Mass/Army/AmalgamFragments.h"

#include "Mass/Collision/SpatialHashGrid.h"
#include "LD/Buildings/BuildingParent.h"

float FAmalgamTargetFragment::GetTargetRangeOffset(EAmalgamAggro AggroType) const
{
	switch (AggroType)
	{
	case NoAggro:
		return 0.f;
	case Amalgam:
		return ASpatialHashGrid::GetEntityData(TargetEntity).TargetableRadius;
	case Building:
		if (TargetBuilding.IsValid())
			return TargetBuilding->GetTargetableRange();
		else
			return 0.0f;
	case LDElement:
		return 0.f;
	}

	return 0.f;
}