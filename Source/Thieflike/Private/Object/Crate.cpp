// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Crate.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"

// Sets default values
ACrate::ACrate()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create Base
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	// Create Lid and attach to Base
	LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
	LidMesh->SetupAttachment(BaseMesh);

	bIsOpen = false;

}

void ACrate::OnInteract()
{
	// Toggle State
	bIsOpen = !bIsOpen;

	if (bIsOpen)
	{
		OnOpen();
	}
	else
	{
		OnClose();
	}
}

// Called when the game starts or when spawned
void ACrate::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACrate::OnOpen_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Crate Opened"));
}

void ACrate::OnClose_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Crate Closed"));
}

// Called every frame
void ACrate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

