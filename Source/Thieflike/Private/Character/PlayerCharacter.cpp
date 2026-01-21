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

	// Limit Ladder height check
	if (bIsClimbingLadder && CurrentLadder)
	{
		CheckLadderConstraints();
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
	const FVector2D MovementVector = Value.Get<FVector2D>();
	LastMovementInput = MovementVector;

	// Ladder Mode
	if (bIsClimbingLadder)
	{
		if (Controller)
		{
			const FRotator ControlRotation = Controller->GetControlRotation();
			const FVector LookDirection = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::X);

			// if press Move forward key: look up move up, look down move down
			// Flying mode Z move is smootly
			AddMovementInput(LookDirection, MovementVector.Y);
		}
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
	}
	else
	{
		// Fallback to regular jump
		Super::Jump();
	}
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

	bHasMantledThisJump = false;

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

void APlayerCharacter::SetLadderMode(bool bEnable, ALadder* Ladder)
{
	bIsClimbingLadder = bEnable;
	CurrentLadder = bEnable ? Ladder : nullptr;

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

void APlayerCharacter::CheckLadderConstraints()
{
	if (!CurrentLadder) return;

	float CurrentZ = GetActorLocation().Z;
	float MaxZ = CurrentLadder->GetLadderMaxZ() - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector Velocity = GetCharacterMovement()->Velocity;

	// reach highest location move up speed terminate
	if (CurrentZ >= MaxZ && Velocity.Z > 0)
	{
		Velocity.Z = 0.0f; // stop immediately
		GetCharacterMovement()->Velocity = Velocity;
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

	// Set movement mode to flying during mantle
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->Velocity = FVector::ZeroVector;

	// Phase 1: Upper (MantlePos1 near height) & attach into front
	float Duration = MantleDuration;

	FTimerDelegate TimerDel;
	TimerDel.BindLambda([this, Duration]()
		{});

	// --- Phase 1: climb up ---
	FVector CurrentLoc = GetActorLocation();
	// Setting: slightly upper than ledge
	FVector UpTarget = FVector(CurrentLoc.X, CurrentLoc.Y, MantlePos2.Z);
	FVector MoveDir = (UpTarget - CurrentLoc);
	float Phase1Time = Duration * 0.5f;

	GetCharacterMovement()->Velocity = MoveDir / Phase1Time;

	// Phase 2 prepare
	GetWorld()->GetTimerManager().SetTimer(MantleTimerHandle, [this, Phase1Time]()
		{
			if (!bCanMantle) return;

			// --- Phase 2: move infront ---
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

	// 2. restore
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	GetCharacterMovement()->GravityScale = 1.0f;

	// 3. being false
	bCanMantle = false;
	bHasMantledThisJump = false;
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