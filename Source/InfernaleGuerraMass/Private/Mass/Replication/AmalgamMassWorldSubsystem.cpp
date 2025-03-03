// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/AmalgamMassWorldSubsystem.h"

#include "MassReplicationSubsystem.h"
#include "Mass/Replication/AmalgamMassBubbleInfoClient.h"

void UAmalgamMassWorldSubsystem::PostInitialize()
{
	Super::PostInitialize();

	UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(GetWorld());
	check(ReplicationSubsystem);
	ReplicationSubsystem->RegisterBubbleInfoClass(AAmalgamMassClientBubbleInfo::StaticClass());
}
