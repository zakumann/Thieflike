// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h" // For TActorIterator
#include "Engine/DirectionalLight.h" // To easily find the main light source
#include "Components/PointLightComponent.h" // For point lights
#include "Components/SpotLightComponent.h" // For spot lights
#include "Kismet/KismetSystemLibrary.h" // For UKismetSystemLibrary::LineTraceSingleByChannel 

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Enable crouching
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;

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
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

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
	MaxLeanDistance = 20.0f;
	MaxLeanRotationAngle = 10.0f;
	LeanSpeed = 6.0f;
	TargetLean = 0.0f;
	CurrentLean = 0.0f; // Initialize CurrentLean
	TargetLeanRoll = 0.0f;
	CurrentLeanRoll = 0.0f;
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

	// Calculate visibility every frame
	CalculateVisibility();

	// -------- Smooth Lean Transition --------

	if (FirstPersonSpringArmComponent && FirstPersonCameraComponent)
	{
		// Apply the lean rotation to the camera component
		CurrentLean = FMath::FInterpTo(CurrentLean, TargetLean, DeltaTime, LeanSpeed);
		FVector CurrentSocketOffset = FirstPersonSpringArmComponent->SocketOffset;
		// We calculate the Y position relative to the default location's Y component
		CurrentSocketOffset.Y = CurrentLean;
		FirstPersonSpringArmComponent->SocketOffset = CurrentSocketOffset;

		// Apply lean rotation
		CurrentLeanRoll = FMath::FInterpTo(CurrentLeanRoll, TargetLeanRoll, DeltaTime, LeanSpeed);
		// Create a new rotator with the current lean roll
		FRotator NewCameraRotation = FRotator(0.0f, 0.0f, CurrentLeanRoll);
		// Camera rotation is relative to the spring arm
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

		// Walk
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Started, this, &APlayerCharacter::StartWalk);
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopWalk);
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
	TargetLean = bPressed ? MaxLeanDistance : 0.0f;
	TargetLeanRoll = bPressed ? MaxLeanRotationAngle : 0.0f;
	UE_LOG(LogTemp, Warning, TEXT("Lean Right Called. bPressed: %s, TargetLean: %f"), bPressed ? TEXT("true") : TEXT("false"), TargetLean);
}

void APlayerCharacter::LeanLeft(const FInputActionValue& Value)
{
	const bool bPressed = Value.Get<bool>();
	TargetLean = bPressed ? -MaxLeanDistance : 0.0f;
	TargetLeanRoll = bPressed ? -MaxLeanRotationAngle : 0.0f;
	UE_LOG(LogTemp, Warning, TEXT("Lean Left Called. bPressed: %s, TargetLean: %f"), bPressed ? TEXT("true") : TEXT("false"), TargetLean);
}

void APlayerCharacter::StartWalk()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::StopWalk()
{
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

// Calculate the player's visibility based on lighting conditions
void APlayerCharacter::CalculateVisibility()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector StartLocation = GetActorLocation(); // Or a specific socket/height on the character

	float AccumulatedLightValue = AmbientLightFactor * 100.0f; // Start with a base ambient light level

	// Use a robust line trace function
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this); // Ignore the player character itself

	// Check against the main Directional Light (Sun/Moon)
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		ADirectionalLight* DirLight = *It;
		if (DirLight && DirLight->GetLightComponent()->IsVisible())
		{
			FVector LightDirection = -DirLight->GetActorForwardVector();
			// Trace a long way in the direction of the light
			FVector EndLocation = StartLocation + (LightDirection * 100000.0f);

			bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);

			// If we didn't hit anything, we have clear line of sight to the main light source
			if (!bHit)
			{
				// Assign a high visibility value for direct sunlight
				AccumulatedLightValue += 90.0f;
				break; // We found the main light, no need to check others in the iterator
			}
		}
	}

	// You could optionally iterate through Point/Spot lights here as well, 
	// but the logic above gives a strong 'in shadow' or 'in light' value for outdoor/main lighting setups.

	// Clamp the final value and interpolate smoothly
	float NewVisibilityPercent = FMath::Clamp(AccumulatedLightValue, 0.0f, 100.0f);

	CurrentVisibility = FMath::FInterpTo(
		CurrentVisibility,
		NewVisibilityPercent,
		World->GetDeltaSeconds(),
		VisibilityInterpSpeed
	);
}

void APlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// Set the target height for the Tick function to interpolate towards (e.g., 44.0f)
	TargetCapsuleHalfHeight = 44.0f; // Half the original height of 88.0f

	if (GetCharacterMovement() && FirstPersonSpringArmComponent && FirstPersonCameraComponent)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
		FirstPersonSpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 32.0f));
		TargetLeanRoll = 0.0f; // Reset lean when crouching
		TargetLean = 0.0f;
		// Camera rotation reset
		FirstPersonCameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
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
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
		FirstPersonSpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
		TargetLeanRoll = 0.0f; // Reset lean when crouching
		TargetLean = 0.0f;
		// Camera rotation reset
		FirstPersonCameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

