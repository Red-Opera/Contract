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

	// 현재 체력이 0이면 데미지를 주지 않음
	if (hp <= 0)
		return;

	// 데미지를 받음
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

	// BeginPlay 전에 damage 값을 설정
	newDamageWidget->SetDamageValue(damage);

	// 액터 스폰을 마무리하여 BeginPlay가 호출되도록 함
	UGameplayStatics::FinishSpawningActor(newDamageWidget, spawnTransform);
}