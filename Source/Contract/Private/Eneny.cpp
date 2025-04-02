// Fill out your copyright notice in the Description page of Project Settings.


#include "Eneny.h"

#include "FloatingDamage.h"
#include "Components/WidgetComponent.h"
#include <Kismet\GameplayStatics.h>

// Sets default values
AEneny::AEneny()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AEneny::BeginPlay()
{
	Super::BeginPlay();

	player = GetWorld()->GetFirstPlayerController()->GetPawn();
	playerController = GetWorld()->GetFirstPlayerController();

	if (player == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player does not exist."));
		return;
	}

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerController does not exist."));
		return;
	}

	if (damageParticle == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("DamageNiagara does not exist."));
		return;
	}

	UInputComponent* inputComponent = playerController->InputComponent;

	if (inputComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InputComponent does not exist."));
		return;
	}


	EnableInput(playerController);

	// �Է� ���ε� ����  
	inputComponent->BindAction(TEXT("GunFire"), IE_Pressed, this, &AEneny::StartFire);
	inputComponent->BindAction(TEXT("GunFire"), IE_Released, this, &AEneny::StopFire);
}

void AEneny::Fire()
{

}

void AEneny::StartFire()
{
	isFire = true;

	// ó�� �� �� ��� �߻�  
	Fire();

	// ���� FireRate �������� Fire() �Լ��� �ݺ� ȣ�� (FireRate�� �߻� ����)  
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AutoFire, this, &AEneny::Fire, fireRate, true);
}

void AEneny::StopFire()
{
	isFire = false;

	// Ÿ�̸� �ڵ��� �̿��Ͽ� �ݺ� ȣ�� ����  
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoFire);
}

// Called every frame
void AEneny::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEneny::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEneny::SetDamage(FVector hitLocation, int damage)
{
	FString damageString = FString::Printf(TEXT("%d"), damage);

	// ���� ü���� 0�̸� �������� ���� ����
	if (hp <= 0)
		return;

	// �������� ����
	int resultHP = hp - damage;

	if (resultHP > 0)
		hp = resultHP;

	else
	{
		hp = 0;
		isDead = true;
	}

	FTransform spawnTransform = FTransform(FRotator::ZeroRotator, hitLocation);
	AFloatingDamage* newDamageWidget = GetWorld()->SpawnActorDeferred<AFloatingDamage>(damageParticle, spawnTransform);

	// BeginPlay ���� damage ���� ����
	newDamageWidget->SetDamageValue(damage);

	// ���� ������ �������Ͽ� BeginPlay�� ȣ��ǵ��� ��
	UGameplayStatics::FinishSpawningActor(newDamageWidget, spawnTransform);
}