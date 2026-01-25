// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h" 
#include "InputActionValue.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "PlayerCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class UInputComponent;
class ALightDetector;
class UChildActorComponent;
class ALadder;

UCLASS()
class THIEFLIKE_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Landed(const FHitResult& Hit) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* FirstPersonContext;

	// Move Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction;

	// Look Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAction;

	// Jump Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* JumpAction;

	// Crouch Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* CrouchAction;

	// Lean Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LeanRightAction;

	// Lean Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LeanLeftAction;

	// Sprint Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* SprintAction;

	// Interact Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* InteractAction;

	// Mantle Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MantleAction;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Handles Movement Input
	void Move(const FInputActionValue& Value);

	// Handles Look Input
	void Look(const FInputActionValue& Value);

	void Jump();
	/*void WhileJumping();*/

	void StartCrouch(const FInputActionValue& Value);
	void StartLeanRight(const FInputActionValue& Value);
	void StopLeanRight(const FInputActionValue& Value);
	void StartLeanLeft(const FInputActionValue& Value);
	void StopLeanLeft(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();

	// ---- Mantle Input Handlers ----
	void StartMantle(const FInputActionValue& Value);
	void OngoingMantle(const FInputActionValue& Value);
	void EndMantle(const FInputActionValue& Value);

public:
	// ---- Mantle ---- //
	void PerformMantle();
protected:

	// Possibility of Mantling.
	bool CanMantle(FVector& OutMantleTargetLocation);

	// Complete it when finish mantle
	void CompleteMantleSequence();
	// If failed stop mantle
	void StopMantle(bool bSuccess);

	// Safety: Previous location to check if we are stuck in mantle
	float StuckTimer = 0.0f;

public:
	//---- Interact ----//
	void Interact();

	UPROPERTY(EditAnywhere)
	float InteractLineTraceLength = 350.f;

	//---- Leaning Functions ----//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaning")
	float MaxLeanOffset = 20.0f; // Move camera right/left

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "leaning")
	float MaxLeanRoll = 12.0f; // Leaning Camera

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "leaning")
	float LeanInterpSpeed = 12.0f;

	// Runtime
	float TargetLeanOffset = 0.0f;
	float CurrentLeanOffset = 0.0f;

	float TargetLeanRoll = 0.0f;
	float CurrentLeanRoll = 0.0f;

	// Lean Wall Check
	UPROPERTY(EditAnywhere, Category = "Leaning|WallCheck")
	float LeanCheckDistance = 35.0f;   // Check the leanDistance

	UPROPERTY(EditAnywhere, Category = "Leaning|WallCheck")
	float LeanSafetyMargin = 5.0f;     // between Wall and Lean safety Margin

	float GetAllowedLeanOffset(float DesiredLean);

	//---- Stealth System Variables & Functions ----//
public:
	// Light Detector Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	UChildActorComponent* LightDetectorComponent;

	// Player's current light level
	// 0.0: hiding in darkness, 1.0: fully illuminated
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	float CurrentLightLevel = 0.0f;

	// Calculate Stealth Visibility Factor
	// This function combines light level, movement speed, and posture to determine visibility
	// ex: return (CurrentLightLevel * MovementSpeedModifier * PostureModifier);
	UFUNCTION(BlueprintCallable, Category = "Stealth")
	float GetStealthVisibilityFactor() const;
protected:
	// Light Detector Instance
	UPROPERTY()
	ALightDetector* LightDetectorInstance = nullptr;

	// Update Stealth Level based on Light Detector and other factors
	void UpdateStealthLevel();
public:
	//Crouch Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching")
	float CrouchSpeed = 150.0f;

	// Crouch Transition Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching")
	float CrouchTransitionSpeed = 10.0f;

	//Sprint Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprinting")
	float RunSpeed = 600.0f;

	//Walk Speed
	float WalkSpeed = 300.0f;

	// First Person Spring Arm
	UPROPERTY(VisibleAnywhere, Category = Camera)
	USpringArmComponent* FirstPersonSpringArmComponent;

	// First Person camera
	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FirstPersonCameraComponent;

	// First Person Mesh(arms; seen only by self)
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FirstPersonMeshComponent;

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

protected:
	// ---- Mantle Vaariables & Functions ---- //
	UPROPERTY(VisibleAnywhere, Category = "Mantle")
	bool bHasMantledThisJump = false;

	UPROPERTY(VisibleAnywhere, Category = "Mantle")
	bool bCanMantle = false;

	UPROPERTY(EditAnywhere, Category = "Mantle")
	float MantleTraceDistance = 100.0f; // detect the wall

	/** using with animation (if use this) */
	UPROPERTY(EditAnywhere, Category = "Mantle")
	float MantleDuration = 0.8f;

	// calculate variable
	FVector MantlePos1; // hainging position
	FVector MantlePos2; // climb up position
	FVector LastMantleLocation;
	FTimerHandle MantleTimerHandle;

	// Variable to track the target height for smooth transition
	float TargetCapsuleHalfHeight;

	// Store the original camera relative location
	FVector DefaultSpringArmLocation;
protected:
	UPROPERTY(VisibleAnywhere, Category = "Movement|Ladder")
	bool bIsClimbingLadder = false;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Ladder")
	class ALadder* CurrentLadder = nullptr;

	UPROPERTY(EditAnywhere, Category = "Movement|Ladder")
	float LadderClimbSpeed = 100.0f;

	// reach the top of the ladder
	FTimerHandle LadderFinishTimerHandle;

	// after reaching the top of the ladder, delay before restoring control
	void PerformLadderTopClimb();

	// Complete it when finish climbing ladder
	void FinishLadderClimbSequence();
protected:
	// Check Mantle
	bool bIsMantling = false;

public:
	void SetLadderMode(bool bEnable, class ALadder* Ladder);

	void CheckLadderConstraints();

public:
	// --- Inventory / Economy ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 CurrentMoney = 0;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddMoney(int32 Amount);
private:
	FVector2D LastMovementInput = FVector2D::ZeroVector;

public:
	FVector2D GetInputVector() const
	{
		return LastMovementInput;
	}
};