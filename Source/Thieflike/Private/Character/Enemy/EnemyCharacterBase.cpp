// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyCharacterBase.h"
#include "Character/Enemy/EnemyAIController.h"
#include "Character/PlayerCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

AEnemyCharacterBase::AEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// ========== SET AI CONTROLLER CLASS ==========
	// CRITICAL: This tells Unreal to use our custom AI controller
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// ========== SETUP PERCEPTION COMPONENT ==========
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComp"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	// Configure sight sense
	SightConfig->SightRadius = BaseSightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngle;

	// Detect all affiliations (ensures we detect player)
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	// Add sight sense to perception component
	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	// ========== INITIALIZE HEALTH ==========
	CurrentHealth = MaxHealth;
}

void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Cache AI controller reference
	AIController = Cast<AEnemyAIController>(GetController());

	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyCharacterBase '%s': AI Controller is not EnemyAIController!"),
			*GetName());
		UE_LOG(LogTemp, Error, TEXT("  -> Check AIControllerClass is set correctly"));
		return;
	}

	// Bind perception event
	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacterBase::OnTargetDetected);
		UE_LOG(LogTemp, Log, TEXT("EnemyCharacterBase '%s': Perception system initialized"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EnemyCharacterBase '%s': AIPerceptionComp is NULL!"), *GetName());
	}
}

void AEnemyCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Movement is handled by Behavior Tree
	// We don't put any AI logic here
}

// ========== PERCEPTION SYSTEM ==========

void AEnemyCharacterBase::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	// Ensure we have a valid AI controller
	if (!AIController)
	{
		return;
	}

	// Check if detected actor is player
	APlayerCharacter* SensedPlayer = Cast<APlayerCharacter>(Actor);
	if (!SensedPlayer)
	{
		return; // Not the player, ignore
	}

	// Check if stimulus was successfully sensed (line of sight exists)
	if (Stimulus.WasSuccessfullySensed())
	{
		// ========== PLAYER SPOTTED ==========

		// Check stealth level (light-based detection)
		if (CanSeePlayerDespiteStealth(SensedPlayer))
		{
			// Calculate distance
			float Distance = FVector::Dist(GetActorLocation(), SensedPlayer->GetActorLocation());

			// Update blackboard with player info
			AIController->UpdateBlackboard_TargetActor(SensedPlayer);
			AIController->UpdateBlackboard_LastKnownLocation(SensedPlayer->GetActorLocation());
			AIController->UpdateBlackboard_Distance(Distance);
			AIController->UpdateBlackboard_CanSeePlayer(true);

			UE_LOG(LogTemp, Warning, TEXT("'%s' spotted player! Distance: %.1f"),
				*GetName(), Distance);
		}
		else
		{
			// Player is in line of sight but too dark to detect
			AIController->UpdateBlackboard_CanSeePlayer(false);

			UE_LOG(LogTemp, Log, TEXT("'%s': Player in LoS but too dark/stealthy"), *GetName());
		}
	}
	else
	{
		// ========== LOST SIGHT OF PLAYER ==========

		// Store last known location before clearing target
		AIController->UpdateBlackboard_LastKnownLocation(SensedPlayer->GetActorLocation());
		AIController->UpdateBlackboard_CanSeePlayer(false);

		UE_LOG(LogTemp, Log, TEXT("'%s' lost sight of player - investigating last known location"),
			*GetName());
	}
}

bool AEnemyCharacterBase::CanSeePlayerDespiteStealth(APlayerCharacter* Player)
{
	if (!Player)
	{
		return false;
	}

	// Calculate distance to player
	float DistanceToPlayer = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

	// Get player's stealth visibility factor (0.0 = hidden, 1.0 = exposed)
	float VisibilityFactor = Player->GetStealthVisibilityFactor();

	// Calculate effective detection range based on visibility
	// If visibility is 0 (dark), we only see up to DarkVisionRange
	// If visibility is 1 (bright), we see up to BaseSightRadius
	float EffectiveDetectionRange = FMath::Lerp(DarkVisionRange, BaseSightRadius, VisibilityFactor);

	// Player is detected if within effective range
	return DistanceToPlayer <= EffectiveDetectionRange;
}

// ========== HEALTH & DAMAGE SYSTEM ==========

float AEnemyCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	// Call parent implementation
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// Apply damage to health
	CurrentHealth -= ActualDamage;
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

	UE_LOG(LogTemp, Warning, TEXT("'%s' took %.1f damage! Health: %.1f/%.1f"),
		*GetName(), ActualDamage, CurrentHealth, MaxHealth);

	// Check if dead
	if (CurrentHealth <= 0.0f)
	{
		Die();
		return ActualDamage;
	}

	// Check for knockout (80%+ damage in one hit)
	if (ActualDamage >= MaxHealth * 0.8f)
	{
		ApplyStun(StunDuration);
		UE_LOG(LogTemp, Warning, TEXT("'%s' was knocked out!"), *GetName());
	}
	// Otherwise, if not already alert, investigate damage source
	else if (AIController)
	{
		UBlackboardComponent* BB = AIController->GetBlackboardComponent();
		if (BB)
		{
			AActor* CurrentTarget = Cast<AActor>(BB->GetValueAsObject(AEnemyAIController::BB_TargetActor));

			// If not currently chasing anyone, investigate the attacker
			if (!CurrentTarget && EventInstigator)
			{
				APawn* Attacker = EventInstigator->GetPawn();
				if (Attacker)
				{
					AIController->UpdateBlackboard_LastKnownLocation(Attacker->GetActorLocation());
					UE_LOG(LogTemp, Log, TEXT("'%s' was damaged - investigating source"), *GetName());
				}
			}
		}
	}

	return ActualDamage;
}

void AEnemyCharacterBase::ApplyStun(float Duration)
{
	UE_LOG(LogTemp, Warning, TEXT("'%s' stunned for %.1f seconds"), *GetName(), Duration);

	// Update blackboard
	if (AIController)
	{
		AIController->UpdateBlackboard_IsStunned(true);
	}

	// Stop all movement
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
	}

	// Set timer to end stun
	GetWorld()->GetTimerManager().SetTimer(
		StunTimerHandle,
		this,
		&AEnemyCharacterBase::OnStunEnd,
		Duration,
		false
	);
}

void AEnemyCharacterBase::OnStunEnd()
{
	UE_LOG(LogTemp, Log, TEXT("'%s' recovered from stun"), *GetName());

	// Clear stun flag in blackboard
	if (AIController)
	{
		AIController->UpdateBlackboard_IsStunned(false);
	}
}

void AEnemyCharacterBase::Die()
{
	UE_LOG(LogTemp, Warning, TEXT("'%s' died!"), *GetName());

	// Stop AI
	if (AIController)
	{
		AIController->StopMovement();
		AIController->GetBrainComponent()->StopLogic("Dead");
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Enable ragdoll
	GetMesh()->SetSimulatePhysics(true);

	// Destroy after 5 seconds
	SetLifeSpan(5.0f);
}
