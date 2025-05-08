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

	// 총구 컴포넌트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	class UArrowComponent* muzzle;

	// 총알 Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	TSubclassOf<class ABullet> bulletBlueprint;

	// 발사 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	bool isFire;

	// 발사 딜레이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	float fireRate = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	class UNiagaraComponent* gunMuzzleFireNiagara;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// 발사 함수
	void Fire();
	void StartFire();
	void StopFire();

	void Reload();

	class AActor* player;
	class APlayerController* playerController;
	class UPlayerInventory* playerInventory;

	// 헤더 파일에 타이머 핸들 선언
	FTimerHandle TimerHandle_AutoFire;

	int maxCount = 30;		// 최대 총알 수
	int ammoCount = 0;		// 현재 총알 수
};
