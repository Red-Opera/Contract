// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Bullet.generated.h"

UCLASS()
class CONTRACT_API ABullet : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABullet();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	class UProjectileMovementComponent* projectileMovement;

	UPROPERTY(EditAnywhere)
	class UCapsuleComponent* capsuleComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOtherHit(class UPrimitiveComponent* HitComp, 
		class AActor* otherActor, 
		class UPrimitiveComponent* otherComp,
		int32 otherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);


private:
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* bulletMesh;
};
