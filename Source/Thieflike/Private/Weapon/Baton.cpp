// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Baton.h"
#include "Character/PlayerCharacter.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ABaton::ABaton()
{
	// Create hit detection capsule
	HitCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitCapsule"));
	HitCapsule->SetupAttachment(WeaponMesh);
	HitCapsule->SetCapsuleSize(5.0f, 25.0f); // Thin capsule along baton length

	// Disable collision by default (enable during attack)
	HitCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Set baton-specific properties
	BaseDamage = SwingDamage;
	AttackRange = 120.0f; // Slightly longer than base weapon
	AttackCooldown = 0.6f;
	AttackDuration = 0.4f;
}

void ABaton::BeginPlay()
{
	Super::BeginPlay();

	// Bind overlap event
	if (HitCapsule)
	{
		HitCapsule->OnComponentBeginOverlap.AddDynamic(this, &ABaton::OnHitCapsuleBeginOverlap);
	}
}

void ABaton::StartAttack()
{
	// If charging, this is a different attack
	if (bIsChargingAttack)
	{
		ReleaseChargedAttack();
		return;
	}

	// Normal attack
	Super::StartAttack();

	// Enable hit detection during swing
	if (HitCapsule)
	{
		HitCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// Disable after attack duration
		FTimerHandle DisableCollisionHandle;
		GetWorld()->GetTimerManager().SetTimer(DisableCollisionHandle, [this]()
			{
				if (HitCapsule)
				{
					HitCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
			}, AttackDuration, false);
	}
}

void ABaton::PerformHitDetection()
{
	// For baton, we use capsule overlap instead of line trace
	// But we keep this for compatibility

	if (!OwningPlayer) return;

	// Perform a sphere sweep for more reliable hit detection
	FVector Start = OwningPlayer->GetActorLocation();
	FVector Forward = OwningPlayer->GetActorForwardVector();
	FVector End = Start + (Forward * AttackRange);

	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwningPlayer);

	// Sphere sweep for wider hit area
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(30.0f),
		Params
	);

	if (bHit)
	{
		// Process all hit actors
		for (const FHitResult& Hit : HitResults)
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				// Check if it's a stealth knockout (from behind)
				if (IsTargetFacingAway(HitActor))
				{
					ApplyDamageToTarget(HitActor, KnockoutDamage, true);
					UE_LOG(LogTemp, Warning, TEXT("STEALTH KNOCKOUT!"));
				}
				else
				{
					ApplyDamageToTarget(HitActor, SwingDamage, false);
				}

				// Only hit one target per swing
				break;
			}
		}
	}
}

void ABaton::StartStealthKnockout()
{
	// This is called when player is behind enemy and presses special takedown button
	if (!CanAttack()) return;

	if (!OwningPlayer) return;

	// Check for enemy directly in front
	FVector Start = OwningPlayer->GetActorLocation();
	FVector Forward = OwningPlayer->GetActorForwardVector();
	FVector End = Start + (Forward * 150.0f); // Slightly longer range for takedown

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwningPlayer);

	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(50.0f),
		Params
	);

	if (bHit && HitResult.GetActor())
	{
		AActor* Target = HitResult.GetActor();

		// Verify target is facing away
		if (IsTargetFacingAway(Target))
		{
			// Play special knockout animation/sound here
			UE_LOG(LogTemp, Warning, TEXT("Stealth Takedown Executed!"));

			ApplyDamageToTarget(Target, KnockoutDamage, true);

			// Start attack animation (different from normal swing)
			SetWeaponState(EWeaponState::Attacking);

			FTimerHandle TakedownTimerHandle;
			GetWorld()->GetTimerManager().SetTimer(TakedownTimerHandle, [this]()
				{
					SetWeaponState(EWeaponState::Equipped);
				}, 1.0f, false); // Longer animation for takedown
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Target is facing you - cannot perform stealth takedown!"));
		}
	}
}

void ABaton::StartChargedAttack()
{
	if (!CanAttack()) return;

	bIsChargingAttack = true;
	CurrentChargeTime = 0.0f;

	// Increment charge over time
	GetWorld()->GetTimerManager().SetTimer(ChargeTimerHandle, [this]()
		{
			CurrentChargeTime += 0.1f;

			float ChargePercent = FMath::Clamp(CurrentChargeTime / MaxChargeTime, 0.0f, 1.0f);
			UpdateChargeFeedback(ChargePercent);

			// Auto-release at max charge
			if (CurrentChargeTime >= MaxChargeTime)
			{
				ReleaseChargedAttack();
			}

		}, 0.1f, true);
}

void ABaton::ReleaseChargedAttack()
{
	if (!bIsChargingAttack) return;

	GetWorld()->GetTimerManager().ClearTimer(ChargeTimerHandle);
	bIsChargingAttack = false;

	// Calculate damage based on charge time
	float ChargePercent = FMath::Clamp(CurrentChargeTime / MaxChargeTime, 0.0f, 1.0f);
	float FinalDamage = FMath::Lerp(SwingDamage, ChargedDamage, ChargePercent);

	UE_LOG(LogTemp, Log, TEXT("Charged Attack Released! Charge: %.2f%% Damage: %.1f"), ChargePercent * 100.0f, FinalDamage);

	// Perform attack with charged damage
	BaseDamage = FinalDamage; // Temporarily override damage
	StartAttack();

	// Reset damage after attack
	FTimerHandle ResetDamageHandle;
	GetWorld()->GetTimerManager().SetTimer(ResetDamageHandle, [this]()
		{
			BaseDamage = SwingDamage;
		}, AttackDuration, false);
}

bool ABaton::IsTargetFacingAway(AActor* Target) const
{
	if (!Target || !OwningPlayer) return false;

	// Get vector from target to player
	FVector ToPlayer = (OwningPlayer->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();

	// Get target's forward vector
	FVector TargetForward = Target->GetActorForwardVector();

	// Calculate dot product (1.0 = same direction, -1.0 = opposite)
	float DotProduct = FVector::DotProduct(TargetForward, ToPlayer);

	// Convert angle to degrees
	float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

	// If angle is greater than 135 degrees (target facing away)
	// Or more lenient: 180 - StealthKnockoutAngle
	return AngleDegrees > (180.0f - StealthKnockoutAngle);
}

void ABaton::ApplyDamageToTarget(AActor* Target, float Damage, bool bIsKnockout)
{
	if (!Target) return;

	// Play hit sound
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, Target->GetActorLocation());
	}

	// Apply damage using UE's damage system
	UGameplayStatics::ApplyDamage(
		Target,
		Damage,
		OwningPlayer ? OwningPlayer->GetController() : nullptr,
		this,
		UDamageType::StaticClass()
	);

	// Special handling for enemies
	if (AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Target))
	{
		if (bIsKnockout)
		{
			// Knockout logic - you might have a "Stunned" state in your enemy
			UE_LOG(LogTemp, Warning, TEXT("Enemy knocked out!"));
			// Enemy->SetEnemyState(EEnemyState::Stunned); // If you add this state
		}
	}

	// Visual feedback - spawn particle effect, camera shake, etc.
	// TODO: Add particle effects and screen shake
}

void ABaton::UpdateChargeFeedback(float ChargePercent)
{
	// Visual feedback during charging
	// You can:
	// - Scale the weapon mesh
	// - Play charging sound (pitch increases with charge)
	// - Spawn particle effects
	// - Screen shake

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			0.1f,
			FColor::Yellow,
			FString::Printf(TEXT("Charging: %.0f%%"), ChargePercent * 100.0f)
		);
	}
}

void ABaton::OnHitCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only process hits during attack
	if (CurrentState != EWeaponState::Attacking) return;

	// Ignore owner
	if (OtherActor == OwningPlayer || OtherActor == this) return;

	// Check if target is facing away for potential knockout
	if (IsTargetFacingAway(OtherActor))
	{
		ApplyDamageToTarget(OtherActor, KnockoutDamage, true);
	}
	else
	{
		ApplyDamageToTarget(OtherActor, BaseDamage, false);
	}

	// Disable collision after first hit to prevent multiple hits
	if (HitCapsule)
	{
		HitCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
