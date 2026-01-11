// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Ladder.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

// Sets default values
ALadder::ALadder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = DefaultSceneRoot;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Collision"));
	BoxCollision->SetupAttachment(GetRootComponent());
	BoxCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	LadderBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LadderBase"));
	LadderBase->SetupAttachment(GetRootComponent());


	MiddleParts = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Middles"));
	MiddleParts->SetupAttachment(LadderBase);
}

float ALadder::GetLadderMinZ() const
{
	if (!BoxCollision)
	{
		return GetActorLocation().Z;
	}

	FVector BoxLocation = BoxCollision->GetComponentLocation();
	FVector BoxExtent = BoxCollision->GetScaledBoxExtent();

	return BoxLocation.Z - BoxExtent.Z;
}

float ALadder::GetLadderMaxZ() const
{
	if (!BoxCollision)
	{
		return GetActorLocation().Z;
	}

	FVector BoxLocation = BoxCollision->GetComponentLocation();
	FVector BoxExtent = BoxCollision->GetScaledBoxExtent();

	return BoxLocation.Z + BoxExtent.Z;
}

void ALadder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (TopLadderMesh != nullptr && MidLadderMesh != nullptr)
	{
		if (LadderBase != nullptr)
		{
			LadderBase->SetStaticMesh(TopLadderMesh);
		}

		if (MidLadderMesh != nullptr)
		{
			MiddleParts->SetStaticMesh(MidLadderMesh);
		}

		MiddleParts->ClearInstances();

		// Cache mesh sizes
		float TopHeight = TopLadderMesh->GetBoundingBox().GetSize().Z;
		float MidHeight = MidLadderMesh->GetBoundingBox().GetSize().Z;

		for (int i = 0; i < MidPartCount; i++)
		{
			float ZLocation = -TopHeight - (MidHeight * i);

			FTransform InstanceTransform(FVector(0.0f, 0.0f, ZLocation));
			MiddleParts->AddInstance(InstanceTransform);
		}

		FVector BoxExtent;
		BoxExtent.X = TopLadderMesh->GetBoundingBox().GetSize().X / 2;
		BoxExtent.Y = TopLadderMesh->GetBoundingBox().GetSize().Y / 2;

		FVector BoxLocation = FVector::ZeroVector;
		BoxLocation.X = 0.f;
		BoxLocation.Y = 0.f;

		const float TotalHeight = TopHeight + (MidHeight * MidPartCount);

		BoxExtent.Z = (TotalHeight * 0.5f) + TopOffset;
		BoxLocation.Z = -BoxExtent.Z + TopOffset;


		BoxCollision->SetBoxExtent(BoxExtent, true);
		BoxCollision->SetRelativeLocation(BoxLocation);
	}
}


