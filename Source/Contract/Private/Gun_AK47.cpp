// Fill out your copyright notice in the Description page of Project Settings.


#include "Gun_AK47.h"

#include "GameFramework/PlayerController.h"

#include "Components/ArrowComponent.h"
#include "Bullet.h"

// Sets default values
AGun_AK47::AGun_AK47()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	muzzle = CreateDefaultSubobject<UArrowComponent>(TEXT("Muzzle"));
	muzzle->SetupAttachment(GetMesh(), TEXT("Muzzle"));
}

// Called when the game starts or when spawned
void AGun_AK47::BeginPlay()
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

	UInputComponent* inputComponent = playerController->InputComponent;

	if (inputComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InputComponent does not exist."));
		return;
	}

	if (bulletBlueprint == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BulletBlueprint does not exist."));
		return;
	}

	EnableInput(playerController);

	inputComponent->BindAction(TEXT("GunFire"), IE_Pressed, this, &AGun_AK47::Fire);
}

void AGun_AK47::Fire()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Bullet is Fire!!"));

	if (bulletBlueprint)
	{
		FActorSpawnParameters params;

		params.Owner = this;
		params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FVector muzzleLocation = muzzle->GetComponentLocation();
		FVector muzzleForawrd = muzzle->GetForwardVector();

		GetWorld()->SpawnActor<ABullet>(bulletBlueprint, muzzleLocation, muzzle->GetComponentRotation(), params);
	}
}

// Called every frame
void AGun_AK47::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AGun_AK47::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

