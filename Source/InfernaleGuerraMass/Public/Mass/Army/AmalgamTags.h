// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"

#include "AmalgamTags.generated.h"

/**
 * 
 */

USTRUCT()
struct FAmalgamInitializeTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FAmalgamClientInitializeTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FAmalgamClientExecuteTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FAmalgamAggroTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FAmalgamFightTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FAmalgamMoveTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FAmalgamInactiveTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FAmalgamStateChangeTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FAmalgamKillTag : public FMassTag
{
	GENERATED_BODY()
};