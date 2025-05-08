﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerEquidItem.h"
#include "HealPack.h"
#include "Grenade.h"
#include "MolotovCocktail.h"
#include "Item.h"

#include "Engine/AssetManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"

UPlayerEquidItem::UPlayerEquidItem()
{
	PrimaryComponentTick.bCanEverTick = true;
}

int UPlayerEquidItem::GetSelectedItemIndex() const
{
	return itemSelectIndex;
}

void UPlayerEquidItem::BeginPlay()
{
	Super::BeginPlay();

	// 인벤토리 데이터 로드
	LoadInventoryData();

	// 소유자가 Character인 경우 입력 설정
	ACharacter* ownerCharacter = Cast<ACharacter>(GetOwner());

	if (ownerCharacter)
	{
		// 기본적으로 캐릭터의 메시 컴포넌트를 타겟으로 설정
		if (!targetSkeletalMesh)
		{
			targetSkeletalMesh = ownerCharacter->GetMesh();

			// TargetSkeletalMesh 설정 여부를 디버그 메시지로 확인
			if (targetSkeletalMesh)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
					FString::Printf(TEXT("타겟 스켈레탈 메시 설정됨: %s"), *targetSkeletalMesh->GetName()));

				// 소켓 존재 여부 확인
				bool bSocketExists = targetSkeletalMesh->DoesSocketExist(attachSocketName);
				GEngine->AddOnScreenDebugMessage(
					-1,
					5.f,
					FColor::Green,
					FString::Printf(
						TEXT("Socket '%s' is Exist : %s"),
						*attachSocketName.ToString(),
						bSocketExists ? TEXT("Yes") : TEXT("No")
					)
				);
			}

			else
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("타겟 스켈레탈 메시가 null")));
		}
	}

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("소유자가 캐릭터가 아님"));

	player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	playerController = GetWorld()->GetFirstPlayerController();

	if (player == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player가 존재하지 않음"));
		return;
	}

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player controller가 존재하지 않음"));
		return;
	}

	playerInputComponent = player->InputComponent;

	if (playerInputComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Input Component가 존재하지 않음"));
		return;
	}

	// 플레이어 애니메이션 가져옴
	playerAnimInstance = targetSkeletalMesh->GetAnimInstance();

	if (playerAnimInstance)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			FString::Printf(TEXT("Player 애니메이션 인스턴스 설정됨: %s"), *playerAnimInstance->GetName()));
	}

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Animation Instance가 null"));

	player->EnableInput(playerController);


	playerInputComponent->BindAction(TEXT("Key1"), IE_Pressed, this, &UPlayerEquidItem::Press1Key);
	playerInputComponent->BindAction(TEXT("Key2"), IE_Pressed, this, &UPlayerEquidItem::Press2Key);
	playerInputComponent->BindAction(TEXT("Key3"), IE_Pressed, this, &UPlayerEquidItem::Press3Key);
	playerInputComponent->BindAction(TEXT("Key4"), IE_Pressed, this, &UPlayerEquidItem::Press4Key);
	playerInputComponent->BindAction(TEXT("Key5"), IE_Pressed, this, &UPlayerEquidItem::Press5Key);
	playerInputComponent->BindAction(TEXT("ThrowItem"), IE_Pressed, this, &UPlayerEquidItem::ThrowItemTrigger);
}

// Called every frame
void UPlayerEquidItem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (currentEquippedItem)
	{
		// 현재 장착된 아이템이 있을 때의 처리
		FVector socketLocation = targetSkeletalMesh->GetSocketLocation(attachSocketName);
		FRotator socketRotation = targetSkeletalMesh->GetSocketRotation(attachSocketName);
		
		socketLocation -= targetSkeletalMesh->GetUpVector() * 4.0f; // 소켓 위치에 약간의 오프셋 추가 (필요시)
		socketLocation -= targetSkeletalMesh->GetRightVector() * 9.0f; // 소켓 위치에 약간의 오프셋 추가 (필요시)

		// 소켓 위치와 회전으로 아이템 위치 업데이트
		currentEquippedItem->SetActorLocation(socketLocation);
		currentEquippedItem->SetActorRotation(socketRotation);
		currentEquippedItem->SetActorScale3D(FVector(0.5f)); // 스케일 조정 (필요시)
	}
}

void UPlayerEquidItem::Press1Key()
{
	pressedKey = 1;

	SpawnSmallHealPack();
}

void UPlayerEquidItem::Press2Key()
{
	pressedKey = 2;

	SpawnLargeHealPack();
}

void UPlayerEquidItem::Press3Key()
{
	pressedKey = 3;

	if (itemSelectIndex != 3)
		itemSelectIndex = 3;

	else
	{
		itemSelectIndex = 0;
		DropCurrentItem();
	}
}

void UPlayerEquidItem::Press4Key()
{
	pressedKey = 4;

	SpawnGrenade();
}

void UPlayerEquidItem::Press5Key()
{
	pressedKey = 5;

	SpawnMolotov();
}

void UPlayerEquidItem::LoadInventoryData()
{
	// 비동기 로드 시작
	if (InventoryDataAsset.IsValid())
		playerInventoryData = InventoryDataAsset.LoadSynchronous();

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryDataAsset이 유효하지 않음"));
}

void UPlayerEquidItem::ThrowItemTrigger()
{
	if (!currentEquippedItem)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("아이템이 장착되어 있지 않음"));

		return;
	}

	// 애니메이션 isThrow bool 변수 true로 설정
	FBoolProperty* boolProp = FindFProperty<FBoolProperty>(playerAnimInstance->GetClass(), TEXT("isThrow"));

	if (boolProp)
	{
		boolProp->SetPropertyValue_InContainer(playerAnimInstance, true);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("던지는 애니메이션 시작!"));

		// 다음 프레임에 false로 설정
		FTimerHandle timerHandle;
		player->GetWorld()->GetTimerManager().SetTimer(timerHandle, [this]()
			{
				FBoolProperty* boolProp = FindFProperty<FBoolProperty>(playerAnimInstance->GetClass(), TEXT("isThrow"));

				if (boolProp)
				{
					boolProp->SetPropertyValue_InContainer(playerAnimInstance, false);
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("던지는 애니메이션 종료!"));
				}
			},
			0.1f,
			false);
	}

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("isThrow 변수를 찾을 수 없음"));

	GetWorld()->GetTimerManager().SetTimer(timerHandle_AutoThrow, this, &UPlayerEquidItem::ThrowItem, 0.79f, false);
}

void UPlayerEquidItem::ThrowItem()
{
	FVector throwDirection = playerController->PlayerCameraManager->GetActorForwardVector();

	currentEquippedItem->SetActorRotation(throwDirection.Rotation());
	currentEquippedItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	
	// 물리 시뮬레이션 활성화
	UPrimitiveComponent* itemMesh = Cast<UPrimitiveComponent>(currentEquippedItem->GetRootComponent());

	if (itemMesh)
	{
		itemMesh->SetSimulatePhysics(true);
		itemMesh->SetEnableGravity(true);
		itemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		float throwForce = 1500.0f; // 던지는 힘의 강도를 조절하는 값
		itemMesh->AddImpulse(throwDirection * throwForce);

		GEngine->AddOnScreenDebugMessage
		(
			-1, 5.f, FColor::Green, FString::Printf
				(
					TEXT("던지는 힘 : %f, 방향: %s"),
					throwForce, 
					*throwDirection.ToString()
				)
		);
	}

	// 아이템 사용
	AItem* item = Cast<AItem>(currentEquippedItem);
	item->UseItem();

	currentEquippedItem = nullptr; // 장착된 아이템 초기화
}

bool UPlayerEquidItem::PlayerInventoryDataLoad()
{
	// 인벤토리가 로드되지 않았다면 로드
	if (playerInventoryData == nullptr)
	{
		LoadInventoryData();

		if (playerInventoryData == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryDataAsset이 유효하지 않음"));

			return false;
		}

		return true;
	}

	return true;
}

void UPlayerEquidItem::SpawnGrenade()
{
	if (!PlayerInventoryDataLoad())
		return;

	// 인벤토리의 아이템 목록 순회
	if (playerInventoryData->itemCount[0] <= 0) 
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Inventory가 수류탄을 가지고 있지 않음!"));

		return;
	}

	// 아이템 토글 처리
	ToggleItemEquip(4, grenadeClass);
}

void UPlayerEquidItem::SpawnMolotov()
{
	if (!PlayerInventoryDataLoad())
		return;

	// 인벤토리의 아이템 목록 순회
	if (playerInventoryData->itemCount[1] <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Inventory가 화염병을 가지고 있지 않음!"));

		return;
	}

	// 아이템 토글 처리
	ToggleItemEquip(5, molotovClass);
}

void UPlayerEquidItem::SpawnSmallHealPack()
{
	if (!PlayerInventoryDataLoad() || itemSelectIndex != 3)
		return;

	// 인벤토리의 아이템 목록 순회
	if (playerInventoryData->itemCount[2] <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Inventory가 소형 힐 팩을 가지고 있지 않음!"));

		return;
	}

	// 아이템 토글 처리
	ToggleItemEquip(6, smallHealPackClass);
}

void UPlayerEquidItem::SpawnLargeHealPack()
{
	if (!PlayerInventoryDataLoad() || itemSelectIndex != 3)
		return;

	// 인벤토리의 아이템 목록 순회
	if (playerInventoryData->itemCount[3] <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Inventory가 대형 힐 팩을 가지고 있지 않음!"));

		return;
	}

	// 아이템 토글 처리
	ToggleItemEquip(7, largeHealPackClass);
}

AActor* UPlayerEquidItem::SpawnItemAtSocket(TSubclassOf<AActor> itemClass, FName socketName)
{
	if (!targetSkeletalMesh)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Target Skeletal Mesh이 유효하지 않음"));
		return nullptr;
	}

	if (!itemClass)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Item Class가 유효하지 않음"));
		return nullptr;
	}

	// 월드에서 소켓의 위치와 회전 정보 가져오기
	FVector socketLocation = targetSkeletalMesh->GetSocketLocation(socketName);
	FRotator socketRotation = targetSkeletalMesh->GetSocketRotation(socketName);

	UWorld* world = GetWorld();

	if (!world)
		return nullptr;

	FActorSpawnParameters spawnParams;
	spawnParams.Owner = GetOwner();
	AItem* spawnItem = world->SpawnActor<AItem>(itemClass, socketLocation, socketRotation, spawnParams);

	if (spawnItem)
	{
		// 소켓의 트랜스폼을 적용하여 배치한 후 부착
		FTransform socketTransform = FTransform(socketRotation, socketLocation);
		spawnItem->SetActorTransform(socketTransform);

		if (spawnItem->AttachToComponent(targetSkeletalMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, socketName))
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("아이템이 소켓에 부착됨!"));
		
		else
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("아이템이 소켓에 부착되지 않음!"));
		
		spawnItem->itemMesh->SetSimulatePhysics(false);
		spawnItem->itemMesh->SetEnableGravity(false);
		spawnItem->itemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("아이템 스폰 실패!"));

	return spawnItem;
}

// 아이템 선택 및 토글 처리 함수
void UPlayerEquidItem::ToggleItemEquip(int itemSlotIndex, TSubclassOf<AActor> itemClass)
{
	// 현재 같은 아이템 인덱스가 선택되어 있다면 파지 해제
	if (currentEquippedItem && itemSelectIndex == itemSlotIndex)
	{
		DropCurrentItem();
		return;
	}

	// 인벤토리 데이터 확인
	if (!PlayerInventoryDataLoad())
		return;

	// 이미 장착된 아이템이 있으면 제거
	if (currentEquippedItem)
	{
		currentEquippedItem->Destroy();
		currentEquippedItem = nullptr;
	}

	// 새 아이템 스폰
	if (itemClass != nullptr)
	{
		currentEquippedItem = SpawnItemAtSocket(itemClass, attachSocketName);

		AItem* item = Cast<AItem>(currentEquippedItem);
		if (item)
		{
			item->SetGetable(false);
		}

		itemSelectIndex = itemSlotIndex;

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			FString::Printf(TEXT("장착된 아이템 : %s"), *currentEquippedItem->GetName()));
	}
}

// 현재 장착된 아이템 제거 함수
void UPlayerEquidItem::DropCurrentItem()
{
	if (!currentEquippedItem)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("아이템이 장착되어 있지 않음"));
		return;
	}

	// 아이템 클래스 확인
	AItem* item = Cast<AItem>(currentEquippedItem);

	if (!item)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("장착된 아이템이 AItem이 아님"));
		return;
	}

	// 현재 장착된 아이템 파괴
	currentEquippedItem->Destroy();
	currentEquippedItem = nullptr;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("아이템이 파괴됨"));

	// 아이템 인덱스 초기화
	itemSelectIndex = 0;
}


bool UPlayerEquidItem::IsItemEquipped() const
{
	return currentEquippedItem != nullptr;
}