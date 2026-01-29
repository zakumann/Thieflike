// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "Baton.generated.h"

class UCapsuleComponent;

/**
 * Baton - A melee weapon for knockouts and non-lethal takedowns
 * Features:
 * - Swing attack for frontal hits
 * - Stealth knockout from behind
 * - Charged attack for extra damage
 */
UCLASS()
class THIEFLIKE_API ABaton : public AWeapon
{
	GENERATED_BODY()
	
public:
	ABaton();

protected:
	virtual void BeginPlay() override;

public:
	// --- Override Attack Functions ---
	virtual void StartAttack() override;
	virtual void PerformHitDetection() override;

	// --- Baton-Specific Attacks ---

	// Start a steatlh kockout attack
	UFUNCTION(BlueprintCallable, Category = "Baton")
	void StartStealthKnockout();

	// Start charging a heavy attack
	UFUNCTION(BlueprintCallable, Category = "Baton")
	void StartChargedAttack();

	// Release charged attack
	UFUNCTION(BlueprintCallable, Category = "Baton")
	void ReleaseChargedAttack();

protected:
	// ---- Hit Detection Components ----

// Capsule for detecting hits along the baton's length
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baton|Components")
	TObjectPtr<UCapsuleComponent> HitCapsule;

	// ---- Baton Stats ----

	// Damage dealt by regular swing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Baton|Stats")
	float SwingDamage = 25.0f;

	// Damage dealt by charged attack
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Baton|Stats")
	float ChargedDamage = 60.0f;

	// Instant knockout damage (from behind)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Baton|Stats")
	float KnockoutDamage = 100.0f;

	// Maximum charge time for charged attack
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Baton|Stats")
	float MaxChargeTime = 1.5f;

	// Angle in degrees to consider "behind" for stealth knockout
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Baton|Stats")
	float StealthKnockoutAngle = 45.0f;

	// ---- Attack State ----

	bool bIsChargingAttack = false;
	float CurrentChargeTime = 0.0f;

	FTimerHandle ChargeTimerHandle;

	// ---- Helper Functions ----

	// Check if target is facing away (for stealth knockout)
	bool IsTargetFacingAway(AActor* Target) const;

	// Apply damage to target
	void ApplyDamageToTarget(AActor* Target, float Damage, bool bIsKnockout = false);

	// Visual/audio feedback for charged attack
	void UpdateChargeFeedback(float ChargePercent);

	// ---- Collision Callbacks ----

	UFUNCTION()
	void OnHitCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
