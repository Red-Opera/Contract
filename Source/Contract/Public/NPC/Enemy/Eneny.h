// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Eneny.generated.h"

UCLASS()
class CONTRACT_API AEneny : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEneny();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void SetDamage(FVector hitLocation, int damage);

	// 데미지 표기 나이아가라
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class AFloatingDamage> damageParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	float textOffset = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TArray<UTexture*> digitImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	bool isDead = false;

	// 발사 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	bool isFire;

	// 발사 딜레이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	float fireRate = 0.1f;

	// 적이 갖고 있는 아이템
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
	TArray<int> itemCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
	int money = 0;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// 발사 함수
	void Fire();
	void StartFire();
	void StopFire();

	void Death();

	class AActor* player;
	class APlayerController* playerController;
	class UIDToItem* idToItem;

	float hp = 1000.0f;

	FTimerHandle TimerHandle_AutoFire;
};
