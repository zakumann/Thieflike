// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Ladder.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Character/PlayerCharacter.h"

// Sets default values
ALadder::ALadder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = DefaultSceneRoot;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Collision"));
	BoxCollision->SetupAttachment(GetRootComponent());
	BoxCollision->SetCollisionProfileName(TEXT("Trigger"));

	LadderBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LadderBase"));
	LadderBase->SetupAttachment(GetRootComponent());

	MiddleParts = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Middles"));
	MiddleParts->SetupAttachment(LadderBase);
}

void ALadder::BeginPlay()
{
	Super::BeginPlay();

	if (BoxCollision)
	{
		BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &ALadder::OnOverlapBegin);
		BoxCollision->OnComponentEndOverlap.AddDynamic(this, &ALadder::OnOverlapEnd);
	}
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
		// GetBoundingBox().GetSize() is total Height
		FVector TopMeshSize = TopLadderMesh->GetBoundingBox().GetSize();
		FVector MidMeshSize = MidLadderMesh->GetBoundingBox().GetSize();

		// Cache mesh sizes
		float TopHeight = TopMeshSize.Z;
		float MidHeight = MidMeshSize.Z;

		for (int i = 0; i <= MidPartCount; i++)
		{
			float ZLocation = -TopHeight - (MidHeight * i);

			FTransform InstanceTransform(FVector(0.0f, 0.0f, ZLocation));
			MiddleParts->AddInstance(InstanceTransform);
		}
		// --- Setup Box Collision ---
		FVector BoxExtent;
		// Z side: Ladder height match with size of mesh
		const float TotalHeight = TopHeight + (MidHeight * MidPartCount);
		// Add offset to reach slightly above the top of ladder
		BoxExtent.Z = (TotalHeight * 0.5f) + TopOffset;
		//Y side : Ladder vertical match with size of mesh
		BoxExtent.Y = TopMeshSize.Y * 0.5f;
		//X side: Thickness
		BoxExtent.X = CollisionThickness;

		// --- Set location ---
		FVector BoxLocation = FVector::ZeroVector;

		// Z location: center point
		BoxLocation.Z = -BoxExtent.Z + TopOffset;

		// Y location: center
		BoxLocation.Y = 0.f;
		// X location: in front of ladder mesh
		float MeshHalfDepth = TopMeshSize.X * 0.5f;
		BoxLocation.X = MeshHalfDepth + BoxExtent.X + CollisionForwardOffset;
		// Set Box Collision extent & location
		BoxCollision->SetBoxExtent(BoxExtent, true);
		BoxCollision->SetRelativeLocation(BoxLocation);

	}
}
// Overlap Events
void ALadder::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor))
	{
		Player->SetLadderMode(true, this);
	}
}
// End Overlap Events
void ALadder::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor))
	{
		Player->SetLadderMode(false, nullptr);
	}
}


