// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GenericTeamAgentInterface.h"
#include "EnemyAIController.generated.h"

/**
 * AI Controller that implements Team Agent Interface so Perception knows
 * who is hostile and who is friendly.
 */
UCLASS()
class THIEFLIKE_API AEnemyAIController : public AAIController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
public:
	AEnemyAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	// --- Blackboard Key Names (must match Blackboard asset exactly!) ---
	static inline const FName BB_TargetActor = FName("TargetActor");
	static inline const FName BB_LastKnownLocation = FName("LastKnownLocation");
	static inline const FName BB_DistanceToTarget = FName("DistanceToTarget");
	static inline const FName BB_IsStunned = FName("IsStunned");
	static inline const FName BB_CanSeePlayer = FName("CanSeePlayer");

	// --- Blackboard Update Functions ---
	UFUNCTION(BlueprintCallable, Category = "AI")
	void UpdateBlackboard_TargetActor(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void UpdateBlackboard_LastKnownLocation(FVector Location);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void UpdateBlackboard_Distance(float Distance);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void UpdateBlackboard_CanSeePlayer(bool bCanSee);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void UpdateBlackboard_IsStunned(bool bStunned);

	// --- Team Interface Implementation ---
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

private:
	// Team ID (1 = Enemy, 0 = Player)
	FGenericTeamId TeamId;

protected:
	// Reference to controlled enemy
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	class AEnemyCharacterBase* ControlledEnemy;
};
