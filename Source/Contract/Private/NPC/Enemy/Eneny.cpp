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
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player가 존재하지 않음"));
		return;
	}

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerController가 존재하지 않음"));
		return;
	}

	if (damageParticle == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("DamageNiagara가 존재하지 않음"));
		return;
	}

	UInputComponent* inputComponent = playerController->InputComponent;

	if (inputComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InputComponent가 존재하지 않음"));
		return;
	}

	FString assetPath = TEXT("DataAsset'/Game/PlayerInventory/IDToDataAsset.IDToDataAsset'");
	idToItem = Cast<UIDToItem>(StaticLoadObject(UIDToItem::StaticClass(), nullptr, *assetPath));

	if (idToItem == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("IDToItem가 존재하지 않음"));
		return;
	}

	EnableInput(playerController);

	// 입력 바인딩 설정  
	inputComponent->BindAction(TEXT("GunFire"), IE_Pressed, this, &AEneny::StartFire);
	inputComponent->BindAction(TEXT("GunFire"), IE_Released, this, &AEneny::StopFire);
}

void AEneny::Fire()
{

}

void AEneny::StartFire()
{
	isFire = true;

	// 처음 한 번 즉시 발사  
	Fire();

	// 이후 FireRate 간격으로 Fire() 함수를 반복 호출 (FireRate는 발사 간격)  
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AutoFire, this, &AEneny::Fire, fireRate, true);
}

void AEneny::StopFire()
{
	isFire = false;

	// 타이머 핸들을 이용하여 반복 호출 중지  
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoFire);
}

void AEneny::Death()
{
	// 적이 죽은 위치를 기준으로 아이템을 배치
	FVector baseLocation = GetActorLocation();

	// itemCount 배열을 순회하면서 각 아이템을 배치
	for (int32 itemId = 0; itemId < itemCount.Num(); itemId++)
	{
		// 해당 ID의 아이템 개수만큼 반복
		for (int32 i = 0; i < itemCount[itemId]; i++)
		{
			// idToItem에서 아이템 클래스 가져오기
			TSubclassOf<AItem> itemClass = idToItem->intToitems[itemId];

			if (itemClass != nullptr)
			{
				// 랜덤한 위치 계산 (현재 위치에서 반경 100 유닛 이내)
				const float spawnRadius = 100.0f;

				FVector randomOffset
				(
					FMath::RandRange(-spawnRadius, spawnRadius),
					FMath::RandRange(-spawnRadius, spawnRadius),
					0.0f  // Z축은 0으로 설정하여 같은 평면에 생성
				);

				FVector spawnLocation = baseLocation + randomOffset;

				// 아이템 생성 (SpawnActorDeferred를 사용하여 BeginPlay 호출 전에 속성 설정 가능)
				FTransform spawnTransform = FTransform(FRotator::ZeroRotator, spawnLocation);
				AItem* newItem = GetWorld()->SpawnActorDeferred<AItem>(itemClass, spawnTransform);

				if (newItem != nullptr)
				{
					// BeginPlay 호출하여 아이템 생성 완료
					UGameplayStatics::FinishSpawningActor(newItem, spawnTransform);

					// 디버그 메시지 출력
					GEngine->AddOnScreenDebugMessage
					(
						-1, 5.f,
						FColor::Green,
						FString::Printf(TEXT("Item ID %d가 위치 %s에 생성됨"), itemId, *spawnLocation.ToString())
					);
				}

				else
					GEngine->AddOnScreenDebugMessage
					(
						-1, 5.f,
						FColor::Red,
						FString::Printf(TEXT("아이템 ID %d 생성 실패"), itemId)
					);
			}

			// 아이템 클래스를 찾을 수 없는 경우 에러 메시지 출력
			else
				GEngine->AddOnScreenDebugMessage
				(
					-1, 5.f,
					FColor::Red,
					FString::Printf(TEXT("아이템 ID %d에 대한 클래스가 존재하지 않음"), itemId)
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

		Death();
	}

	FTransform spawnTransform = FTransform(FRotator::ZeroRotator, hitLocation);
	AFloatingDamage* newDamageWidget = GetWorld()->SpawnActorDeferred<AFloatingDamage>(damageParticle, spawnTransform);

	// BeginPlay 전에 damage 값을 설정
	newDamageWidget->SetDamageValue(damage);

	// 액터 스폰을 마무리하여 BeginPlay가 호출되도록 함
	UGameplayStatics::FinishSpawningActor(newDamageWidget, spawnTransform);
}