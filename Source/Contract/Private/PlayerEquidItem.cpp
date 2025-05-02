// Fill out your copyright notice in the Description page of Project Settings.

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

	// �κ��丮 ������ �ε�
	LoadInventoryData();

	// �����ڰ� Character�� ��� �Է� ����
	ACharacter* ownerCharacter = Cast<ACharacter>(GetOwner());

	if (ownerCharacter)
	{
		// �⺻������ ĳ������ �޽� ������Ʈ�� Ÿ������ ����
		if (!targetSkeletalMesh)
		{
			targetSkeletalMesh = ownerCharacter->GetMesh();

			// TargetSkeletalMesh ���� ���θ� ����� �޽����� Ȯ��
			if (targetSkeletalMesh)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
					FString::Printf(TEXT("TargetSkeletalMesh Setting Complete: %s"), *targetSkeletalMesh->GetName()));

				// ���� ���� ���� Ȯ��
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
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TargetSkeletalMesh Not Complete: %s"), *targetSkeletalMesh->GetName()));
		}
	}

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("This is not Character Type. TargetSkeletalMesh Setting Disable!"));

	player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	playerController = GetWorld()->GetFirstPlayerController();

	if (player == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player does not exist."));
		return;
	}

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player controller does not exist."));
		return;
	}

	playerInputComponent = player->InputComponent;

	if (playerInputComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Input Component does not exist."));
		return;
	}

	// �÷��̾� �ִϸ��̼� ������
	playerAnimInstance = targetSkeletalMesh->GetAnimInstance();

	if (playerAnimInstance)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			FString::Printf(TEXT("Player Animation Instance: %s"), *playerAnimInstance->GetName()));
	}

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Animation Instance is null."));

	player->EnableInput(playerController);

	playerInputComponent->BindAction(TEXT("Item1"), IE_Pressed, this, &UPlayerEquidItem::SpawnGrenade);
	playerInputComponent->BindAction(TEXT("Item2"), IE_Pressed, this, &UPlayerEquidItem::SpawnMolotov);
	playerInputComponent->BindAction(TEXT("SmallHealPack"), IE_Pressed, this, &UPlayerEquidItem::SpawnSmallHealPack);
	playerInputComponent->BindAction(TEXT("LargeHealPack"), IE_Pressed, this, &UPlayerEquidItem::SpawnLargeHealPack);
	playerInputComponent->BindAction(TEXT("ThrowItem"), IE_Pressed, this, &UPlayerEquidItem::ThrowItemTrigger);
}

// Called every frame
void UPlayerEquidItem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (currentEquippedItem)
	{
		// ���� ������ �������� ���� ���� ó��
		FVector socketLocation = targetSkeletalMesh->GetSocketLocation(attachSocketName);
		FRotator socketRotation = targetSkeletalMesh->GetSocketRotation(attachSocketName);
		
		socketLocation -= targetSkeletalMesh->GetUpVector() * 4.0f; // ���� ��ġ�� �ణ�� ������ �߰� (�ʿ��)
		socketLocation -= targetSkeletalMesh->GetRightVector() * 9.0f; // ���� ��ġ�� �ణ�� ������ �߰� (�ʿ��)

		// ���� ��ġ�� ȸ������ ������ ��ġ ������Ʈ
		currentEquippedItem->SetActorLocation(socketLocation);
		currentEquippedItem->SetActorRotation(socketRotation);
		currentEquippedItem->SetActorScale3D(FVector(0.5f)); // ������ ���� (�ʿ��)
	}
}

void UPlayerEquidItem::LoadInventoryData()
{
	// �񵿱� �ε� ����
	if (InventoryDataAsset.IsValid())
		playerInventoryData = InventoryDataAsset.LoadSynchronous();

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryDataAsset is not valid!"));
}

void UPlayerEquidItem::ThrowItemTrigger()
{
	if (!currentEquippedItem)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No item equipped!"));

		return;
	}

	// �ִϸ��̼� isThrow bool ���� true�� ����
	FBoolProperty* boolProp = FindFProperty<FBoolProperty>(playerAnimInstance->GetClass(), TEXT("isThrow"));

	if (boolProp)
	{
		boolProp->SetPropertyValue_InContainer(playerAnimInstance, true);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Throw animation triggered!"));

		// ���� �����ӿ� false�� ����
		FTimerHandle timerHandle;
		player->GetWorld()->GetTimerManager().SetTimer(timerHandle, [this]()
			{
				FBoolProperty* boolProp = FindFProperty<FBoolProperty>(playerAnimInstance->GetClass(), TEXT("isThrow"));

				if (boolProp)
				{
					boolProp->SetPropertyValue_InContainer(playerAnimInstance, false);
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Throw animation reset!"));
				}
			},
			0.1f,
			false);
	}

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("isThrow property not found!"));

	GetWorld()->GetTimerManager().SetTimer(timerHandle_AutoThrow, this, &UPlayerEquidItem::ThrowItem, 0.79f, false);
}

void UPlayerEquidItem::ThrowItem()
{
	FVector throwDirection = playerController->PlayerCameraManager->GetActorForwardVector();

	currentEquippedItem->SetActorRotation(throwDirection.Rotation());
	currentEquippedItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	
	// ���� �ùķ��̼� Ȱ��ȭ
	UPrimitiveComponent* itemMesh = Cast<UPrimitiveComponent>(currentEquippedItem->GetRootComponent());

	if (itemMesh)
	{
		itemMesh->SetSimulatePhysics(true);
		itemMesh->SetEnableGravity(true);
		itemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		float throwForce = 1500.0f; // ������ ���� ������ �����ϴ� ��
		itemMesh->AddImpulse(throwDirection * throwForce);

		GEngine->AddOnScreenDebugMessage
		(
			-1, 5.f, FColor::Green, FString::Printf
				(
					TEXT("Throwing item with force: %f in direction: %s"),
					throwForce, 
					*throwDirection.ToString()
				)
		);
	}

	// ������ ���
	AItem* item = Cast<AItem>(currentEquippedItem);
	item->UseItem();

	currentEquippedItem = nullptr; // ������ ������ �ʱ�ȭ
}

bool UPlayerEquidItem::PlayerInventoryDataLoad()
{
	// �κ��丮�� �ε���� �ʾҴٸ� �ε�
	if (playerInventoryData == nullptr)
	{
		LoadInventoryData();

		if (playerInventoryData == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InventoryDataAsset is not valid!"));

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

	// �κ��丮�� ������ ��� ��ȸ
	if (playerInventoryData->itemCount[0] <= 0) 
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Inventory does not have a grenade!"));

		return;
	}

	// ������ ��� ó��
	ToggleItemEquip(4, grenadeClass);
}

void UPlayerEquidItem::SpawnMolotov()
{
	if (!PlayerInventoryDataLoad())
		return;

	// �κ��丮�� ������ ��� ��ȸ
	if (playerInventoryData->itemCount[1] <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Inventory does not have a Molotov Cocktail!"));

		return;
	}

	// ������ ��� ó��
	ToggleItemEquip(5, molotovClass);
}

void UPlayerEquidItem::SpawnSmallHealPack()
{
	if (!PlayerInventoryDataLoad())
		return;

	// �κ��丮�� ������ ��� ��ȸ
	if (playerInventoryData->itemCount[2] <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Inventory does not have a Small Heal Pack!"));

		return;
	}

	// ������ ��� ó��
	ToggleItemEquip(3, smallHealPackClass);
}

void UPlayerEquidItem::SpawnLargeHealPack()
{
	if (!PlayerInventoryDataLoad())
		return;

	// �κ��丮�� ������ ��� ��ȸ
	if (playerInventoryData->itemCount[3] <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Inventory does not have a grenade!"));

		return;
	}

	// ������ ��� ó��
	ToggleItemEquip(2, largeHealPackClass);
}

AActor* UPlayerEquidItem::SpawnItemAtSocket(TSubclassOf<AActor> itemClass, FName socketName)
{
	if (!targetSkeletalMesh)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Target Skeletal Mesh is not valid!"));
		return nullptr;
	}

	if (!itemClass)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Item Class is not valid!"));
		return nullptr;
	}

	// ���忡�� ������ ��ġ�� ȸ�� ���� ��������
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
		// ������ Ʈ�������� �����Ͽ� ��ġ�� �� ����
		FTransform socketTransform = FTransform(socketRotation, socketLocation);
		spawnItem->SetActorTransform(socketTransform);

		if (spawnItem->AttachToComponent(targetSkeletalMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, socketName))
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Spawned item at socket!"));
		
		else
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to attach item to socket!"));
		
		spawnItem->itemMesh->SetSimulatePhysics(false);
		spawnItem->itemMesh->SetEnableGravity(false);
		spawnItem->itemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to spawn item!"));
	}

	return spawnItem;
}

// ������ ���� �� ��� ó�� �Լ�
void UPlayerEquidItem::ToggleItemEquip(int itemSlotIndex, TSubclassOf<AActor> itemClass)
{
	// ���� ���� ������ �ε����� ���õǾ� �ִٸ� ���� ����
	if (currentEquippedItem && itemSelectIndex == itemSlotIndex)
	{
		DropCurrentItem();
		return;
	}

	// �κ��丮 ������ Ȯ��
	if (!PlayerInventoryDataLoad())
		return;

	// �̹� ������ �������� ������ ����
	if (currentEquippedItem)
	{
		currentEquippedItem->Destroy();
		currentEquippedItem = nullptr;
	}

	// �� ������ ����
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
			FString::Printf(TEXT("Equipped item with index: %d"), itemSlotIndex));
	}
}

// ���� ������ ������ ���� �Լ�
void UPlayerEquidItem::DropCurrentItem()
{
	if (!currentEquippedItem)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("No item equipped to remove!"));
		return;
	}

	// ������ Ŭ���� Ȯ��
	AItem* item = Cast<AItem>(currentEquippedItem);

	if (!item)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to cast equipped item!"));
		return;
	}

	// ���� ������ ������ �ı�
	currentEquippedItem->Destroy();
	currentEquippedItem = nullptr;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Item removed from inventory"));

	// ������ �ε��� �ʱ�ȭ
	itemSelectIndex = 0;
}


bool UPlayerEquidItem::IsItemEquipped() const
{
	return currentEquippedItem != nullptr;
}