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
#include "Character/LightDetector.h" // LightDetector

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

	// Child Actor for lightDetector
	LightDetectorComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("LightDetectorComponent"));
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

	if (LightDetectorComponent)
	{
		AActor* ChildActor = LightDetectorComponent->GetChildActor();
		LightDetectorInstance = Cast<ALightDetector>(ChildActor);

		if (!LightDetectorInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("LightDetector class is not inherit or failed to cast."));
		}
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateStealthLevel();

	if (!FirstPersonSpringArmComponent && !FirstPersonCameraComponent)
	{
		return;
	}

	if (bIsClimbingLadder)
	{
		UpdateLadderMovement(DeltaTime);
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
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Ongoing, this, &APlayerCharacter::WhileJumping);

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

void APlayerCharacter::UpdateStealthLevel()
{
	if (LightDetectorInstance)
	{
		float CurrentBrightness = LightDetectorInstance->CalculateBrightness();

		// (Debug) Output into screen.
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Light: %f"), CurrentBrightness));
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// 2D Vector of movement values returned from the input action
	const FVector2D MovementValue = Value.Get<FVector2D>();
	LastMovementInput = MovementValue;

	if (bIsClimbingLadder)
	{
		return; // Don't process normal walking movement
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

void APlayerCharacter::Jump()
{
	// If crouching, stand up first so mantle checks use standing height
	if (GetCharacterMovement() && GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
		return;
	}

	// Can mantle we are in the air (Jump state)
	FVector TargetLocation;

	// Check for Mantle Opportunity
	if (!bHasMantledThisJump && CanMantle(TargetLocation))
	{
		// Mantle trace successful
		bHasMantledThisJump = true; // Mark that we've used our mantle for this jump
		PerformMantle();
		return;
	}
	// Fallback to regular jump
	Super::Jump();
}

void APlayerCharacter::WhileJumping()
{
	if (!bCanMantle)
	{
		return;
	}

	PerformMantle();
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// if Landed
	bHasMantledThisJump = false;

	// Reset Timer if falling while Mantle
	GetWorld()->GetTimerManager().ClearTimer(MantleTimerHandle);
}


void APlayerCharacter::StartCrouch(const FInputActionValue& Value)
{
	// Toggle Crouch
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
		// If mantling 

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

		// 4. Ladder
		if (ALadder* Ladder = Cast<ALadder>(HitActor))
		{
			if (bIsClimbingLadder)
			{
				StopClimbLadder();
			}
			else
			{
				StartClimbLadder(Ladder);
			}
			return;
		}
	}

	else if (bIsClimbingLadder)
	{
		StopClimbLadder();
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

void APlayerCharacter::StopMantle(bool bSuccess)
{
	GetWorld()->GetTimerManager().ClearTimer(MantleTimerHandle);

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

// Calculate the player's visibility based on lighting conditions
void APlayerCharacter::CalculateVisibility()
{
	//Determine target visibility percentage (0 to 100)
	float TargetVisibilityPercent = AmbientLightFactor * 100.0f;

	if (LightDetector)
	{
		//LightDetector returns brightness (0 ~ 255). regularitise 0 ~ 1.
		float Brightness = LightDetector->CalculateBrightness();
		float Normalized = FMath::Clamp(Brightness / 255.0f, 0.0f, 1.0f);

		// AmbientLightFactor Normlized - If Normalized is 0 then being Ambient, otherwise, 1 being exposure
		float Exposure = FMath::Lerp(AmbientLightFactor, 1.0f, Normalized);
		TargetVisibilityPercent = Exposure * 100.0f;
	}

	// Smoothly
	if (GetWorld())
	{
		CurrentVisibility = FMath::FInterpTo(CurrentVisibility, TargetVisibilityPercent, GetWorld()->GetDeltaSeconds(), VisibilityInterpSpeed);
	}
	else
	{
		CurrentVisibility = TargetVisibilityPercent;
	}

	// limited safety
	CurrentVisibility = FMath::Clamp(CurrentVisibility, 0.0f, 100.0f);
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

void APlayerCharacter::StartClimbLadder(ALadder* Ladder)
{
	if (!Ladder || bIsClimbingLadder)
	{
		return;
	}

	CurrentLadder = Ladder;
	bIsClimbingLadder = true;


	// Disable normal walking/falling
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	// Stop current movement
	GetCharacterMovement()->StopMovementImmediately();

	UBoxComponent* BoxCollision = Ladder->BoxCollision;
	FVector LadderCenter = BoxCollision->GetComponentLocation();
	FVector PlayerLocation = GetActorLocation();

	// Target Location: Ladder's X/Y, Player's Z
	FVector TargetLoc = FVector(LadderCenter.X, LadderCenter.Y, PlayerLocation.Z);

	// Offset slightly forward from the ladder so we don't clip inside the mesh
	FVector ForwardOffset = Ladder->GetActorForwardVector() * 40.0f;
	TargetLoc += ForwardOffset;

	SetActorLocation(TargetLoc);

	// 3. Snap Rotation (Face the ladder)
	FRotator TargetRot = Ladder->GetActorRotation();
	TargetRot.Yaw += 180.0f; // Face the ladder
	SetActorRotation(TargetRot);

	// --- Set Ladder Max and Min
	float LadderMinZ = Ladder->GetLadderMinZ();
	float LadderMaxZ = Ladder->GetLadderMaxZ();
	LadderTargetZ = FMath::Clamp(GetActorLocation().Z, LadderMinZ, LadderMaxZ);

	UE_LOG(LogTemp, Warning, TEXT("Started climbing ladder at Z: %f (Min: %f, Max: %f)"), LadderTargetZ, LadderMinZ, LadderMaxZ);
}

void APlayerCharacter::StopClimbLadder()
{
	if (!bIsClimbingLadder)
	{
		return;
	}

	bIsClimbingLadder = false;
	CurrentLadder = nullptr;

	// Return to normal movement
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);

	/*const float JumpOffForce = 300.0f;
	FVector BackwardDirection = -GetActorForwardVector();

	LaunchCharacter(BackwardDirection * JumpOffForce, true, true);*/

	UE_LOG(LogTemp, Warning, TEXT("Stopped climbing ladder"));
}

void APlayerCharacter::UpdateLadderMovement(float DeltaTime)
{
	if (!CurrentLadder || !GetCharacterMovement())
	{
		StopClimbLadder();
		return;
	}

	FVector PlayerLocation = GetActorLocation();
	UBoxComponent* BoxCollision = CurrentLadder->BoxCollision;
	FVector LadderLocation = BoxCollision->GetComponentLocation();
	FVector BoxExtent = BoxCollision->GetScaledBoxExtent();

	// --- Ladder : Min and Max ---
	float LadderMinZ = CurrentLadder->GetLadderMinZ();
	float LadderMaxZ = CurrentLadder->GetLadderMaxZ();

	// ---- Check if player is still within ladder bounds ----
	float DistX = FMath::Abs(PlayerLocation.X - LadderLocation.X);
	float DistY = FMath::Abs(PlayerLocation.Y - LadderLocation.Y);

/*	// If player moves outside the box collision, stop climbing
	if (DistX > BoxExtent.X || DistY > BoxExtent.Y)
	{
		UE_LOG(LogTemp, Warning, TEXT("Out of ladder bounds (X/Y)! Falling..."));
		StopClimbLadder();
		return;
	}*/

	// LstMovementInput.Y works into Move function(W/S)
	float ClimbInput = LastMovementInput.Y;

	if (FMath::Abs(ClimbInput) > 0.1f)
	{
		LadderTargetZ += ClimbInput * LadderClimbSpeed * DeltaTime;
	}

	// Clamp target Z within ladder bounds
	LadderTargetZ = FMath::Clamp(LadderTargetZ, LadderMinZ, LadderMaxZ);

	// Move player to stay aligned with ladder (within box bounds)
	float ClampedX = FMath::Clamp(PlayerLocation.X, LadderLocation.X - BoxExtent.X, LadderLocation.X + BoxExtent.X);
	float ClampedY = FMath::Clamp(PlayerLocation.Y, LadderLocation.Y - BoxExtent.Y, LadderLocation.Y + BoxExtent.Y);

	FVector NewLocation = FVector(ClampedX, ClampedY, LadderTargetZ);

	// Set movement velocity to reach target position
	FVector DeltaPosition = NewLocation - PlayerLocation;

	if (FVector::Dist(PlayerLocation, NewLocation) < 1.0f)
	{
		SetActorLocation(NewLocation);
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}
	else
	{
		GetCharacterMovement()->Velocity = DeltaPosition / DeltaTime;
	}

	// Debug visualization
	DrawDebugBox(
		GetWorld(),
		LadderLocation,
		BoxExtent,
		FColor::Green,
		false,
		0.0f,
		1
	);

	// Debug visualisation
	DrawDebugLine(
		GetWorld(),
		LadderLocation + FVector(-BoxExtent.X, 0, 0),
		LadderLocation + FVector(BoxExtent.X, 0, 0),
		FColor::Yellow,
		false,
		0.0f,
		2
	);

	DrawDebugLine(
		GetWorld(),
		FVector(LadderLocation.X - BoxExtent.X, LadderLocation.Y, LadderMaxZ),
		FVector(LadderLocation.X + BoxExtent.X, LadderLocation.Y, LadderMaxZ),
		FColor::Cyan,
		false,
		0.0f,
		2
	);

	// Draw current player position on ladder
	DrawDebugSphere(
		GetWorld(),
		PlayerLocation,
		10.0f,
		12,
		FColor::Red,
		false,
		0.0f,
		1
	);

	// Check if player wants to exit the ladder (Jump or Interact)
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC && (PC->WasInputKeyJustPressed(EKeys::SpaceBar) || PC->WasInputKeyJustPressed(EKeys::E)))
	{
		// Exit ladder forward
		FVector ExitDirection = GetActorForwardVector() * LadderExitDistance;
		AddActorWorldOffset(ExitDirection);
		StopClimbLadder();
	}
}


FVector APlayerCharacter::GetNearestPointOnLadder(ALadder* Ladder) const
{
	if (!Ladder || !Ladder->BoxCollision)
	{
		return GetActorLocation();
	}

	// Get the ladder's box collision component
	UBoxComponent* BoxCollision = Ladder->BoxCollision;
	FVector LadderLocation = BoxCollision->GetComponentLocation();
	FVector PlayerLocation = GetActorLocation();

	// Clamp player position to ladder bounds
	FVector BoxExtent = BoxCollision->GetScaledBoxExtent();

	float ClampedX = FMath::Clamp(PlayerLocation.X, LadderLocation.X - BoxExtent.X, LadderLocation.X + BoxExtent.X);
	float ClampedY = FMath::Clamp(PlayerLocation.Y, LadderLocation.Y - BoxExtent.Y, LadderLocation.Y + BoxExtent.Y);
	float ClampedZ = FMath::Clamp(PlayerLocation.Z, LadderLocation.Z - BoxExtent.Z, LadderLocation.Z + BoxExtent.Z);

	return FVector(ClampedX, ClampedY, ClampedZ);
}

void APlayerCharacter::AddMoney(int32 Amount)
{
	CurrentMoney += Amount;
	UE_LOG(LogTemp, Warning, TEXT("Picked Up loot! Current Money: %d"), CurrentMoney);
}

void APlayerCharacter::PerformMantle()
{
	if (!GetCharacterMovement())
	{
		return;
	}

	// Set movement mode to flying during mantle
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->GravityScale = 0.0f; // Disable gravity during mantle
	GetCharacterMovement()->Velocity = FVector::ZeroVector;

	// Store last mantle location for safety checks
	LastMantleLocation = GetActorLocation();
	StuckTimer = 0.0f;

	// Calculate movement for Phase 1: Move to MantlePos1 (up and slightly back)
	FVector CurrentPos = GetActorLocation();
	FVector ToPos1 = MantlePos1 - CurrentPos;
	float Phase1Distance = ToPos1.Length();
	float Phase1Duration = MontageLength * 0.4f;

	FVector Phase1Velocity = (Phase1Distance > 0.0f) ? (ToPos1 / Phase1Duration) : FVector::ZeroVector;
	GetCharacterMovement()->Velocity = Phase1Velocity;

	// Draw debug line for movement direction
	DrawDebugLine(
		GetWorld(),
		CurrentPos,
		MantlePos1,
		FColor::Yellow,
		false,
		Phase1Duration,
		0,
		2.0f
	);

	// Schedule Phase 2: Move to MantlePos2 (continue forward and maintain height)
	GetWorld()->GetTimerManager().SetTimer(
		MantleTimerHandle,
		[this]()
		{
			if (!GetCharacterMovement() || !bCanMantle)
			{
				return;
			}

			FVector CurrentPos = GetActorLocation() + 5.0f;
			FVector ToPos2 = MantlePos2 - CurrentPos;
			float Phase2Distance = ToPos2.Length();
			float Phase2Duration = MontageLength * 0.6f;

			FVector Phase2Velocity = (Phase2Distance > 0.0f) ? (ToPos2 / Phase2Duration) : FVector::ZeroVector;
			GetCharacterMovement()->Velocity = Phase2Velocity;

			// Draw debug line for Phase 2 movement
			DrawDebugLine(
				GetWorld(),
				CurrentPos,
				MantlePos2,
				FColor::Green,
				false,
				Phase2Duration,
				0,
				2.0f
			);

			// Complete mantle sequence after Phase 2
			GetWorld()->GetTimerManager().SetTimer(
				MantleTimerHandle,
				[this]()
				{
					CompleteMantleSequence();
				},
				MontageLength * 0.6f,
				false
			);
		},
		Phase1Duration,
		false
	);
}

void APlayerCharacter::CompleteMantleSequence()
{
	if (!GetCharacterMovement())
	{
		return;
	}

	SetActorLocation(MantlePos2);

	// Stop all movement
	GetCharacterMovement()->Velocity = FVector::ZeroVector;

	// Re-enable gravity
	GetCharacterMovement()->GravityScale = 1.0f;

	// Re-enable collision after mantle
	SetActorEnableCollision(true);

	// Return to walking movement mode
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	bCanMantle = false;

	bHasMantledThisJump = false;
}

// -------- Mantling --------
bool APlayerCharacter::CanMantle(FVector& OutMantleTargetLocation)
{
	if (!GetCharacterMovement() || !GetWorld()) return false;

	// if do Mantle already while jump/falling
	if (bHasMantledThisJump)
	{
		return false;
	}

	// --- SETUP: Get Player Dimensions ---
	float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float FullHeight = CapsuleHalfHeight * 2.0f;
	FVector PlayerLoc = GetActorLocation();
	FVector FeetLoc = PlayerLoc - FVector(0, 0, CapsuleHalfHeight);
	FVector Forward = GetActorForwardVector();

	// 1. TRACE FORWARD (Find the Wall)
	FVector WallTraceStart = PlayerLoc;
	FVector WallTraceEnd = WallTraceStart + (Forward * InitialTraceLength); // Uses your InitialTraceLength

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FHitResult WallHit;
	// Trace against WorldStatic (Walls)
	bool bHitWall = GetWorld()->LineTraceSingleByChannel(WallHit, WallTraceStart, WallTraceEnd, ECC_WorldStatic, Params);

	if (!bHitWall) return false;

	// 2. DEFINE HEIGHT LIMIT (Thief Style: Height + 50.0f)
	// We start the "Down Trace" exactly at this limit.
	// If the wall is taller than this, the start point will be inside the wall, and the trace will fail.
	float MaxReachZ = FeetLoc.Z + MaxMantleHeight;

	// 3. TRACE DOWN (Find the Ledge Top)
	// Move the trace slightly into the wall (15 units) + Up to the limit
	FVector WallForwardDir = -WallHit.ImpactNormal; // Direction pointing into the wall
	FVector LedgeTraceStart = WallHit.ImpactPoint + (WallForwardDir * 15.0f);
	LedgeTraceStart.Z = MaxReachZ; // Strict height limit

	FVector LedgeTraceEnd = LedgeTraceStart;
	LedgeTraceEnd.Z = WallHit.ImpactPoint.Z; // Trace down to the wall impact height

	FHitResult LedgeHit;
	bool bHitLedge = GetWorld()->LineTraceSingleByChannel(LedgeHit, LedgeTraceStart, LedgeTraceEnd, ECC_WorldStatic, Params);

	// [Debug] Visualize the Down Trace
	DrawDebugLine(GetWorld(), LedgeTraceStart, LedgeTraceEnd, bHitLedge ? FColor::Green : FColor::Red, false, 2.0f, 0, 1.0f);

	if (bHitLedge)
	{
		if (LedgeHit.bStartPenetrating)
		{
			return false;
		}
		// 4. VALIDATE SURFACE
		// Is the surface flat enough to stand on?
		if (LedgeHit.ImpactPoint.Z < GetCharacterMovement()->GetWalkableFloorZ())
		{
			return false;
		}
		// ====================================================
		// [Add] 4-A. HEIGHT CHECK
		// ====================================================
		float LedgeZ = LedgeHit.ImpactPoint.Z;
		float FeetZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		// Ledge height - Feet height = Climb Height
		float ClimbHeight = LedgeZ - FeetZ;

		UE_LOG(LogTemp, Log, TEXT("Climb Height: %f / Max: %f"), ClimbHeight, MaxMantleHeight);

		// If ClimbHeight is higher than MaxMantleHeight failed
		if (LedgeHit.ImpactNormal.Z < GetCharacterMovement()->GetWalkableFloorZ())
        {
            return false;
        }

		// 5. CALCULATE TARGET POSITIONS
		FVector LedgeTop = LedgeHit.ImpactPoint;

		// MantlePos2 (Final Landing Spot): On top of the ledge, slightly forward from the edge
		// We add CapsuleHalfHeight to Z so the feet land on the surface
		MantlePos2 = LedgeTop + FVector(0, 0, CapsuleHalfHeight + 2.0f) + (Forward * 20.0f);

		// MantlePos1 (Climb Phase): Just below the edge, ready to hoist
		// Offset slightly back from the ledge edge and down
		MantlePos1 = LedgeTop + (WallHit.ImpactNormal * 30.0f) + FVector(0, 0, -10.0f);

		OutMantleTargetLocation = MantlePos2;
		bCanMantle = true;

		// [Debug] Draw Spheres
		DrawDebugSphere(GetWorld(), MantlePos1, 10.0f, 12, FColor::Cyan, false, 2.0f);
		DrawDebugSphere(GetWorld(), MantlePos2, 10.0f, 12, FColor::Magenta, false, 2.0f);

		return true;
	}

	bCanMantle = false;
	return false;
}