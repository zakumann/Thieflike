// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Enable crouching
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;

	// Position the camera slightly above the eyes
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));

	// Initialize TargetCapsuleHalfHeight to current standing height
	TargetCapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	// Create and attach the first person camera component
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	check(FirstPersonCameraComponent != nullptr);
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Position the camera slightly above the eyes
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));

	// Set camera properties
	FirstPersonCameraComponent->FieldOfView = 90.0f;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 90.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.60f;

	// Create and attach the first person mesh component
	FirstPersonMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	check(FirstPersonMeshComponent != nullptr);
	FirstPersonMeshComponent->SetOnlyOwnerSee(true);
	FirstPersonMeshComponent->SetupAttachment(FirstPersonCameraComponent);
	FirstPersonMeshComponent->bCastDynamicShadow = false;
	FirstPersonMeshComponent->CastShadow = false;

	// Set Leaning variables
	MaxLeanAngle = 20.0f;
	LeanSpeed = 6.0f;
	TargetLean = 0.0f;
	CurrentLean = 0.0f; // Initialize CurrentLean
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
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("We are using FPSCharacter."));
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth Lean Interpolation
	CurrentLean = FMath::FInterpTo(CurrentLean, TargetLean, DeltaTime, LeanSpeed);

	if (FirstPersonCameraComponent)
	{
		// Apply the lean rotation to the camera component
		FRotator NewCameraRotation = FirstPersonCameraComponent->GetRelativeRotation();
		NewCameraRotation.Roll = CurrentLean; // Roll axis handles the left/right tilt
		FirstPersonCameraComponent->SetRelativeRotation(NewCameraRotation);
	}

	// -------- Smooth Crouch Capsule Height Transition --------
	float CurrentHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	// Interpolate the current height towards the target height
	float NewHalfHeight = FMath::FInterpTo(CurrentHalfHeight, TargetCapsuleHalfHeight, DeltaTime, CrouchTransitionSpeed);

	// Update the capsule half-height
	GetCapsuleComponent()->SetCapsuleHalfHeight(NewHalfHeight);
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
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// 2D Vector of movement values returned from the input action
	const FVector2D MovementValue = Value.Get<FVector2D>();

	// Check if the controller posessing this Actor is valid
	if (Controller)
	{
		// Find out which way is forward and right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement input based on direction and input value
		AddMovementInput(ForwardDirection, MovementValue.Y);
		AddMovementInput(RightDirection, MovementValue.X);
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
	// Toggle Crouch
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
	}
	else
	{
		Crouch();
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

void APlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// Set the target height for the Tick function to interpolate towards (e.g., 44.0f)
	TargetCapsuleHalfHeight = 44.0f; // Half the original height of 88.0f

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
		FirstPersonCameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 32.0f));
	}
}

void APlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// Set the target height for the Tick function to interpolate towards (e.g., 88.0f)
	TargetCapsuleHalfHeight = 88.0f; // Original standing height

	if (GetCharacterMovement())
	{
		// Set back to your default walk speed (e.g., 600.0f)
		GetCharacterMovement()->MaxWalkSpeed = 600.0f;
		FirstPersonCameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	}
}

