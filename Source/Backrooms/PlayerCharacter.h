

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSprintStateChangedDelegate, bool, bIsSprinting);

UCLASS()
class BACKROOMS_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

	USkeletalMeshComponent* FirstPersonMesh;

	UCameraComponent* FirstPersonCamera;

// Player stat section
protected:

	// The amount of maximum HEALTH a player can have
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Stats|Health")
		float MaxHealth = 100.0f;

	// Current player HEALTH
	float CurrentHealth = 0.0f;

	// The amount of maximum STAMINA a player can have
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Stats|Stamina")
		float MaxStamina = 100.0f;

	// Current player STAMINA
	float CurrentStamina = 0.0f;

	// STAMINA recovery rate: if bIsSprinting != true
	float StaminaRecoveryRate = 10.0f;

	// STAMINA drain rate: while bIsSprinting = true
	float StaminaDrainRate = 25.0f;

	// Sprinting flags
	bool bIsSprinting = false;

	// Exhausted flags
	bool bIsExhausted = false;

	// The amount of maximum SANITY a player can have
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Stats|Sanity")
		float MaxSanity = 100.0;

	// Current player SANITY
	float CurrentSanity = 0.0f;

// Player movement stat section
protected:

	// Player movement speed while crouching
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Movement|Crouch")
		float CrouchSpeed = 150.0f;

	// Player movement speed while walking
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Movement|Walk")
		float WalkSpeed = 300.0f;

	// Player movement speed while sprinting
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Movement|Sprint")
		float SprintSpeed = 600.0f;

// Player Camera Effect properties section
protected:

	// Crouching eye offset
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player Movement|Crouch")
	FVector CrouchEyeOffset;

	// Crouching camera offset speed
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player Movement|Crouch")
	float CrouchCameraSpeed = 4.0f;

	// Player default FOV
	float PlayerWalkFOV = 90.0f;

	// Player sprinting FOV
	float PlayerSprintFOV = 100.0f;

// Player Input Action section
protected:

	// Default Player Mapping Context
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
		class UInputMappingContext* IMC_PlayerContext;

	// Player MOVE input action: WASD - press & hold
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
		class UInputAction* IA_Move;

	// Player MOVE action
	void MoveAction(const FInputActionValue& Value);

	// Player LOOK input action: MOUSE - moving
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
		class UInputAction* IA_Look;

	// Player LOOK action
	void LookAction(const FInputActionValue& Value);

	// Player JUMP input action: Space - pressed
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
		class UInputAction* IA_Jump;

	// Player JUMP action: start jump
	void DoJumpStart();

	// Player JUMP action: end jump
	void DoJumpEnd();

	// Player CROUCH input action: Ctrl - press & hold
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
		class UInputAction* IA_Crouch;

	// Player CROUCH action: start crouching
	void DoCrouchStart();

	void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult) override;

	// Player CROUCH action: end jump
	void DoCrouchEnd();

	// Player SPRINT input action: LeftShift - press & hold
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
		class UInputAction* IA_Sprint;

	// Player SPRINT action: start sprinting
	void DoSprintStart();

	// Player SPRINT action: end sprinting
	void DoSprintEnd();

public:
	// Constructor
	APlayerCharacter();

	float GetIsSprinting() { return bIsSprinting; }
	float GetCurrentStamina() { return CurrentStamina; }
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	// Event for BP - Stamina ran out
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnStaminaDepleted();

	// Event for BP - Stamina recovered
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnStaminaRecovered();


	// Delegate called when we start and stop sprinting
	FSprintStateChangedDelegate OnSprintStateChanged;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Gameplay cleanup
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;


	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
