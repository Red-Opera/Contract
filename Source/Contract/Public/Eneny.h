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

	// ������ ǥ�� ���̾ư���
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class AFloatingDamage> damageParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	float textOffset = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TArray<UTexture*> digitImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	bool isDead = false;

	// �߻� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	bool isFire;

	// �߻� ������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	float fireRate = 0.1f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// �߻� �Լ�
	void Fire();
	void StartFire();
	void StopFire();

	class AActor* player;
	class APlayerController* playerController;

	float hp = 1000000.0f;

	FTimerHandle TimerHandle_AutoFire;
};
