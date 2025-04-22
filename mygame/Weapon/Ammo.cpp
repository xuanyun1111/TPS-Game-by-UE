// Fill out your copyright notice in the Description page of Project Settings.


#include "Ammo.h"
#include "Sound/SoundCue.h"

#include "Kismet/GameplayStatics.h"


// Sets default values
AAmmo::AAmmo()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	AmmoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AmmoMesh"));
	SetRootComponent(AmmoMesh);
	AmmoMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	AmmoMesh->SetSimulatePhysics(true);
	AmmoMesh->SetEnableGravity(true);
	AmmoMesh->SetNotifyRigidBodyCollision(true);
	AmmoEjectionImpulse = 5.f;
}

// Called when the game starts or when spawned
void AAmmo::BeginPlay()
{
	Super::BeginPlay();
	AmmoMesh->OnComponentHit.AddDynamic(this, &AAmmo::OnHit);
	AmmoMesh->AddImpulse(GetActorForwardVector()* AmmoEjectionImpulse);
}

void AAmmo::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (AmmoSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AmmoSound, GetActorLocation());
	}
	Destroy();
}

// Called every frame


