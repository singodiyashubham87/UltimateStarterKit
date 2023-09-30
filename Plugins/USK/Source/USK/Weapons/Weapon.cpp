﻿// Created by Henry Jooste

#include "Weapon.h"

#include "USK/Audio/AudioUtils.h"
#include "USK/Character/USKCharacter.h"

/**
 * @brief Attach the weapon to a character
 * @param TargetCharacter The character that will use the weapon
 */
void AWeapon::AttachWeapon(AUSKCharacter* TargetCharacter)
{
	Character = TargetCharacter;
	if (!IsValid(Character))
	{
		return;
	}

	const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh(), AttachmentRules, Character->WeaponAttachPoint);
	Character->SetWeapon(this);
}

/**
 * @brief Fire the weapon
 */
void AWeapon::Fire()
{
	if (!IsValid(Character) || !IsValid(Character->GetController()))
	{
		return;
	}

	SpawnProjectile();
	PlayFireAnimation();	
}

/**
 * @brief Spawn the projectile
 */
void AWeapon::SpawnProjectile() const
{
	if (!IsValid(ProjectileClass))
	{
		return;
	}

	const APlayerController* PlayerController = dynamic_cast<APlayerController*>(Character->GetController());
	const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
	const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	GetWorld()->SpawnActor<AWeaponProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	UAudioUtils::PlaySound(Character, FireSound);
}

/**
 * @brief Play the fire animation
 */
void AWeapon::PlayFireAnimation() const
{
	if (!IsValid(FireAnimation))
	{
		return;
	}

	Character->PlayAnimMontage(FireAnimation, 1.0f);
}