// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Ammo.generated.h"

UCLASS()
class MYGAME_API AAmmo : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAmmo();
private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AmmoMesh;
	UPROPERTY(EditAnywhere)
	float AmmoEjectionImpulse;
	UPROPERTY(EditAnywhere)
	class USoundCue* AmmoSound;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UFUNCTION()
		virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
