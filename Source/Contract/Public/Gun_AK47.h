// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Gun_AK47.generated.h"

UCLASS()
class CONTRACT_API AGun_AK47 : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGun_AK47();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ÃÑ±¸ ÄÄÆ÷³ÍÆ®
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	class UArrowComponent* muzzle;

	// ÃÑ¾Ë Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	TSubclassOf<class ABullet> bulletBlueprint;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// ¹ß»ç ÇÔ¼ö
	void Fire();

	class AActor* player;
	class APlayerController* playerController;
};
