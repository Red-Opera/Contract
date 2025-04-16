// Fill out your copyright notice in the Description page of Project Settings.


#include "Eneny.h"
#include "FloatingDamage.h"
#include "IDToItem.h"

#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

AEneny::AEneny()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AEneny::BeginPlay()
{
	Super::BeginPlay();

	itemCount.Add(2);
	itemCount.Add(4);

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

	FString assetPath = TEXT("DataAsset'/Game/PlayerInventory/IDToDataAsset.IDToDataAsset'");
	idToItem = Cast<UIDToItem>(StaticLoadObject(UIDToItem::StaticClass(), nullptr, *assetPath));

	if (idToItem == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("IDToItem does not exist."));
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

void AEneny::Death()
{
	// ���� ���� ��ġ�� �������� �������� ��ġ
	FVector baseLocation = GetActorLocation();

	// itemCount �迭�� ��ȸ�ϸ鼭 �� �������� ��ġ
	for (int32 itemId = 0; itemId < itemCount.Num(); itemId++)
	{
		// �ش� ID�� ������ ������ŭ �ݺ�
		for (int32 i = 0; i < itemCount[itemId]; i++)
		{
			// idToItem���� ������ Ŭ���� ��������
			TSubclassOf<AItem> itemClass = idToItem->intToitems[itemId];

			if (itemClass != nullptr)
			{
				// ������ ��ġ ��� (���� ��ġ���� �ݰ� 100 ���� �̳�)
				const float spawnRadius = 100.0f;

				FVector randomOffset
				(
					FMath::RandRange(-spawnRadius, spawnRadius),
					FMath::RandRange(-spawnRadius, spawnRadius),
					0.0f  // Z���� 0���� �����Ͽ� ���� ��鿡 ����
				);

				FVector spawnLocation = baseLocation + randomOffset;

				// ������ ���� (SpawnActorDeferred�� ����Ͽ� BeginPlay ȣ�� ���� �Ӽ� ���� ����)
				FTransform spawnTransform = FTransform(FRotator::ZeroRotator, spawnLocation);
				AItem* newItem = GetWorld()->SpawnActorDeferred<AItem>(itemClass, spawnTransform);

				if (newItem != nullptr)
				{
					// BeginPlay ȣ���Ͽ� ������ ���� �Ϸ�
					UGameplayStatics::FinishSpawningActor(newItem, spawnTransform);

					// ����� �޽��� ���
					GEngine->AddOnScreenDebugMessage
					(
						-1, 5.f, 
						FColor::Green,
						FString::Printf(TEXT("Item ID %d spawned at location: %s"), itemId, *spawnLocation.ToString())
					);
				}

				else
					GEngine->AddOnScreenDebugMessage
					(
						-1, 5.f,
						FColor::Red,
						FString::Printf(TEXT("Failed to spawn item ID %d"), itemId)
					);
			}

			// ������ Ŭ������ ã�� �� ���� ��� ���� �޽��� ���
			else
				GEngine->AddOnScreenDebugMessage
				(
					-1, 5.f, 
					FColor::Red, 
					FString::Printf(TEXT("Item ID %d not found in idToItem map"), itemId)
				);
		}
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

		Death();
	}

	FTransform spawnTransform = FTransform(FRotator::ZeroRotator, hitLocation);
	AFloatingDamage* newDamageWidget = GetWorld()->SpawnActorDeferred<AFloatingDamage>(damageParticle, spawnTransform);

	// BeginPlay ���� damage ���� ����
	newDamageWidget->SetDamageValue(damage);

	// ���� ������ �������Ͽ� BeginPlay�� ȣ��ǵ��� ��
	UGameplayStatics::FinishSpawningActor(newDamageWidget, spawnTransform);
}