// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyCharacterBase.generated.h"

// Simple State Machine for the Enemy
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
		Idle			UMETA(DisplayName = "Idle"),
		Patrol			UMETA(DisplayName = "Patrol"),
		Chase			UMETA(DisplayName = "Chase"),
		Investigate		UMETA(DisplayName = "Investigate"),
		Attack			UMETA(DisplayName = "Attack")
};

UCLASS()
class THIEFLIKE_API AEnemyCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacterBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// --- Enemy Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAISenseConfig_Sight* SightConfig;

	// --- AI Stats (Editable in BP for variations) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Stats")
	float BaseSightRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Stats")
	float LoseSightRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Stats")
	float PeripheralVisionAngle = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Stats")
	float ChaseSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Stats")
	float PatrolSpeed = 300.f;

	// How close the player must be to be seen in Total Darkness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Stats")
	float DarkVisionRange = 200.0f;

protected:
	// ---State Management ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI State")
	EEnemyState CurrentState;

	UPROPERTY(BlueprintReadOnly, Category = "AI State")
	class APlayerCharacter* TargetPlayer;

	UPROPERTY(BlueprintReadOnly, Category = "AI State")
	class AAIController* EnemyController;

	// Functions ---
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

	// The logic to decide if we Actually see the player based on light level
	bool CanSeePlayerDespiteStealth(APlayerCharacter* Player);

	void SetEnemyState(EEnemyState NewState);

	// Virtual functions for child classes to override behaviors
	virtual void HandlePatrol(float DeltaTime);
	virtual void HandleChase(float DeltaTime);
	virtual void HandleInvestigate(float DeltaTime);
};
