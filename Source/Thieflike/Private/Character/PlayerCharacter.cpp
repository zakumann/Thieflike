// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h" // For TActorIterator
#include "Engine/DirectionalLight.h" // To easily find the main light source
#include "Components/PointLightComponent.h" // For point lights
#include "Components/SpotLightComponent.h" // For spot lights
#include "Components/BoxComponent.h" 
#include "Kismet/KismetSystemLibrary.h" // For UKismetSystemLibrary::LineTraceSingleByChannel 
#include "Object/Door.h"
#include "Object/Crate.h"
#include "Object/LootItem.h"
#include "Object/Ladder.h"
#include "Components/ChildActorComponent.h"
#include "Character/LightDetector.h" // LightDetector
#include "Weapon/Weapon.h"

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
	FirstPersonCameraComponent->SetupAttachment(FirstPersonSpringArmComponent, USpringArmComponent::SocketName);
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

	LightDetectorComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("LightDetectorComp"));
	LightDetectorComponent->SetupAttachment(GetCapsuleComponent());
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

	FActorSpawnParameters Params;
	Params.Owner = this;

	if (LightDetectorComponent)
	{
		LightDetectorInstance = Cast<ALightDetector>(LightDetectorComponent->GetChildActor());

		if (!LightDetectorInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("LightDetector is missing from PlayerCharacter!"));
		}
	}

	// Spawn default weapon if set
	if (DefaultWeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

		DefaultWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass, SpawnParams);

		if (DefaultWeapon)
		{
			DefaultWeapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				TEXT("HolsterSocket")
			);

			UE_LOG(LogTemp, Log, TEXT("Default weapon spawned and holstered"));
		}
	}
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

	// -------- Smooth Crouch Capsule Height Transition --------
	float CurrentHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	// Interpolate the current height towards the target height
	float NewHalfHeight = FMath::FInterpTo(CurrentHalfHeight, TargetCapsuleHalfHeight, DeltaTime, CrouchTransitionSpeed);

	// Update the capsule half-height
	GetCapsuleComponent()->SetCapsuleHalfHeight(NewHalfHeight);

	// Smooth camera height
	FVector CameraLocation = FirstPersonSpringArmComponent->GetRelativeLocation();
	CameraLocation.Z = FMath::FInterpTo(CameraLocation.Z, TargetCapsuleHalfHeight, DeltaTime, CrouchTransitionSpeed);
	FirstPersonSpringArmComponent->SetRelativeLocation(CameraLocation);

	if (GetCharacterMovement()->MovementMode == MOVE_Flying && MantleTimerHandle.IsValid())
	{
		FVector CurrentLoc = GetActorLocation();
		// if final position and current position is same(can't moveable)
		if (FVector::DistSquared(CurrentLoc, LastMantleLocation) < 5.0f)
		{
			StuckTimer += DeltaTime;
			// if stuck more than 0.2 seconds
			if (StuckTimer > 0.2f)
			{
				StopMantle(false); //failed.
			}
		}
		else
		{
			StuckTimer = 0.0f;
		}
		LastMantleLocation = CurrentLoc;
	}

	// Limit Ladder height check
	if (bIsClimbingLadder && CurrentLadder)
	{
		CheckLadderConstraints();
	}

	// Update Light Detector
	UpdateStealthLevel();
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
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::Jump);
		/*EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Ongoing, this, &APlayerCharacter::WhileJumping);*/

		if (MantleAction)
		{
			EnhancedInputComponent->BindAction(MantleAction, ETriggerEvent::Started, this, &APlayerCharacter::StartMantle);
			EnhancedInputComponent->BindAction(MantleAction, ETriggerEvent::Ongoing, this, &APlayerCharacter::OngoingMantle);
			EnhancedInputComponent->BindAction(MantleAction, ETriggerEvent::Completed, this, &APlayerCharacter::EndMantle);
		}

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

		// Weapon Actions
		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &APlayerCharacter::OnAttackPressed);
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnAttackReleased);
		}

		if (ChargeAttackAction)
		{
			EnhancedInputComponent->BindAction(ChargeAttackAction, ETriggerEvent::Started, this, &APlayerCharacter::OnChargeAttackPressed);
			EnhancedInputComponent->BindAction(ChargeAttackAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnChargeAttackReleased);
		}

		if (StealthTakedownAction)
		{
			EnhancedInputComponent->BindAction(StealthTakedownAction, ETriggerEvent::Started, this, &APlayerCharacter::OnStealthTakedownPressed);
		}

		if (EquipWeaponAction)
		{
			EnhancedInputComponent->BindAction(EquipWeaponAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleWeapon);
		}
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// 2D Vector of movement values returned from the input action
	const FVector2D MovementVector = Value.Get<FVector2D>();
	LastMovementInput = MovementVector;

	// Ladder Mode
	if (bIsClimbingLadder && CurrentLadder)
	{
		const FVector LadderUp = CurrentLadder->GetActorUpVector();

		// if press Move forward key: look up move up, look down move down
		// Flying mode Z move is smootly
		AddMovementInput(LadderUp, MovementVector.Y);

		return;
	}

	// Check if the controller posessing this Actor is valid
	if (Controller)
	{
		// Find out which way is forward and right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement input based on direction and input value
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
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

void APlayerCharacter::Jump()
{
	if (bIsClimbingLadder)
	{
		SetLadderMode(false, nullptr); // 1. Ladder mode off

		// 2. Ladder opposite direction jump + upward launch
		if (CurrentLadder)
		{
			// Ladder forward vector
			FVector JumpDir = CurrentLadder->GetActorForwardVector();
			// Reverse direction
			// if you want to jump to the same direction as ladder forward, remove the '-' sign
			// if Player jump to look direction, use GetActorForwardVector()
			JumpDir *= -1.0f;
			LaunchCharacter(FVector(JumpDir.X * 500.0f, JumpDir.Y * 500.0f, 500.0f), false, false);

/*			FVector JumpVelocity = (JumpDir * 500.0f) + FVector(0, 0, 500.0f); // 500 backward, 500 upward
			LaunchCharacter(JumpVelocity, false, false);*/
		}
		return;
	}


	// If crouching, stand up first so mantle checks use standing height
	if (GetCharacterMovement() && GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
		return;
	}
		// Fallback to regular jump
		Super::Jump();
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	bHasMantledThisJump = false;

	GetWorld()->GetTimerManager().ClearTimer(MantleTimerHandle);
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

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, Params))
	{
		AActor* HitActor = HitResult.GetActor();

		// 1. Door
		if (ADoor* Door = Cast<ADoor>(HitActor))
		{
			Door->OnInteract(GetActorForwardVector());
		}

		// 2. Crate
		else if (ACrate* Crate = Cast<ACrate>(HitActor))
		{
			Crate->OnInteract();
		}

		// 3. Loot Item
		else if (ALootItem* Loot = Cast<ALootItem>(HitActor))
		{
			// We pass 'this' (the player) so the item knows who to give money to
			Loot->OnInteract(this);
		}
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

// ----- Mantle System ----

void APlayerCharacter::StartMantle(const FInputActionValue& Value)
{
	if (bIsMantling) return;

	// Can mantle we are in the air (Jump state)
	FVector TargetLocation;

	// Check for Mantle Opportunity
	if (CanMantle(TargetLocation))
	{
		PerformMantle();
	}
}

void APlayerCharacter::OngoingMantle(const FInputActionValue& Value)
{
	if (bIsMantling) return;

	FVector TargetLocation;
	if (CanMantle(TargetLocation))
	{
		PerformMantle();
	}
}

void APlayerCharacter::EndMantle(const FInputActionValue& Value)
{
}

void APlayerCharacter::StopMantle(bool bSuccess)
{
	GetWorld()->GetTimerManager().ClearTimer(MantleTimerHandle);

	bIsMantling = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->GravityScale = 1.0f;
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}

	SetActorEnableCollision(true);

	if (bSuccess)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else
	{
		//Failed or Cancelled: Push back slightly!
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);

		FVector NudgeBack = -GetActorForwardVector() * 5.0f;
		AddActorWorldOffset(NudgeBack, true);
	}
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

void APlayerCharacter::UpdateStealthLevel()
{
	if (LightDetectorInstance)
	{
		// A. Bring in raw brightness value from Light Detector
		float RawBrightness = LightDetectorInstance->GetCurrentBrightness();

		// B. Normalize to 0.0 ~ 1.0 range
		// Pixel value ranges from 0 to 255, but in practice, outdoor lighting can exceed this.
		// Threshold it at 200 for normalization.
		const float MaxBrightnessRef = 200.0f;
		CurrentLightLevel = FMath::Clamp(RawBrightness / MaxBrightnessRef, 0.0f, 1.0f);

		// Debuging: Show current light level on screen
		if(GEngine) GEngine->AddOnScreenDebugMessage(10, 0.1f, FColor::Yellow, FString::Printf(TEXT("Light: %.2f"), CurrentLightLevel));
	}
}

float APlayerCharacter::GetStealthVisibilityFactor() const
{
	float Visibility = CurrentLightLevel;

	if (GetCharacterMovement()->IsCrouching())
	{
		Visibility *= 0.5f;
	}

	if (GetVelocity().SizeSquared() > 10000.0f) // 움직이고 있다면
	{
		Visibility *= 1.2f;
	}

	return FMath::Clamp(Visibility, 0.0f, 1.0f);
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

// ---- Ladder System ----

void APlayerCharacter::SetLadderMode(bool bEnable, ALadder* Ladder)
{
	bIsClimbingLadder = bEnable;
	CurrentLadder = bEnable ? Ladder : nullptr;

	//If try mantling sucess while climbing ladder, deactivate mantle
	if(bIsMantling)
	{
		StopMantle(false);
	}

	if (bIsClimbingLadder)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		GetCharacterMovement()->BrakingDecelerationFlying = 2000.0f;
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterMovement()->BrakingDecelerationFlying = 0.0f;
	}
}

void APlayerCharacter::CheckLadderConstraints() {

	if (!CurrentLadder || !CurrentLadder->GetBoxCollision()) return;

	UBoxComponent* LadderBox = CurrentLadder->GetBoxCollision();

	// --- 1. Horizontal Lock ---
	// Do not allow moving out of the ladder box horizontally

	// Get the player's location in the ladder's local space
	FTransform LadderTransform = CurrentLadder->GetActorTransform();
	FVector LocalLoc = LadderTransform.InverseTransformPosition(GetActorLocation());

	// Yside(width) position clamp
	FVector BoxExtent = LadderBox->GetScaledBoxExtent();
	float CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	float SafeY = FMath::Max(0.0f, BoxExtent.Y - CapsuleRadius);
	LocalLoc.Y = FMath::Clamp(LocalLoc.Y, -SafeY, SafeY);

	// --- 2. Vertical Limits ---

	float MaxZ = BoxExtent.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	if (LocalLoc.Z >= MaxZ && GetCharacterMovement()->Velocity.Z > 0)
	{
		// At the top of the ladder and trying to move up further
		PerformLadderTopClimb();
		return;
	}
	// Clamp the Z position to not go below the ladder base
	if (LocalLoc.Z > MaxZ)
	{
		LocalLoc.Z = MaxZ;
		FVector Velocity = GetCharacterMovement()->Velocity;
		if (Velocity.Z > 0)
		{
			Velocity.Z = 0.f;
			GetCharacterMovement()->Velocity = Velocity;
		}
	}

	if (GetCharacterMovement()->Velocity.Z < 0)
	{
		FVector Start = GetActorLocation();
		FVector End = Start - FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.0f);
		FHitResult Hit;
		FCollisionQueryParams Params;

		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			SetLadderMode(false, nullptr);
			return;
		}
	}

	// --- 3. position ---
	// Convert back to world space and set actor location
	FVector CorrectedWorldLoc = LadderTransform.TransformPosition(LocalLoc);
	SetActorLocation(CorrectedWorldLoc);

	// ---- 4. Face towards ladder ----
	// Get ladder forward vector
	FVector LadderForward = CurrentLadder->GetActorForwardVector();
	FRotator TargetRot = LadderForward.ToOrientationRotator();
	TargetRot.Pitch = 0.f;
	TargetRot.Roll = 0.f;
	SetActorRotation(TargetRot); 
}

// after reaching the top of the ladder, delay before restoring control
void APlayerCharacter::PerformLadderTopClimb()
{
	if (GetWorld()->GetTimerManager().IsTimerActive(LadderFinishTimerHandle)) return;

	ALadder* SavedLadder = CurrentLadder;
	SetLadderMode(false, nullptr);

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->StopMovementImmediately();

	if (SavedLadder)
	{
		FVector LadderForward = SavedLadder->GetActorForwardVector();
		FVector ClimbVelocity = FVector(0, 0, 650.0f) + (-LadderForward * 200.0f);
		GetCharacterMovement()->Velocity = ClimbVelocity;
	}

	float ClimbDuration = 0.5f;
	GetWorld()->GetTimerManager().SetTimer(LadderFinishTimerHandle, this, &APlayerCharacter::FinishLadderClimbSequence, ClimbDuration, false);
}

// Complete it when finish climbing ladder
void APlayerCharacter::FinishLadderClimbSequence()
{
	// 1. Stop any residual movement
	GetCharacterMovement()->StopMovementImmediately();

	// Restore normal movement mode
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// If in the air, set to falling
	if (GetCharacterMovement()->IsFalling())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}
}

void APlayerCharacter::AddMoney(int32 Amount)
{
	CurrentMoney += Amount;
	UE_LOG(LogTemp, Warning, TEXT("Picked Up loot! Current Money: %d"), CurrentMoney);
}

void APlayerCharacter::PerformMantle()
{
	if (!GetCharacterMovement()) return;

	//block multiple activate
	if (bIsMantling) return;
	bIsMantling = true;

	// Set movement mode to flying during mantle
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->Velocity = FVector::ZeroVector;

	// Phase 1: Upper (MantlePos1 near height) & attach into front
	float Duration = MantleDuration;

	// --- Phase 1: climb up ---
	FVector CurrentLoc = GetActorLocation();
	// Setting: slightly upper than ledge
	FVector UpTarget = FVector(CurrentLoc.X, CurrentLoc.Y, MantlePos2.Z); // MantlePos2 phase1 move into Z-height
	FVector MoveDir = (UpTarget - CurrentLoc);
	float Phase1Time = Duration * 0.5f;

	GetCharacterMovement()->Velocity = MoveDir / Phase1Time;

	// Phase 2 prepare
	GetWorld()->GetTimerManager().SetTimer(MantleTimerHandle, [this, Phase1Time]()
	{
		if (!bCanMantle)
		{
			StopMantle(false);
			return;
		}

		// --- Phase 2: move Forward ---
		FVector CurrentLoc = GetActorLocation();
		// considering Z side as correct, move forward XY flat
		FVector ForwardTarget = MantlePos2;
		FVector MoveDir = (ForwardTarget - CurrentLoc);

		GetCharacterMovement()->Velocity = MoveDir / Phase1Time;

		// End
		FTimerHandle EndTimer;
		GetWorld()->GetTimerManager().SetTimer(EndTimer, [this]()
		{
			CompleteMantleSequence();
		}, Phase1Time, false);

	}, Phase1Time, false);
}

void APlayerCharacter::CompleteMantleSequence()
{
	if (!GetCharacterMovement()) return;

	// 1. final location force locate.
	SetActorLocation(MantlePos2);

	// 2. Restore Movement
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	GetCharacterMovement()->GravityScale = 1.0f;

	// 3. Reset Flags
	bCanMantle = false;
	bHasMantledThisJump = false;

	bIsMantling = false;
}

// -------- Mantling --------
bool APlayerCharacter::CanMantle(FVector& OutMantleTargetLocation)
{
	if (!GetCharacterMovement() || !GetWorld()) return false;

	// if do Mantle already while jump/falling
	if (bHasMantledThisJump) return false;

	// Player information
	float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector PlayerLoc = GetActorLocation();
	FVector ForwardDir = GetActorForwardVector();

	// feet location
	float FeetZ = PlayerLoc.Z - CapsuleHalfHeight;

	float MaxReachZ = FeetZ + (CapsuleHalfHeight * 2.0f) + 10.0f;

	// 1. TRACE FORWARD (Find the Wall)
	FVector WallTraceStart = PlayerLoc;
	FVector WallTraceEnd = WallTraceStart + (ForwardDir * MantleTraceDistance); // Uses your InitialTraceLength

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FHitResult WallHit;
	// Trace against WorldStatic (Walls)
	bool bHitWall = GetWorld()->LineTraceSingleByChannel(WallHit, WallTraceStart, WallTraceEnd, ECC_WorldStatic, Params);

	if (!bHitWall) return false;

	// Move the trace slightly into the wall (15 units) + Up to the limit
	FVector WallNormal = WallHit.ImpactNormal; // Direction pointing into the wall
	FVector WallInDir = -WallNormal;

	FVector LedgeTraceStart = WallHit.ImpactPoint + (WallInDir * 30.0f);
	LedgeTraceStart.Z = MaxReachZ; // Trace down to the wall impact height

	FVector LedgeTraceEnd = LedgeTraceStart;
	LedgeTraceEnd.Z = WallHit.ImpactPoint.Z;

	FHitResult LedgeHit;
	bool bHitLedge = GetWorld()->LineTraceSingleByChannel(LedgeHit, LedgeTraceStart, LedgeTraceEnd, ECC_WorldStatic, Params);

	// [Debug] Visualize the Down Trace
	DrawDebugLine(GetWorld(), LedgeTraceStart, LedgeTraceEnd, bHitLedge ? FColor::Green : FColor::Red, false, 2.0f);

	if (!bHitLedge) return false; // there is no landing spot
	if (LedgeHit.bStartPenetrating) return false; // start inside the wall

	// --- 4. Check the works ---

	// A. check flatness : is that flat?
	if (LedgeHit.ImpactNormal.Z < GetCharacterMovement()->GetWalkableFloorZ())
	{
		return false; // too rough
	}

	// B. check height: is this higher than climbheight?
	float ClimbHeight = LedgeHit.ImpactPoint.Z - FeetZ;
	const float MinClimbHeight = 40.0f; // works at least higher than knee

	if (ClimbHeight < MinClimbHeight)
	{
		return false;
	}

	// C. check space : is there space for climb (cehck capsule height)
	FVector TargetLocation = LedgeHit.ImpactPoint + FVector(0, 0, CapsuleHalfHeight + 5.0f);
	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);

	bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
		TargetLocation, FQuat::Identity, ECC_Pawn, CapsuleShape, Params
	);

	if (bBlocked) return false;

	// --- 5. Target setting---
	// MantlePos1: hanging position(in front of wall, slightly up)
	MantlePos1 = LedgeHit.ImpactPoint + (WallNormal * (CapsuleRadius + 10.0f)) + FVector(0, 0, -50.0f);

	// MantlePos2: final landing ( upper ledge, inside wall infront)
	MantlePos2 = TargetLocation + (WallInDir * 20.0f);

	OutMantleTargetLocation = MantlePos2;
	bCanMantle = true;

	return true;
}

// --- Weapon System ---
void APlayerCharacter::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!WeaponToEquip)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempted to equip a null weapon"));
		return;
	}

	if (CurrentWeapon)
	{
		UnequipWeapon();
	}

	CurrentWeapon = WeaponToEquip;
	CurrentWeapon->OnEquip(this);

	UE_LOG(LogTemp, Log, TEXT("EQuipped weapon: %s"), *CurrentWeapon->GetName());
}

void APlayerCharacter::UnequipWeapon()
{
	if(!CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->OnUnequip();
	CurrentWeapon = nullptr;

	UE_LOG(LogTemp, Log, TEXT("Weapon unequipped"));
}

void APlayerCharacter::ToggleWeapon()
{
	if (CurrentWeapon)
	{
		UnequipWeapon();

		if (DefaultWeapon)
		{
			DefaultWeapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				TEXT("HolsterSocket")
			);
		}
	}
	else if (DefaultWeapon)
	{
		EquipWeapon(DefaultWeapon);
	}
}

void APlayerCharacter::OnAttackPressed()
{
	if (bIsClimbingLadder || bIsMantling) return;
	if (CurrentWeapon && CurrentWeapon->CanAttack())
	{
		CurrentWeapon->StartAttack();
	}
}

void APlayerCharacter::OnAttackReleased()
{
	// Currently not needed, but useful for future combo systems
}

void APlayerCharacter::OnChargeAttackPressed()
{
	if (bIsClimbingLadder || bIsMantling)
	{
		return;
	}
}

void APlayerCharacter::OnChargeAttackReleased()
{
	if (bIsClimbingLadder || bIsMantling)
	{
		return;
	}
}

void APlayerCharacter::OnStealthTakedownPressed()
{
	if (bIsClimbingLadder || bIsMantling)
	{
		return;
	}
}
