// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyCharacterBase.h"
#include "Character/PlayerCharacter.h" // Include your player header
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AEnemyCharacterBase::AEnemyCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 1. Setup Perception Component
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComp"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	// 2. Configure Sight
	SightConfig->SightRadius = BaseSightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngle;

	// Important: Detect all affiliations (Friendly, Neutral, Enemy) to ensure Player is caught
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	// Add the sense to the component
	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	CurrentState = EEnemyState::Idle;
}

// Called when the game starts or when spawned
void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	EnemyController = Cast<AAIController>(GetController());
	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacterBase::OnTargetDetected);
	}

	// Apply initial speed
	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
}

// Called every frame
void AEnemyCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// State Machine Logic
	switch (CurrentState)
	{
	case EEnemyState::Idle:
		// Logic for looking around or waiting
		break;
	case EEnemyState::Patrol:
		HandlePatrol(DeltaTime);
		break;
	case EEnemyState::Chase:
		HandleChase(DeltaTime);
		break;
	case EEnemyState::Investigate:
		HandleInvestigate(DeltaTime);
		break;
	}
}

void AEnemyCharacterBase::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	APlayerCharacter* SensedPlayer = Cast<APlayerCharacter>(Actor);
	if (!SensedPlayer) return;

	// Was the stimulus "Successfully Sensed"? (True = Saw them, False = Lost them)
	if (Stimulus.WasSuccessfullySensed())
	{
		// --- STEALTH CHECK ---
		// The Perception system says "Line of Sight exists". 
		// Now WE decide if it's bright enough to actually trigger aggro.

		if (CanSeePlayerDespiteStealth(SensedPlayer))
		{
			TargetPlayer = SensedPlayer;
			SetEnemyState(EEnemyState::Chase);
			UE_LOG(LogTemp, Warning, TEXT("Player Spotted! Light Level High enough."));
		}
		else
		{
			// We physically see them, but it's too dark, so we ignore (or maybe investigate)
			// Optional: If very close but dark, switch to Investigate instead of Chase
			UE_LOG(LogTemp, Log, TEXT("Player in LoS, but too dark/hidden."));
		}
	}
	else
	{
		// Lost sight of player
		// After a delay, return to patrol (simplified here)
		if (CurrentState == EEnemyState::Chase)
		{
			TargetPlayer = nullptr;
			SetEnemyState(EEnemyState::Idle); // Or investigate last known location
		}
	}
}

bool AEnemyCharacterBase::CanSeePlayerDespiteStealth(APlayerCharacter* Player)
{
	if (!Player) return false;

	// 1. Get Distance
	float DistanceToPlayer = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

	// 2. Get Player's Visibility Factor (0.0 to 1.0) calculated in your PlayerCharacter
	// 0.0 = Pitch Black/Crouched, 1.0 = Bright Light/Running
	float VisibilityFactor = Player->GetStealthVisibilityFactor();

	// 3. Logic:
	// If Visibility is 1.0, we see them at full BaseSightRadius.
	// If Visibility is 0.0, we only see them at DarkVisionRange.

	// Lerp calculates the "Effective Vision Range" for the current light level
	float EffectiveDetectionRange = FMath::Lerp(DarkVisionRange, BaseSightRadius, VisibilityFactor);

	// If the player is within this calculated range, they are seen.
	return DistanceToPlayer <= EffectiveDetectionRange;
}

void AEnemyCharacterBase::SetEnemyState(EEnemyState NewState)
{
	if (CurrentState == NewState) return;

	CurrentState = NewState;

	switch (CurrentState)
	{
	case EEnemyState::Chase:
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
		break;
	case EEnemyState::Patrol:
	case EEnemyState::Idle:
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
		if (EnemyController) EnemyController->StopMovement();
		break;
	}
}


void AEnemyCharacterBase::HandleChase(float DeltaTime)
{
	if (!TargetPlayer || !EnemyController)
	{
		SetEnemyState(EEnemyState::Patrol);
		return;
	}

	// Continuously check stealth while chasing? 
	// Usually once spotted, you stay spotted for a bit, but let's re-check
	// to allow player to run into shadows to escape.
	if (!CanSeePlayerDespiteStealth(TargetPlayer))
	{
		// Player ran into darkness!
		// In a real game, you would go to "Investigate" last known location.
		SetEnemyState(EEnemyState::Idle);
		TargetPlayer = nullptr;
		return;
	}

	// Move to Actor
	EnemyController->MoveToActor(TargetPlayer, 100.0f); // Stop 100 units away
}

void AEnemyCharacterBase::HandlePatrol(float DeltaTime)
{
	// Override this in BP or child class to follow waypoints
}

void AEnemyCharacterBase::HandleInvestigate(float DeltaTime)
{
	// Logic to move to the location where a sound was heard or player was last seen
}

