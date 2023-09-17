﻿// Created by Henry Jooste

#include "USKCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "USK/Audio/AudioUtils.h"
#include "USK/Logger/Log.h"

/**
 * @brief Create a new instance of the AUSKCharacter actor
 */
AUSKCharacter::AUSKCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));

	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

/**
 * @brief Overridable native event for when play begins for this actor
 */
void AUSKCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	USK_LOG_TRACE("Initializing character movement");	
	GetCharacterMovement()->JumpZVelocity = JumpVelocity;
	GetCharacterMovement()->AirControl = AirControl;
	GetCharacterMovement()->FallingLateralFriction = FallingFriction;
	GetCharacterMovement()->GravityScale = Gravity;
	GetCharacterMovement()->BrakingFriction = BrakingFriction;
	GetCharacterMovement()->MaxAcceleration = MaxAcceleration;

	USK_LOG_TRACE("Initializing character jumping");
	JumpMaxCount = 1;
	JumpMaxHoldTime = VariableJumpHeight ? VariableJumpHeightMaxHoldTime : 0.0f;

	if (ShadowDecalClass != nullptr)
	{
		USK_LOG_TRACE("Adding shadow decal");
		AActor* ShadowDecalActor = GetWorld()->SpawnActor(ShadowDecalClass);
		dynamic_cast<AShadowDecal*>(ShadowDecalActor)->Initialize(this);
	}
}

/**
 * @brief Tell client that the Pawn is begin restarted
 */
void AUSKCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	USK_LOG_TRACE("Adding input mapping context");
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	Subsystem->RemoveMappingContext(InputMappingContext);
	Subsystem->AddMappingContext(InputMappingContext, 0);
}

/**
 * @brief Allows a Pawn to set up custom input bindings
 * @param PlayerInputComponent The component that enables an Actor to bind various forms of input events
 */
void AUSKCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	USK_LOG_TRACE("Setting up input bindings");

	UEnhancedInputComponent* EnhancedInput = dynamic_cast<UEnhancedInputComponent*>(PlayerInputComponent);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUSKCharacter::MoveCharacter);
	EnhancedInput->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &AUSKCharacter::RotateCamera);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AUSKCharacter::Jump);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AUSKCharacter::StopJumping);
}

/**
 * @brief Called upon landing when falling, to perform actions based on the Hit result
 * @param Hit Result describing the landing that resulted in a valid landing spot
 */
void AUSKCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	USK_LOG_TRACE("Character Landed");
	JumpMaxCount = 1;
	CoyoteJumpPerformed = false;
	UAudioUtils::PlayRandomSound(this, LandedSoundEffects);

	if (LandParticleFx != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LandParticleFx,
			Hit.Location + LandParticleFxSpawnOffset);
	}
}

/**
 * @brief Called from CharacterMovementComponent to notify the character that the movement mode has changed
 * @param PrevMovementMode Movement mode before the change
 * @param PreviousCustomMode Custom mode before the change (applicable if PrevMovementMode is Custom)
 */
void AUSKCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	USK_LOG_TRACE("Movement mode changed");
	
	if (GetCharacterMovement()->MovementMode != MOVE_Falling)
	{
		return;
	}

	CanPerformCoyoteJump = true;

	USK_LOG_TRACE("Starting latent reset coyote jump action");
	FLatentActionInfo LatentAction;
	LatentAction.Linkage = 0;
	LatentAction.CallbackTarget = this;
	LatentAction.UUID = GetUniqueID();
	LatentAction.ExecutionFunction = "ResetCoyoteJump";
	UKismetSystemLibrary::RetriggerableDelay(GetWorld(), CoyoteJumpTime, LatentAction);
}

/**
 * @brief Make the character jump on the next update
 */
void AUSKCharacter::Jump()
{
	if (!CanJump() && !CanPerformCoyoteJump)
	{
		USK_LOG_TRACE("Can't jump");
		return;
	}

	bool WasRegularJump = false;
	if (CanJump())
	{
		WasRegularJump = true;
		Super::Jump();
	}
	else
	{		
		LaunchCharacter(FVector(0.0f, 0.0f, CoyoteJumpVelocity), false, true);
		CoyoteJumpPerformed = true;
		CanPerformCoyoteJump = false;
		USK_LOG_TRACE("Coyote jump performed");
	}

	JumpMaxCount = CanDoubleJump ? 2 : 1;
	IsDoubleJumping = WasRegularJump && (CoyoteJumpPerformed || JumpCurrentCount > 0);
	UAudioUtils::PlayRandomSound(this, JumpSoundEffects);

	if (JumpParticleFx != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), JumpParticleFx,
			GetActorLocation() + JumpParticleFxSpawnOffset);
	}
}

/**
 * @brief Move the character
 * @param Input The input action containing the input values
 */
void AUSKCharacter::MoveCharacter(const FInputActionValue& Input)
{
	const FVector2D InputValue = Input.Get<FVector2D>();
	
	FRotator Rotation = GetControlRotation();
	Rotation.Pitch = 0.0f;
	Rotation.Roll = 0.0f;
	AddMovementInput(UKismetMathLibrary::GetForwardVector(Rotation), InputValue.Y);

	Rotation = GetControlRotation();
	Rotation.Pitch = 0.0f;
	AddMovementInput(UKismetMathLibrary::GetRightVector(Rotation), InputValue.X);
}

/**
 * @brief Rotate the camera
 * @param Input The input action containing the input values
 */
void AUSKCharacter::RotateCamera(const FInputActionValue& Input)
{
	const FVector2D InputValue = Input.Get<FVector2D>();
	AddControllerYawInput(InputValue.X);
	AddControllerPitchInput(InputValue.Y);
}

/**
 * @brief Reset the coyote jump values
 */
void AUSKCharacter::ResetCoyoteJump()
{
	USK_LOG_TRACE("Resetting coyote jump");
	CanPerformCoyoteJump = false;
}