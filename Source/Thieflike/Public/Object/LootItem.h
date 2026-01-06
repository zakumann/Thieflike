// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootItem.generated.h"

UCLASS()
class THIEFLIKE_API ALootItem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALootItem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// The visual representation of the loot
	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Components")
	UStaticMeshComponent* LootMesh;

	// How much money this item is worth
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Config")
	int32 LootValue = 50;

	// Function called by PlayerCharacter when interacting
	// We pass the Player pointer so the item knows who to give money to
	void OnInteract(class APlayerCharacter* Player);

};
