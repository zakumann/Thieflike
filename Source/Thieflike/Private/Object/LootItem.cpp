// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/LootItem.h"
#include "Character/PlayerCharacter.h"

// Sets default values
ALootItem::ALootItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	LootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LootMesh"));
	RootComponent = LootMesh;

	// Optional: Enable physis so it falls to the ground
	LootMesh->SetSimulatePhysics(true);
	LootMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
}

// Called when the game starts or when spawned
void ALootItem::BeginPlay()
{
	Super::BeginPlay();
	
}

void ALootItem::OnInteract(APlayerCharacter* Player)
{
	if (Player)
	{
		// 1. Give Money to Player
		Player->AddMoney(LootValue);

		// 2. Play Sound (Optional - add logic here or in BP

		// 3. Destroy this actor
		Destroy();
	}
}

