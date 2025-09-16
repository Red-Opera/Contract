﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "Bullet.h"
#include "Eneny.h"

#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include <Kismet\KismetMathLibrary.h>

ABullet::ABullet()
{
	PrimaryActorTick.bCanEverTick = true;

	bulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	RootComponent = bulletMesh;

	capsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	capsuleComponent->SetupAttachment(bulletMesh);

	projectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	projectileMovement->UpdatedComponent = RootComponent;
	projectileMovement->InitialSpeed = 6000.0f;
	projectileMovement->MaxSpeed = 6000.0f;
	projectileMovement->bRotationFollowsVelocity = true;

	capsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &ABullet::OnOtherHit);

	// 충돌 속성 설정
	capsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	capsuleComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	capsuleComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	capsuleComponent->SetGenerateOverlapEvents(true);
}

void ABullet::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABullet::OnOtherHit(UPrimitiveComponent* HitComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (otherActor == nullptr || otherActor == this || !otherActor->IsA(ACharacter::StaticClass()))
		return;

	FString actorName = otherActor->GetName();

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Bullet Hit : %s"), *actorName));

	// Eneny 타입인지 확인
	if (otherActor->IsA(AEneny::StaticClass()))
	{
		AEneny* eneny = Cast<AEneny>(otherActor);
		eneny->SetDamage(GetActorLocation(), UKismetMathLibrary::RandomIntegerInRange(0, 2000));
	}

	Destroy();
}

// Called every frame
void ABullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABullet::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

