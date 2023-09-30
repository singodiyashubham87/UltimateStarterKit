## Introduction
The projectile spawned by weapons

## Components
The <code>WeaponProjectile</code> uses the following components:
<table>
	<tr>
		<th>Name</th>
		<th>Description</th>
		<th>Type</th>
	</tr>
	<tr>
		<td>CollisionComponent</td>
		<td>The projectile spawned by weapons</td>
		<td>USphereComponent*</td>
	</tr>
	<tr>
		<td>ProjectileMovementComponent</td>
		<td>The projectile movement component used to move the projectile</td>
		<td>UProjectileMovementComponent*</td>
	</tr>
</table>

## API Reference
### Functions
<table>
	<tr>
		<th>Name</th>
		<th>Description</th>
		<th>Params</th>
		<th>Return</th>
	</tr>
	<tr>
		<td>GetCollisionComponent</td>
		<td>Get the collision component used by the projectile</td>
		<td></td>
		<td><strong>USphereComponent*</strong><br/>The collision component used by the projectile</td>
	</tr>
	<tr>
		<td>GetProjectileMovementComponent</td>
		<td>Get the projectile movement component used to move the projectile</td>
		<td></td>
		<td><strong>UProjectileMovementComponent*</strong><br/>The projectile movement component used to move the projectile</td>
	</tr>
	<tr>
		<td>NormalImpulse, const FHitResult& HitResult);</td>
		<td>Called after the projectile hits something</td>
		<td><strong>HitComponent (HitResult);)</strong><br/>The component responsible for the hit<br/><br/><strong>OtherActor (HitResult);)</strong><br/>The actor that was hit<br/><br/><strong>OtherComponent (HitResult);)</strong><br/>The component that was hit<br/><br/><strong>NormalImpulse (FVector)</strong><br/>The normal impulse of the hit<br/><br/><strong>HitResult (FHitResult&)</strong><br/>Result describing the hit</td>
		<td></td>
	</tr>
</table>

## Blueprint Usage
You can use the <code>WeaponProjectile</code> using Blueprints by adding one of the following nodes:
<ul>
	<li>Ultimate Starter Kit > Weapon Projectile > Get Collision Component</li>
	<li>Ultimate Starter Kit > Weapon Projectile > Get Projectile Movement Component</li>
	<li>Ultimate Starter Kit > Weapon Projectile > Normal Impulse, const FHit Result& Hit Result);</li>
</ul>

## C++ Usage
Before you can use the plugin, you first need to enable the plugin in your <code>Build.cs</code> file:
```c++
PublicDependencyModuleNames.Add("USK");
```

The <code>WeaponProjectile</code> can now be used in any of your C++ files:
```c++
#include "USK/Weapons/WeaponProjectile.h"

void ATestActor::Test()
{
	// WeaponProjectile is a pointer to the AWeaponProjectile
	USphereComponent* CollisionComponent = WeaponProjectile->GetCollisionComponent();
	UProjectileMovementComponent* ProjectileMovementComponent = WeaponProjectile->GetProjectileMovementComponent();
	WeaponProjectile->NormalImpulse, const FHitResult& HitResult);(HitComponent, OtherActor, OtherComponent, NormalImpulse, HitResult);
}
```