// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item.h"
#include "AmmoBox.generated.h"

UCLASS()
class CONTRACT_API AAmmoBox : public AItem
{
	GENERATED_BODY()
	
public:	
	AAmmoBox();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	void AddAmmo();				// �Ѿ��� �߰��ϴ� �޼ҵ�

private:
	int ammoCount = 30;
};
