// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyAIController.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

AEnemyAIController::AEnemyAIController()
{
	// Set this agent's team to Team 1 (The Enemy Team)
	// Usually: 0 = No Team, 1 = Red, 2 = Blue, etc.
	TeamId = FGenericTeamId(1);
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Cache reference to controlled enemy
	ControlledEnemy = Cast<AEnemyCharacterBase>(InPawn);

	if (!ControlledEnemy)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyAIController: Tried to possess non-enemy pawn '%s'"),
			InPawn ? *InPawn->GetName() : TEXT("NULL"));
		return;
	}

	// Check if behavior tree is assigned
	if (!ControlledEnemy->EnemyBehaviorTree)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyAIController: No Behavior Tree assigned to '%s'!"),
			*ControlledEnemy->GetName());
		UE_LOG(LogTemp, Error, TEXT("  -> Set 'Enemy Behavior Tree' in the Blueprint!"));
		return;
	}

	// Start the Behavior Tree
	// This automatically initializes the Blackboard if the BT has one assigned
	bool bSuccess = RunBehaviorTree(ControlledEnemy->EnemyBehaviorTree);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("EnemyAIController: Successfully started Behavior Tree for '%s'"),
			*ControlledEnemy->GetName());

		// Verify blackboard is initialized
		if (GetBlackboardComponent())
		{
			UE_LOG(LogTemp, Log, TEXT("  -> Blackboard initialized successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("  -> Blackboard is NULL! Check BT has a Blackboard asset assigned."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyAIController: Failed to start Behavior Tree for '%s'"),
			*ControlledEnemy->GetName());
	}
}

void AEnemyAIController::OnUnPossess()
{
	Super::OnUnPossess();

	ControlledEnemy = nullptr;
}

// ========== BLACKBOARD UPDATE FUNCTIONS ==========

void AEnemyAIController::UpdateBlackboard_TargetActor(AActor* NewTarget)
{
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsObject(BB_TargetActor, NewTarget);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateBlackboard_TargetActor: Blackboard is NULL!"));
	}
}

void AEnemyAIController::UpdateBlackboard_LastKnownLocation(FVector Location)
{
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsVector(BB_LastKnownLocation, Location);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateBlackboard_LastKnownLocation: Blackboard is NULL!"));
	}
}

void AEnemyAIController::UpdateBlackboard_Distance(float Distance)
{
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsFloat(BB_DistanceToTarget, Distance);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateBlackboard_Distance: Blackboard is NULL!"));
	}
}

void AEnemyAIController::UpdateBlackboard_CanSeePlayer(bool bCanSee)
{
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsBool(BB_CanSeePlayer, bCanSee);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateBlackboard_CanSeePlayer: Blackboard is NULL!"));
	}
}

void AEnemyAIController::UpdateBlackboard_IsStunned(bool bStunned)
{
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsBool(BB_IsStunned, bStunned);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateBlackboard_IsStunned: Blackboard is NULL!"));
	}
}

// ========== TEAM INTERFACE ==========

FGenericTeamId AEnemyAIController::GetGenericTeamId() const
{
	return TeamId;
}

ETeamAttitude::Type AEnemyAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	// Check if other actor implements team interface
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);

	if (OtherTeamAgent)
	{
		FGenericTeamId OtherTeamId = OtherTeamAgent->GetGenericTeamId();

		// Same team = friendly
		if (OtherTeamId == TeamId)
		{
			return ETeamAttitude::Friendly;
		}
		// No team = neutral
		else if (OtherTeamId == FGenericTeamId::NoTeam)
		{
			return ETeamAttitude::Neutral;
		}
	}

	// Everything else = hostile
	return ETeamAttitude::Hostile;
}
