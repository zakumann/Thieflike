// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"
#include "Character/PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	// Disable collision on the mesh by default (will enable during attack)
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CurrentState = EWeaponState::Idle;
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::OnEquip(APlayerCharacter* NewOwner)
{
	if (!NewOwner) return; // Safety check

	OwningPlayer = NewOwner;
	SetWeaponState(EWeaponState::Equipping);

	// Attach weapon to player's equipped socket
	if (USkeletalMeshComponent* PlayerMesh = OwningPlayer->GetMesh())
	{
		AttachToComponent(PlayerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, EquippedSocketName);
	}
	else if (USkeletalMeshComponent* FPMesh = OwningPlayer->FindComponentByClass<USkeletalMeshComponent>())
	{
		// Fallback to first person mesh if it exists
		AttachToComponent(FPMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, EquippedSocketName);
	}

	// small delay to simulate equipping animation
	FTimerHandle EquipTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(EquipTimerHandle, [this]()
	{
		SetWeaponState(EWeaponState::Equipped);
	}, 0.3f, false);
}

void AWeapon::OnUnequip()
{
	SetWeaponState(EWeaponState::Unequipping);

	// Clear any active timers
	GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);

	// Detach from player
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// small delay, then set to idle
	FTimerHandle UnequipTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(UnequipTimerHandle, [this]()
	{
		SetWeaponState(EWeaponState::Idle);
		OwningPlayer = nullptr;
	}, 0.3f, false);
}

void AWeapon::StartAttack()
{
	// Check if we can attack
	if (!CanAttack() || !bCanAttack)
	{
		return;
	}

	// Set state to attacking
	SetWeaponState(EWeaponState::Attacking);
	bCanAttack = false;

	// Play swing sound
	if (SwingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SwingSound, GetActorLocation());
	}

	// Perform hit detection after a slight delay (mid-swing)
	FTimerHandle HitDetectionTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(HitDetectionTimerHandle, this, &AWeapon::PerformHitDetection, AttackDuration * 0.5f, false);

	// End attack after duration
	GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &AWeapon::EndAttack, AttackDuration, false);
}

void AWeapon::EndAttack()
{
	// Return to equipped state
	SetWeaponState(EWeaponState::Equipped);

	// Start cooldown timer
	GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, [this]()
		{
			bCanAttack = true;
		}, AttackCooldown, false);
}

void AWeapon::PerformHitDetection()
{
	// Base implementation - override in child classes for specific hit detection
	// This is a simple forward trace

	if (!OwningPlayer) return;

	FVector Start = OwningPlayer->GetActorLocation();
	FVector Forward = OwningPlayer->GetActorForwardVector();
	FVector End = Start + (Forward * AttackRange);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwningPlayer);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, Params);

	if (bHit && HitResult.GetActor())
	{
		// Play hit sound
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, HitSound, HitResult.ImpactPoint);
		}

		// Apply damage (UGameplayStatics::ApplyDamage or custom damage system)
		UE_LOG(LogTemp, Log, TEXT("Weapon hit: %s"), *HitResult.GetActor()->GetName());

		// Child classes should override this to implement specific damage logic
	}
}

void AWeapon::SetWeaponState(EWeaponState NewState)
{
	if (CurrentState == NewState) return;

	EWeaponState OldState = CurrentState;
	CurrentState = NewState;

	// You can add state transition logic here
	UE_LOG(LogTemp, Log, TEXT("Weapon state changed: %d -> %d"), (int32)OldState, (int32)NewState);
}

