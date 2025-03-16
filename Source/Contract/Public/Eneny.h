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
	class UNiagaraSystem* damageNiagara;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	float textOffset = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TArray<UTexture*> digitImage;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	class AActor* player;
	class APlayerController* playerController;
};
