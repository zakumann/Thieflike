// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyCharacterBase.generated.h"

/**
 * Base class for all enemy characters
 * Handles AI perception, health, and behavior tree integration
 */
UCLASS()
class THIEFLIKE_API AEnemyCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacterBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ========== AI COMPONENTS ==========

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAISenseConfig_Sight* SightConfig;

	// ========== BEHAVIOR TREE ==========

	// Assign this in Blueprint! (e.g., BP_Enemy)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Behavior Tree")
	class UBehaviorTree* EnemyBehaviorTree;

	// ========== AI STATS ==========

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Vision")
	float BaseSightRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Vision")
	float LoseSightRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Vision")
	float PeripheralVisionAngle = 60.0f;

	// How close player must be to see in total darkness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stealth")
	float DarkVisionRange = 200.0f;

	// ========== HEALTH SYSTEM ==========

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Health")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Health")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Health")
	float StunDuration = 5.0f;

	// Damage handling
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	// Apply stun (called by weapon system)
	UFUNCTION(BlueprintCallable, Category = "AI|Health")
	void ApplyStun(float Duration);

protected:
	// ========== AI CONTROLLER REFERENCE ==========

	UPROPERTY()
	class AEnemyAIController* AIController;

	// ========== PERCEPTION CALLBACKS ==========

	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

	// Check if player is visible despite stealth
	bool CanSeePlayerDespiteStealth(class APlayerCharacter* Player);

	// ========== HEALTH/STUN SYSTEM ==========

	FTimerHandle StunTimerHandle;
	void OnStunEnd();
	void Die();
};