// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacter.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Player height
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0);

	// Create a first person camera component
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	check(FirstPersonCameraComponent != nullptr);
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Position the camera slightly above the eyes
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	// Set camera properties
	FirstPersonCameraComponent->FieldOfView = 90.0f;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 90.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.60f;

	// Create a first person mesh component for the player character
	FirstPersonMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	check(FirstPersonMeshComponent != nullptr);
	FirstPersonMeshComponent->SetOnlyOwnerSee(true);
	FirstPersonMeshComponent->SetupAttachment(FirstPersonCameraComponent);
	FirstPersonMeshComponent->bCastDynamicShadow = false;
	FirstPersonMeshComponent->CastShadow = false;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	check(GEngine != nullptr);

	// Get the player controller for this character
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		// Get the enhanced input local player subsystem and add a new input mapping context to it
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(FirstPersonContext, 0);
		}
	}
	// Display a debug message for five seconds. 
	// The -1 "Key" value argument prevents the message from being updated or refreshed.
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("We are using AdventureCharacter."));
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth Lean Interpolation
	CurrentLean = FMath::FInterpTo(CurrentLean, TargetLean, DeltaTime, LeanSpeed);

	// Apply Roll (lean tilt)
	FRotator CamRot = FirstPersonCameraComponent->GetRelativeRotation();
	CamRot.Roll = CurrentLean;
	FirstPersonCameraComponent->SetRelativeRotation(CamRot);


	// -------- Smooth Crouch Capsule Height --------
	float TargetHalfHeight = GetCharacterMovement()->IsCrouching() ? 44.0f : 88.0f;


	float CurrentHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();


	float NewHalfHeight = FMath::FInterpTo(CurrentHalfHeight, TargetHalfHeight, DeltaTime, 10.0f);


	float HeightDelta = NewHalfHeight - CurrentHalfHeight;


	GetCapsuleComponent()->SetCapsuleHalfHeight(NewHalfHeight);
	AddActorWorldOffset(FVector(0, 0, -HeightDelta));
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Check the UInputComponent passed to this function and cast it to an UEnhancedInputComponent
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind Movement Actions
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

		// Bind Look Actions
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);

		// Bind Jump Actions
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Lean
		EnhancedInputComponent->BindAction(LeanLeftAction, ETriggerEvent::Started, this, &APlayerCharacter::LeanLeft);
		EnhancedInputComponent->BindAction(LeanLeftAction, ETriggerEvent::Completed, this, &APlayerCharacter::LeanLeft);


		EnhancedInputComponent->BindAction(LeanRightAction, ETriggerEvent::Started, this, &APlayerCharacter::LeanRight);
		EnhancedInputComponent->BindAction(LeanRightAction, ETriggerEvent::Completed, this, &APlayerCharacter::LeanRight);

		// Crouch
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::StartCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::StartCrouch);
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// 2D Vector of movement values returned from the input action
	const FVector2D MovementValue = Value.Get<FVector2D>();

	// Check if the controller posessing this Actor is valid
	if (Controller)
	{
		// Add left and right movement
		const FVector Right = GetActorRightVector();
		AddMovementInput(Right, MovementValue.X);

		// Add forward and back movement
		const FVector Forward = GetActorForwardVector();
		AddMovementInput(Forward, MovementValue.Y);
	}
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisValue = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisValue.X);
		AddControllerPitchInput(LookAxisValue.Y);
	}
}

void APlayerCharacter::StartCrouch(const FInputActionValue& Value)
{
	const bool bPressed = Value.Get<bool>();
	if (!bPressed) return; // Toggle only on press

	// Toggle Crouch
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
		GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	}
	else
	{
		Crouch();
		GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
	}
}

void APlayerCharacter::LeanRight(const FInputActionValue& Value)
{
	const bool bPressed = Value.Get<bool>();
	TargetLean = bPressed ? MaxLeanAngle : 0.0f;
}

void APlayerCharacter::LeanLeft(const FInputActionValue& Value)
{
	const bool bPressed = Value.Get<bool>();
	TargetLean = bPressed ? -MaxLeanAngle : 0.0f;
}

