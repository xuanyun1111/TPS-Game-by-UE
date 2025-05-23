// Fill out your copyright notice in the Description page of Project Settings.


#include "NewWorld1GameMode.h"
#include "GameFramework/GameStateBase.h"


void ANewWorld1GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	if (NumberOfPlayers == 2)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			bUseSeamlessTravel = true;
			World->ServerTravel(FString("/Game/map/AssetsvilleTown?listen"));
		}
	}
}