// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "mygame/Weapon/Weapon.h"
#include "mygame/gameComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MyAnimInstance.h"
#include "mygame/Weapon/Projectile.h"
#include "mygame/mygame.h"
#include "mygame/PlayerController/MyPlayerController.h"
#include "mygame/GameMode/MyGameMode.h"
#include "TimerManager.h"
#include "mygame/PlayerState/MyPlayerState.h"
#include "mygame/Weapon/WeaponTypes.h"
// Sets default values
AMyCharacter::AMyCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 250.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverHeadWidget"));
	OverHeadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}
void AMyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::GetLifetimeReplicatedProps")));
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AMyCharacter, OverlappingWeapon, COND_OwnerOnly);
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::GetLifetimeReplicatedProps__DOREPLIFETIME_CONDITION")));
	DOREPLIFETIME(AMyCharacter, Health);
}



// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateHUDHealth();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AMyCharacter::ReceiveDamage);
	}
}
// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAim_Pitch();
	}
	HideCameraIfCharacterClose();
	PollInit();
}
// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &AMyCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMyCharacter::LookUp);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMyCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &AMyCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AMyCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AMyCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &AMyCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AMyCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AMyCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AMyCharacter::ReloadButtonPressed);
	//°ó¶¨°´¼ü
}
void AMyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

void AMyCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AMyCharacter::PlayDeathMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
		
	}
}

void AMyCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		}

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AMyCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void AMyCharacter::Death()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastDeath();
	GetWorldTimerManager().SetTimer(
		DeathTimer,
		this,
		&AMyCharacter::DeathTimerFinished,
		DeathDelay
	);
}

void AMyCharacter::MulticastDeath_Implementation()
{
	if (MyPlayerController)
	{
		MyPlayerController->SetHUDWeaponAmmo(0);
	}
	bDeath = true;
	PlayDeathMontage();
	if (DissolveMaterialInstance_body)
	{
		DynamicDissolveMaterialInstance_body = UMaterialInstanceDynamic::Create(DissolveMaterialInstance_body, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance_body);
		DynamicDissolveMaterialInstance_body->SetScalarParameterValue(TEXT("Dissolve"), 0.6f);
		DynamicDissolveMaterialInstance_body->SetScalarParameterValue(TEXT("light"), 182.f);
	}
	if (DissolveMaterialInstance_logo)
	{
		DynamicDissolveMaterialInstance_logo = UMaterialInstanceDynamic::Create(DissolveMaterialInstance_logo, this);
		GetMesh()->SetMaterial(1, DynamicDissolveMaterialInstance_logo);
		DynamicDissolveMaterialInstance_logo->SetScalarParameterValue(TEXT("Dissolve"), 0.6f);
		DynamicDissolveMaterialInstance_logo->SetScalarParameterValue(TEXT("light"), 182.f);
	}
	StartDissolve();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (MyPlayerController)
	{
		DisableInput(MyPlayerController);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMyCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AMyCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCause)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();
	if (Health == 0.f)
	{
		AMyGameMode* MyGameMode = GetWorld()->GetAuthGameMode<AMyGameMode>();
		if (MyGameMode)
		{
			MyPlayerController = MyPlayerController == nullptr ? Cast<AMyPlayerController>(Controller) : MyPlayerController;
			AMyPlayerController* AttackerController = Cast<AMyPlayerController>(InstigatorController);
			MyGameMode->PlayerDeath(this, MyPlayerController, AttackerController);
		}
	}
}

void AMyCharacter::UpdateHUDHealth()
{
	MyPlayerController = MyPlayerController == nullptr ? Cast<AMyPlayerController>(Controller) : MyPlayerController;
	if (MyPlayerController)
	{
		MyPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void AMyCharacter::PollInit()
{
	if (MyPlayerState == nullptr)
	{
		MyPlayerState = GetPlayerState<AMyPlayerState>();
		if (MyPlayerState)
		{
			MyPlayerState->AddToScore(0.f);
			MyPlayerState->AddToDefeats(0);
		}
	}
}


void AMyCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f) {
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void AMyCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f) {
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void AMyCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void AMyCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}
void AMyCharacter::EquipButtonPressed()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("use function:EquipButtonPressed")));
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void AMyCharacter::ServerEquipButtonPressed_Implementation()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("use function:ServerEquipButtonPressed_Implementation")));
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}

}
void AMyCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}
void AMyCharacter::ReloadButtonPressed()
{
	if (Combat)
	{
		Combat->Reload();
	}
}
void AMyCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::AimButtonPressed__Combat->bAiming = true")));
	}
}
void AMyCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::AimButtonPressed__Combat->bAiming = false")));
	}
}
void AMyCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr)return;
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	if (Speed == 0.f && !bIsInAir)
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation,StartingAimRotation );
		Aim_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAim_Yaw = Aim_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		Aim_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
	CalculateAim_Pitch();
	/*if (!HasAuthority() && !IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("%.f"), Aim_Yaw));
	}
	*/
}
void AMyCharacter::CalculateAim_Pitch()
{
	Aim_Pitch = GetBaseAimRotation().Pitch;
	if (Aim_Pitch > 90.f && !IsLocallyControlled())
	{
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		Aim_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, Aim_Pitch);
	}
}
void AMyCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)return;
	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;
	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}
void AMyCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}
void AMyCharacter::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::FireButtonPressed__Combat->FireButtonPressed = true")));
	}
}
void AMyCharacter::FireButtonReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::FireButtonPressed__Combat->FireButtonPressed = false")));
	}
}
void AMyCharacter::TurnInPlace(float DeltaTime)
{
	//UE_LOG(LogTemp, Warning, TEXT("Aim_Yam: %f"), Aim_Yaw);
	if (Aim_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (Aim_Yaw < -90.f) 
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAim_Yaw = FMath::FInterpTo(InterpAim_Yaw, 0.f, DeltaTime, 4.f);
		Aim_Yaw = InterpAim_Yaw;
		if (FMath::Abs(Aim_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void AMyCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled())return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}
float AMyCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}
void AMyCharacter::OnRep_Health()
{
	
	UpdateHUDHealth();
	PlayHitReactMontage();
}
void AMyCharacter::DeathTimerFinished()
{
	AMyGameMode* MyGameMode = GetWorld()->GetAuthGameMode<AMyGameMode>();
	if (MyGameMode)
	{
		MyGameMode->RequestRespawn(this, Controller);
	}
}
void AMyCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance_body)
	{
		DynamicDissolveMaterialInstance_body->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
	if (DynamicDissolveMaterialInstance_logo)
	{
		DynamicDissolveMaterialInstance_logo->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}
void AMyCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AMyCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}
void AMyCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::SetOverlappingWeapon")));
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::SetOverlappingWeapon__OverlappingWeapon->ShowPickupWidget(false)")));
	}
	OverlappingWeapon = Weapon;
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::SetOverlappingWeapon__OverlappingWeapon = Weapon")));
	if (IsLocallyControlled())
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::SetOverlappingWeapon__IsLocallyControlled")));
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
			//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::SetOverlappingWeapon__IsLocallyControlled__OverlappingWeapon__OverlappingWeapon->ShowPickupWidget(true)")));
		}
	}
}
void AMyCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::OnRep_OverlappingWeapon")));
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::OnRep_OverlappingWeapon__OverlappingWeapon__OverlappingWeapon->ShowPickupWidget(true)")));
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::OnRep_OverlappingWeapon__LastWeapon__LastWeapon->ShowPickupWidget(false);")));
	}
}
bool AMyCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool AMyCharacter::IsAiming()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("AMyCharacter::IsAiming")));
	return (Combat && Combat->bAiming);
}

AWeapon* AMyCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr)return nullptr;
	return Combat->EquippedWeapon;
}

FVector AMyCharacter::GetHitTarget() const
{
	if (Combat == nullptr)return FVector();
	return Combat->HitTarget;
}

ECombatState AMyCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}
