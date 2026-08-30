


#include "PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Engine/Engine.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Player Collision Capsule init
	GetCapsuleComponent()->InitCapsuleSize(48.0f, 96.0f);

	// Player First Person Mesh init
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Player First Person Camera init
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	
	// Change it after getting 3D model
	//FirstPersonCamera->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	
	// Change it after getting 3D model
	//FirstPersonCamera->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->bEnableFirstPersonFieldOfView = true;
	FirstPersonCamera->bEnableFirstPersonScale = true;
	FirstPersonCamera->FirstPersonFieldOfView = 70.0f;
	FirstPersonCamera->FirstPersonScale = 0.6f;

	// Configure Player Components
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(IMC_PlayerContext, 0);
		}
	}

	CurrentStamina = MaxStamina;

	// Initialize Player movement speed
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;

	// Initialize Player jumping
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.6f;

	// Initialize Player smooth crouching
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	CrouchEyeOffset = FVector(0.0f);
}

void APlayerCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth crouch parameters
	float CrouchInterpTime = FMath::Min(1.0f, CrouchCameraSpeed * DeltaTime);
	CrouchEyeOffset = (1.0f - CrouchInterpTime) * CrouchEyeOffset;

	// Smooth FOV effect
	float TargetFOV = bIsSprinting && !bIsExhausted && GetCharacterMovement()->Velocity.Size2D() > WalkSpeed ? PlayerSprintFOV : PlayerWalkFOV;
	float SmoothFOV = FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaTime, 5.0f);
	FirstPersonCamera->SetFieldOfView(SmoothFOV);

	// Sprinting stamina drain logic
	if (bIsSprinting && !bIsExhausted && GetCharacterMovement()->Velocity.Size2D() > WalkSpeed)
	{
		CurrentStamina -= StaminaDrainRate * DeltaTime;

		if (CurrentStamina <= 0.0f)
		{
			CurrentStamina = 0.0f;
			bIsExhausted = true;
			bIsSprinting = false;
			OnStaminaDepleted();
			DoSprintEnd();
		}
	}

	// Sprinting stamina recovery logic
	if (CurrentStamina < MaxStamina)
	{
		CurrentStamina += StaminaRecoveryRate * DeltaTime;
		if (CurrentStamina >= MaxStamina && bIsExhausted)
		{
			bIsExhausted = false;
			OnStaminaRecovered();
		}
	}

	// OnGame debug string
	if (GEngine && GetCharacterMovement())
	{
		// Current Player state
		FString State = GetIsSprinting() ? TEXT("Sprint") : UEnum::GetValueAsString(GetCharacterMovement()->MovementMode);

		// Current Player speed
		float Speed = GetCharacterMovement()->Velocity.Size2D();

		// Current state of Exhaustion
		FString Exhausted = bIsExhausted ? TEXT("True") : TEXT("False");

		// Debug string
		FString DebugMsg = FString::Printf(
			TEXT("State: %s | Speed: %.1f | Current Stamina: %.1f | Exhausted: %s"),
			*State, 
			Speed,
			GetCurrentStamina(),
			*Exhausted
		);

		// Screen output
		GEngine->AddOnScreenDebugMessage(1, 5.0f, FColor::Red, DebugMsg);
	}
}


// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Movement: WASD - press & hold
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &APlayerCharacter::MoveAction);

		// Looking: MOUSE - moving
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &APlayerCharacter::LookAction);

		// Crouching: C (or LCtrl) - press & hold
		EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &APlayerCharacter::DoCrouchStart);
		EIC->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &APlayerCharacter::DoCrouchEnd);

		// Jumping: SpaceBar - press
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &APlayerCharacter::DoJumpStart);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &APlayerCharacter::DoJumpEnd);
	
		// Sprinting: LShift - press & hold
		EIC->BindAction(IA_Sprint, ETriggerEvent::Triggered, this, &APlayerCharacter::DoSprintStart);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &APlayerCharacter::DoSprintEnd);
	}
}

// Player movement logic
void APlayerCharacter::MoveAction(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		// Get a 2D vector (x = forward/backward, y = right/left)
		FVector2D MovementVector = Value.Get<FVector2D>();

		// Get Forward and Right direction
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
		// Adding movement Forward or Right
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);

	}
}

// Player looking logic
void APlayerCharacter::LookAction(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y * (-1));
}

// Player crouch logic
void APlayerCharacter::DoCrouchStart()
{
	Crouch();
}

// Player smooth croucing logic #1
void APlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (HalfHeightAdjust == 0.0f)
	{
		return;
	}

	float StartBaseEyeHeight = BaseEyeHeight;
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	CrouchEyeOffset.Z += StartBaseEyeHeight - BaseEyeHeight + HalfHeightAdjust;
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight), false);
}

// Player smooth croucing logic #2
void APlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (HalfHeightAdjust == 0.0f)
	{
		return;
	}

	float StartBaseEyeHeight = BaseEyeHeight;
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	CrouchEyeOffset.Z += StartBaseEyeHeight - BaseEyeHeight - HalfHeightAdjust;
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight), false);
}

// Player smooth croucing logic #3
void APlayerCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	if (FirstPersonCamera)
	{
		FirstPersonCamera->GetCameraView(DeltaTime, OutResult);
		OutResult.Location += CrouchEyeOffset;
	}
}

// Player smooth croucing logic #4
void APlayerCharacter::DoCrouchEnd()
{
	UnCrouch();
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
}

// Player jump logic
void APlayerCharacter::DoJumpStart()
{
	Jump();
}

// Player jump logic
void APlayerCharacter::DoJumpEnd()
{
	StopJumping();
}

// Player sprint logic
void APlayerCharacter::DoSprintStart()
{
	bIsSprinting = true;

	if (!bIsExhausted)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

		OnSprintStateChanged.Broadcast(true);
	}
}

// Player sprint logic
void APlayerCharacter::DoSprintEnd()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = bIsCrouched ? CrouchSpeed : WalkSpeed;
	OnSprintStateChanged.Broadcast(false);
}