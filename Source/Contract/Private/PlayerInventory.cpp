// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerInventory.h"
#include "IDToItem.h"

UPlayerInventory::UPlayerInventory()
{
    // IDToItemAsset �ε�
    FString assetPath = TEXT("DataAsset'/Game/PlayerInventory/IDToDataAsset.IDToDataAsset'");
    idToItemAsset = TSoftObjectPtr<UIDToItem>(FSoftObjectPath(assetPath));

    // �ʱ� �ε� �õ�
    if (!idToItemAsset.IsValid())
    {
        UIDToItem* loadedAsset = Cast<UIDToItem>(StaticLoadObject(UIDToItem::StaticClass(), nullptr, *assetPath));

        if (loadedAsset)
            idToItemAsset = loadedAsset;

        else
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("IDToItem ������ �ּ��� �ε��� �� �����ϴ�."));
    }

	// �κ��丮 �ʱ�ȭ
	itemCount.Empty();
	itemCount.Add(0, 0); // �⺻ ������ ID�� ���� �߰�
	itemCount.Add(1, 0);
	itemCount.Add(2, 0);
	itemCount.Add(3, 0);
}

int32 UPlayerInventory::GetItemCount(int32 itemID) const
{
    if (itemCount.Contains(itemID))
        return itemCount[itemID];

    return 0;
}

void UPlayerInventory::SetItemCount(int32 itemID, int32 count)
{
    itemCount.Add(itemID, count);
}

void UPlayerInventory::AddItem(int32 itemID, int32 count)
{
    int32 currentCount = GetItemCount(itemID);
    itemCount.Add(itemID, currentCount + count);
}

bool UPlayerInventory::RemoveItem(int32 itemID, int32 count)
{
    int32 currentCount = GetItemCount(itemID);

    if (currentCount >= count)
    {
        itemCount.Add(itemID, currentCount - count);

        return true;
    }

    return false;
}

TSubclassOf<AItem> UPlayerInventory::GetItemClassByID(int32 itemID) const
{
    // TSoftObjectPtr���� �ε� �õ�
    if (!idToItemAsset.IsValid())
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("IDToItem ������ �ּ��� �ε��� �� �����ϴ�."));

        return nullptr;
    }

    if (idToItemAsset->intToitems.Contains(itemID))
        return idToItemAsset->intToitems[itemID];

    return nullptr;
}

int32 UPlayerInventory::GetItemIDByClass(TSubclassOf<AItem> itemClass) const
{
    if (itemClass == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("������ Ŭ������ ��ȿ���� �ʽ��ϴ�."));

        return -1;
    }

    if (idToItemAsset.IsValid())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("IDToItem ������ �ּ��� �ε��� �� �����ϴ�."));

        return -1;
    }

    for (const auto& pair : idToItemAsset->intToitems)
    {
        if (pair.Value == itemClass)
            return pair.Key;
    }

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("������ Ŭ������ ã�� �� �����ϴ�."));

    return -1; // ã�� ����
}