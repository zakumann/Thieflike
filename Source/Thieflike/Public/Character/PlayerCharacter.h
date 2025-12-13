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

	// Lean Right Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LeanRightAction;

	// Lean Left Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LeanLeftAction;

	// Sprint Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* WalkAction;

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
	void LeanRight(const FInputActionValue& Value);
	void LeanLeft(const FInputActionValue& Value);
	void StartWalk();
	void StopWalk();


	//---- Leaning Functions ----//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaning")
	float MaxLeanDistance;

	// Lean Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leaning")
	float LeanSpeed;

	// Current and Target Lean Values
	float CurrentLean;
	float TargetLean;

	//Crouch Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching")
	float CrouchSpeed = 200.0f;

	// Crouch Transition Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouching")
	float CrouchTransitionSpeed = 10.0f;

	//Sprint Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprinting")
	float RunSpeed = 500.0f;

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
