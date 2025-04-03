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

	// �ѱ� ������Ʈ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	class UArrowComponent* muzzle;

	// �Ѿ� Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	TSubclassOf<class ABullet> bulletBlueprint;

	// �߻� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	bool isFire;

	// �߻� ������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	float fireRate = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	class UNiagaraComponent* gunMuzzleFireNiagara;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// �߻� �Լ�
	void Fire();
	void StartFire();
	void StopFire();

	void Reload();

	class AActor* player;
	class APlayerController* playerController;
	class UPlayerItem* playerData;

	// ��� ���Ͽ� Ÿ�̸� �ڵ� ����
	FTimerHandle TimerHandle_AutoFire;

	int maxCount = 30;		// �ִ� �Ѿ� ��
	int ammoCount = 0;		// ���� �Ѿ� ��
};
