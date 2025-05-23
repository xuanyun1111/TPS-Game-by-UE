// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnMenu.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API UReturnMenu : public UUserWidget
{
	GENERATED_BODY()
public:
    void MenuSetup();
    void MenuTearDown();

protected:
    virtual bool Initialize() override;

    UFUNCTION()
        void OnDestroySession(bool bWasSuccessful);

private:
    UPROPERTY(meta = (BindWidget))
        class UButton* ReturnToMenu;

    UFUNCTION()
        void ReturnButtonClicked();

    UPROPERTY()
        class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

    UPROPERTY()
        class APlayerController* PlayerController;
};
