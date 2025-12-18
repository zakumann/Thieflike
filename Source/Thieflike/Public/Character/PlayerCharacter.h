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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Handles Movement Input
	void Move(const FInputActionValue& Value);

	// Handles Look Input
	void Look(const FInputActionValue& Value);

	void StartCrouch(const FInputActionValue& Value);
	void StartLeanRight(const FInputActionValue& Value);
	void StopLeanRight(const FInputActionValue& Value);
	void StartLeanLeft(const FInputActionValue& Value);
	void StopLeanLeft(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();


	//---- Leaning Functions ----//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaning")
	float MaxLeanOffset = 20.0f; // Move camera right/left

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "leaning")
	float MaxLeanRoll = 12.0f; // Leaning Camera

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "leaning")
	float LeanInterpSpeed = 8.0f;

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

	/** Current visibility percentage (0 = fully hidden, 100 = fully visible) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	float CurrentVisibility;

	/** Calculates the current visibility of the character based on surrounding light */
	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void CalculateVisibility();

	// Default exposure needed to be visible (adjustable in Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	float VisibilityThreshold = 0.5f;

	// How quickly the visibility value changes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	float VisibilityInterpSpeed = 5.0f;

	// A factor to simulate ambient light when not directly in a bright spot
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	float AmbientLightFactor = 0.1f; // Represents 10% ambient light when completely hidden from direct light

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

private:
	// Variable to track the target height for smooth transition
	float TargetCapsuleHalfHeight;

	// Store the original camera relative location
	FVector DefaultSpringArmLocation;
};
