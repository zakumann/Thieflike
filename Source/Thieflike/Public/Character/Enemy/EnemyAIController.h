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
class THIEFLIKE_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
public:
	AEnemyAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	// --- Movement Helpers ---
	void MoveToTargetActor(AActor* TargetActor, float AcceptanceRadius = 100.0f);
	void MoveToTargetLocation(const FVector& Location, float AcceptanceRadius = 50.f);
	void StopMovementSafe();

	// --- Focus Helpers ---
	void SetFocusOnActor(AActor* FocusActor);
	void ClearFocusSafe();

	// --- IGenericTeamAgentInterface Implementation ---
	// This allows the AI Perception system to ask "What team are you on?"
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

private:
	// --- Team ID ---
	FGenericTeamId TeamId;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	class AEnemyCharacterBase* ControlledEnemy;
};
