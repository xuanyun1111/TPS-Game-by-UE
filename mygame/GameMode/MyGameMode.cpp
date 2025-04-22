// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameMode.h"
#include "mygame/Character/MyCharacter.h"
#include "mygame/PlayerController/MyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "mygame/PlayerState/MyPlayerState.h"

AMyGameMode::AMyGameMode()
{
	bDelayedStart = true;
}

void AMyGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
}

void AMyGameMode::PlayerDeath(class AMyCharacter* DeathCharacter, class AMyPlayerController* VictimController, AMyPlayerController* AttackerController)
{
	AMyPlayerState* AttackerPlayerState = AttackerController ? Cast<AMyPlayerState>(AttackerController->PlayerState) : nullptr;
	AMyPlayerState* VictimPlayerState = VictimController ? Cast<AMyPlayerState>(VictimController->PlayerState) : nullptr;
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(10.f);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}
	if (DeathCharacter)
	{
		DeathCharacter->Death();
	}
}
void AMyGameMode::RequestRespawn(class ACharacter* DeathCharacter, AController* DeathController)
{
	if (DeathCharacter)
	{
		DeathCharacter->Reset();
		DeathCharacter->Destroy();
	}
	if (DeathController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(DeathController, PlayerStarts[Selection]);
	}
}

void AMyGameMode::BeginPlay()
{
	Super::BeginPlay();
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void AMyGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMyPlayerController* MyPlayer = Cast<AMyPlayerController>(*It);
		if (MyPlayer)
		{
			MyPlayer->OnMatchStateSet(MatchState);
		}
	}
}
