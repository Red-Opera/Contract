// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmmoBox.generated.h"

UCLASS()
class CONTRACT_API AAmmoBox : public AActor
{
	GENERATED_BODY()
	
public:	
	AAmmoBox();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	UStaticMeshComponent* mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float interactionDistance = 120.0f;

protected:
	virtual void BeginPlay() override;

	bool CheckPlayerIsClose();	// 플레이어가 시계와 가까운지 확인하는 메소드

	void AddAmmo();				// 총알을 추가하는 메소드

private:
	class ACharacter* player;
	class APlayerController* playerController;

	bool isPlayerClose = false;
	int ammoCount;
};
