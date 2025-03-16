// Fill out your copyright notice in the Description page of Project Settings.


#include "Eneny.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

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

	float currentOffset = 0.0f;

	FRotator playerRotate = playerController->GetControlRotation();
	FRotator niagaraRotate = FRotator(180.0f, playerRotate.Yaw, 0.0f);

	// ÀÚ¸´¼ö ºÐ¸®
	for (int i = 0; i < damageString.Len(); i++)
	{
		FString digit = damageString.Mid(i, 1);

		UNiagaraComponent* digitNia = UNiagaraFunctionLibrary::SpawnSystemAtLocation
		(
			GetWorld(), 
			damageNiagara, 
			hitLocation, 
			niagaraRotate,
			FVector(1.0f)
		);

		digitNia->AddLocalOffset(FVector(0.0f, currentOffset, 0.0f));
		currentOffset += textOffset;

		FName digitName = FName(*digit);
		int32 digitIndex = FCString::Atoi(*digit);

		UNiagaraFunctionLibrary::SetTextureObject(digitNia, "Digit", digitImage[digitIndex]);
	}
}