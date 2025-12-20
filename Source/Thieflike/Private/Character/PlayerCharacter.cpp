// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h" // For TActorIterator
#include "Engine/DirectionalLight.h" // To easily find the main light source
#include "Components/PointLightComponent.h" // For point lights
#include "Components/SpotLightComponent.h" // For spot lights
#include "Kismet/KismetSystemLibrary.h" // For UKismetSystemLibrary::LineTraceSingleByChannel 
#include "Object/Door.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Enable crouching
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// Initialize TargetCapsuleHalfHeight to current standing height
	TargetCapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	// Create and attach the first person Spring Arm component
	FirstPersonSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("FirstPersonSpringArm"));
	check(FirstPersonSpringArmComponent != nullptr);
	FirstPersonSpringArmComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonSpringArmComponent->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	// Position the spring arm at the character's eye level
	FirstPersonSpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	// Set the arm length to zero to position the camera at the character's location
	FirstPersonSpringArmComponent->TargetArmLength = 0.0f;

	// Create and attach the first person camera component
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	check(FirstPersonCameraComponent != nullptr);
	FirstPersonCameraComponent->SetupAttachment(FirstPersonSpringArmComponent, USpringArmComponent::SocketName);// Attach to the end of the spring arm
	// Disable pawn control rotation, we want the spring arm to handle it
	FirstPersonCameraComponent->bUsePawnControlRotation = false;

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

	if (!FirstPersonSpringArmComponent && !FirstPersonCameraComponent)
	{
		return;
	}

	float AllowedLean = GetAllowedLeanOffset(TargetLeanOffset); //GetAllowedLeanOffset is for Lean to the Playercharacter FirstPersonSpringArmComponent.
	float LeanRatio = (MaxLeanOffset != 0.f) ? FMath::Abs(CurrentLeanOffset / MaxLeanOffset) : 0.f; // While Leaning Roll until contacts the wall

	CurrentLeanOffset = FMath::FInterpTo(CurrentLeanOffset, AllowedLean, DeltaTime, LeanInterpSpeed);
	CurrentLeanRoll = FMath::FInterpTo(CurrentLeanRoll, TargetLeanRoll * LeanRatio, DeltaTime, LeanInterpSpeed);

	// Move camera right/left
	FVector SocketOffset = FirstPersonSpringArmComponent->SocketOffset;
	SocketOffset.Y = CurrentLeanOffset;
	FirstPersonSpringArmComponent->SocketOffset = SocketOffset;

	// Roll
	FirstPersonCameraComponent->SetRelativeRotation(FRotator(0.f, 0.f, CurrentLeanRoll));

	// Calculate visibility every frame
	CalculateVisibility();

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
		EnhancedInputComponent->BindAction(LeanRightAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLeanRight);
		EnhancedInputComponent->BindAction(LeanRightAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopLeanRight);
		EnhancedInputComponent->BindAction(LeanLeftAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLeanLeft);
		EnhancedInputComponent->BindAction(LeanLeftAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopLeanLeft);

		// Crouch
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::StartCrouch);

		// Walk
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopSprint);

		// Interact
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &APlayerCharacter::Interact);
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

void APlayerCharacter::StartLeanRight(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Lean Right Started"));
	TargetLeanOffset = +MaxLeanOffset;
	TargetLeanRoll = +MaxLeanRoll;
}

void APlayerCharacter::StopLeanRight(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Lean Right Stopped"));
	TargetLeanOffset = 0.0f;
	TargetLeanRoll = 0.0f;
}

void APlayerCharacter::StartLeanLeft(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Lean Left Started"));
	TargetLeanOffset = -MaxLeanOffset;
	TargetLeanRoll = -MaxLeanRoll;
}

void APlayerCharacter::StopLeanLeft(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Lean Left Stopped"));
	TargetLeanOffset = 0.0f;
	TargetLeanRoll = 0.0f;
}


void APlayerCharacter::Interact()
{
	FHitResult HitResult;
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + FirstPersonCameraComponent->GetForwardVector() * InteractLineTraceLength;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, Params);

	ADoor* Door = Cast<ADoor>(HitResult.GetActor());
	if (Door)
	{
		FVector PlayerForward = GetActorForwardVector();
		Door->OnInteract(PlayerForward);
	}
}

void APlayerCharacter::StartSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void APlayerCharacter::StopSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

float APlayerCharacter::GetAllowedLeanOffset(float DesiredLean)
{
	if (!GetWorld()) return DesiredLean;

	FVector Start = FirstPersonCameraComponent->GetComponentLocation();

	// Lean direction(Right/Left)
	FVector RightVector = FirstPersonCameraComponent->GetRightVector();
	FVector Direction = (DesiredLean > 0.f) ? RightVector : -RightVector;

	FVector End = Start + Direction * LeanCheckDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	if (bHit)
	{
		float Distance = FVector::Distance(Start, Hit.ImpactPoint);
		float Allowed = Distance - LeanSafetyMargin;

		return FMath::Clamp(Allowed, 0.f, FMath::Abs(DesiredLean)) * FMath::Sign(DesiredLean);
	}
	return DesiredLean;
}

// Calculate the player's visibility based on lighting conditions
void APlayerCharacter::CalculateVisibility()
{
	// For simplicity, we'll set CurrentVisibility to a fixed value.
	// In a real implementation, you would analyze the lighting conditions around the character.
	CurrentVisibility = 50.0f; // Placeholder value

	// Set visibility based on exposure threshold
	if (CurrentVisibility >= VisibilityThreshold * 100.0f)
	{
		// Player is visible
		// You can add logic here to notify AI or trigger events
		APlayerCharacter::CurrentVisibility = 100.0f;
	}
	else
	{
		// Player is hidden
		// You can add logic here for stealth mechanics
		APlayerCharacter::CurrentVisibility = 0.0f;
	}

}

void APlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// Set the target height for the Tick function to interpolate towards (e.g., 44.0f)
	TargetCapsuleHalfHeight = 44.0f; // Half the original height of 88.0f

	if (GetCharacterMovement() && FirstPersonSpringArmComponent && FirstPersonCameraComponent)
	{
		GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
		FirstPersonSpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 32.0f));
		// Camera rotation reset
		FirstPersonCameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
		TargetLeanOffset = 0.0f;
		TargetLeanRoll = 0.0f;
	}
}

void APlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// Set the target height for the Tick function to interpolate towards (e.g., 88.0f)
	TargetCapsuleHalfHeight = 88.0f; // Original standing height

	if (GetCharacterMovement() && FirstPersonSpringArmComponent && FirstPersonCameraComponent)
	{
		// Set back to your default walk speed (e.g., 600.0f)
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		FirstPersonSpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
		// Camera rotation reset
		FirstPersonCameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
		TargetLeanOffset = 0.0f;
		TargetLeanRoll = 0.0f;
	}
}

