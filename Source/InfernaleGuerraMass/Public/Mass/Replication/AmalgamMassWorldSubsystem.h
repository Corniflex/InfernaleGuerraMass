// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AmalgamMassWorldSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class INFERNALETESTING_API UAmalgamMassWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void PostInitialize() override;
	
};
