// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "mygame/HUD/MyHUD.h"
#include "mygame/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Net/UnrealNetwork.h"
#include "mygame/GameMode/MyGameMode.h"
#include "mygame/PlayerState/MyPlayerState.h"
#include "mygame/HUD/notice.h"
#include "Kismet/GameplayStatics.h"

void AMyPlayerController::SetHUDScore(float Score)
{
	MyHUD = MyHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : MyHUD;
	bool bHUDValid = MyHUD && 
		MyHUD->CharacterOverlay && 
		MyHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		MyHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void AMyPlayerController::SetHUDDefeats(int32 Defeats)
{
	MyHUD = MyHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : MyHUD;
	bool bHUDValid = MyHUD &&
		MyHUD->CharacterOverlay &&
		MyHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		MyHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void AMyPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	MyHUD = MyHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : MyHUD;
	bool bHUDValid = MyHUD &&
		MyHUD->CharacterOverlay &&
		MyHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		MyHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void AMyPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	MyHUD = MyHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : MyHUD;
	bool bHUDValid = MyHUD &&
		MyHUD->CharacterOverlay &&
		MyHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString CarriedAmmoText = FString::Printf(TEXT("%d"), Ammo);
		MyHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoText));
	}
}

void AMyPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	MyHUD = MyHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : MyHUD;
	bool bHUDValid = MyHUD &&
		MyHUD->CharacterOverlay &&
		MyHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			MyHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		MyHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void AMyPlayerController::SetHUDnoticeCountdown(float CountdownTime)
{
	MyHUD = MyHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : MyHUD;
	bool bHUDValid = MyHUD &&
		MyHUD->notice &&
		MyHUD->notice->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			MyHUD->notice->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		MyHUD->notice->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void AMyPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
}

void AMyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyPlayerController, MatchState);
}

float AMyPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AMyPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AMyPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}

}

void AMyPlayerController::HandleMatchHasStarted()
{
	MyHUD = MyHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : MyHUD;
	if (MyHUD)
	{
		MyHUD->AddCharacterOverlay();
		if (MyHUD->notice)
		{
			MyHUD->notice->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AMyPlayerController::HandleCooldown()
{
	MyHUD = MyHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : MyHUD;
	if (MyHUD)
	{
		MyHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = MyHUD->notice &&
			MyHUD->notice->notice &&
			MyHUD->notice->notwhat;

		if (bHUDValid)
		{
			MyHUD->notice->SetVisibility(ESlateVisibility::Visible);
			FString noticeText("New Match Starts In:");
			MyHUD->notice->notice->SetText(FText::FromString(noticeText));
			MyHUD->notice->notwhat->SetText(FText());
		}
		if (MyHUD->notice)
		{
			MyHUD->notice->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	MyHUD = Cast<AMyHUD>(GetHUD());
	ServerCheckMatchState();
	
}
void AMyPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDnoticeCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

void AMyPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (MyHUD && MyHUD->CharacterOverlay)
		{
			CharacterOverlay = MyHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}

void AMyPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}
void AMyPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}
void AMyPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
	
}
void AMyPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if (MyHUD && MatchState == MatchState::WaitingToStart)
	{
		MyHUD->Addnotice();
	}
}
void AMyPlayerController::ServerCheckMatchState_Implementation()
{
	AMyGameMode* GameMode = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void AMyPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}
void AMyPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	MyHUD = MyHUD == nullptr ? Cast<AMyHUD>(GetHUD()) : MyHUD;
	bool bHUDValid = MyHUD && MyHUD->CharacterOverlay && MyHUD->CharacterOverlay->HealthBar && MyHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		MyHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		MyHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}