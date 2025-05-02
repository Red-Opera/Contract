#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Item.h"
#include "IDToItem.h"
#include "PlayerInventory.generated.h"

UCLASS()
class CONTRACT_API UPlayerInventory : public UDataAsset
{
	GENERATED_BODY()

public:
	// �⺻ ������
	UPlayerInventory();

	// ������ ID�� ������ ���� ��������
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemCount(int32 ItemID) const;

	// ������ ID�� ������ ���� �����ϱ�
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemCount(int32 ItemID, int32 Count);

	// ������ ID�� ������ �߰��ϱ�
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(int32 ItemID, int32 Count = 1);

	// ������ ID�� ������ �����ϱ�
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(int32 ItemID, int32 Count = 1);

	// ������ ID�� Ŭ������ ��ȯ
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TSubclassOf<AItem> GetItemClassByID(int32 ItemID) const;

	// ������ Ŭ������ ID�� ��ȯ
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemIDByClass(TSubclassOf<AItem> itemClass) const;

	// ������ ���� ������ �ּ� ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSoftObjectPtr<UIDToItem> idToItemAsset;

	// ������ ���� �� (������ ID�� Ű�� ���)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TMap<int32, int32> itemCount;

	// �Ѿ� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	int32 bulletCount = 0;

	// �� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	int32 money = 0;
};
