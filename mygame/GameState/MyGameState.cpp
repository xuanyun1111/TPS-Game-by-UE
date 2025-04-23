                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        // Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameState.h"
#include "Net/UnrealNetwork.h"
#include "mygame/PlayerState/MyPlayerState.h"
void AMyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMyGameState, TopScoringPlayers);
}

void AMyGameState::UpdateTopScore(class AMyPlayerState* ScoringPlayer)
{
    if (TopScoringPlayers.Num() == 0)
    {
        TopScoringPlayers.Add(ScoringPlayer);
        TopScore = ScoringPlayer->GetScore();
    }
    else if (ScoringPlayer->GetScore() == TopScore)
    {
        TopScoringPlayers.AddUnique(ScoringPlayer);
    }
    else if (ScoringPlayer->GetScore() > TopScore)
    {
        TopScoringPlayers.Empty();
        TopScoringPlayers.AddUnique(ScoringPlayer);
        TopScore = ScoringPlayer->GetScore();
    }
}