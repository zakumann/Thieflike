// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class USkeletalMeshComponent;
class APlayerCharacter;

// Weapon states for animation/logic
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	Equipping   UMETA(DisplayName = "Equipping"),
	Equipped    UMETA(DisplayName = "Equipped"),
	Attacking   UMETA(DisplayName = "Attacking"),
	Unequipping UMETA(DisplayName = "Unequipping")
};

UCLASS()
class THIEFLIKE_API AWeapon : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	
public:	
	// Sets default values for this actor's properties
	AWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// ---- Core Weapon Interface ----

	// Called when player equips this weapon
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void OnEquip(APlayerCharacter* NewOwner);

	// Called when player unequips this weapon
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void OnUnequip();

	// Called when player presses attack button
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartAttack();

	// Called when attack animation/timer completes
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void EndAttack();

	// Called during attack to check for hits
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void PerformHitDetection();

	// ---- Getters ----

	UFUNCTION(BlueprintPure, Category = "Weapon")
	EWeaponState GetWeaponState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanAttack() const { return CurrentState == EWeaponState::Equipped; }

protected:
	// ---- State Management ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|State")
	EWeaponState CurrentState;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	TObjectPtr<APlayerCharacter> OwningPlayer;

	// ---- Weapon Properties ----

	// Base damage this weapon deals
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float BaseDamage = 30.0f;

	// Attack range in centimeters
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float AttackRange = 100.0f;

	// Time between attacks (cooldown)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float AttackCooldown = 0.5f;

	// How long the weapon stays in "attacking" state
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float AttackDuration = 0.3f;

	// Sound effect for attack swing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TObjectPtr<USoundBase> SwingSound;

	// Sound effect for successful hit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TObjectPtr<USoundBase> HitSound;

	// ---- Socket Names ----

	// Socket name on player mesh to attach weapon when equipped
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Sockets")
	FName EquippedSocketName = TEXT("WeaponSocket");

	// Socket name on player mesh to attach weapon when holstered
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Sockets")
	FName HolsteredSocketName = TEXT("HolsterSocket");

	// ---- Internal Timers ----

	FTimerHandle AttackTimerHandle;
	FTimerHandle CooldownTimerHandle;

	// Flag to prevent spam clicking
	bool bCanAttack = true;

	// Helper function to set weapon state
	void SetWeaponState(EWeaponState NewState);
};
