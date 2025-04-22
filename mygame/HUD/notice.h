// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "notice.generated.h"

/**
 * 
 */
UCLASS()
class MYGAME_API Unotice : public UUserWidget
{
	GENERATED_BODY()
public:

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* WarmupTime;
    UPROPERTY(meta = (BindWidget))
    UTextBlock* notice;
    UPROPERTY(meta = (BindWidget))
    UTextBlock* notwhat;

};
