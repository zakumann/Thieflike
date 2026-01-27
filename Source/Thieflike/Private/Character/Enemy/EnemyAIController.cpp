// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyAIController.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "GameFramework/Actor.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

AEnemyAIController::AEnemyAIController()
{
	// Set this agent's team to Team 1 (The Enemy Team)
	// Usually: 0 = No Team, 1 = Red, 2 = Blue, etc.
	TeamId = FGenericTeamId(1);
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledEnemy = Cast<AEnemyCharacterBase>(InPawn);

	if (!ControlledEnemy)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyAIController possessed non-enemy pawn!"));
		return;
	}

	// Optional: If you add a BehaviorTree to your EnemyCharacterBase later, run it here.
	// if (ControlledEnemy->BehaviorTreeAsset)
	// {
	//     RunBehaviorTree(ControlledEnemy->BehaviorTreeAsset);
	// }
}

void AEnemyAIController::OnUnPossess()
{
	Super::OnUnPossess();
	ControlledEnemy = nullptr;
}

void AEnemyAIController::MoveToTargetActor(AActor* TargetActor, float AcceptanceRadius)
{
	if (!TargetActor) return;

	FAIMoveRequest MoveReq;
	MoveReq.SetGoalActor(TargetActor);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);
	MoveReq.SetUsePathfinding(true);

	// Critical for gameplay: If true, AI will move as close as possible if goal is unreachable.
	// If false, AI aborts immediately if the goal is off-navmesh.
	MoveReq.SetAllowPartialPath(true);

	MoveTo(MoveReq);
}

void AEnemyAIController::MoveToTargetLocation(const FVector& Location, float AcceptanceRadius)
{
	FAIMoveRequest MoveReq;
	MoveReq.SetGoalLocation(Location);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);

	MoveTo(MoveReq);
}

void AEnemyAIController::StopMovementSafe()
{
	StopMovement();
	ClearFocusSafe();
}

void AEnemyAIController::SetFocusOnActor(AActor* FocusActor)
{
	if (!FocusActor) return;
	// Gameplay priority ensures this override lower priority look requests
	SetFocus(FocusActor, EAIFocusPriority::Gameplay);
}

void AEnemyAIController::ClearFocusSafe()
{
	ClearFocus(EAIFocusPriority::Gameplay);
}

// Team Interface Logic
FGenericTeamId AEnemyAIController::GetGenericTeamId() const
{
	return TeamId;
}

ETeamAttitude::Type AEnemyAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	// 1. Check if the other actor implements the interface
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);

	if (OtherTeamAgent)
	{
		FGenericTeamId OtherTeamId = OtherTeamAgent->GetGenericTeamId();

		// If Team IDs match, they are friendly.
		if (OtherTeamId == TeamId)
		{
			return ETeamAttitude::Friendly;
		}
		// If Other has No Team (ID 255), treat as Neutral
		else if (OtherTeamId == FGenericTeamId::NoTeam)
		{
			return ETeamAttitude::Neutral;
		}
	}

	// By default, treat everything not on my team as Hostile
	return ETeamAttitude::Hostile;
}