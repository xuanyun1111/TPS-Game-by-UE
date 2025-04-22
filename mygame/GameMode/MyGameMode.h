// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API AMyGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	AMyGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerDeath(class AMyCharacter* DeathCharacter, class AMyPlayerController* VictimController, AMyPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* DeathCharacter, AController* DeathController);
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 20.f;
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 300.f;

	float LevelStartingTime = 0.f;
protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
private:
	float CountdownTime = 0.f;
};
