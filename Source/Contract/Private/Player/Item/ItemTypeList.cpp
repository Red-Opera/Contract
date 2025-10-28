#include "Player/Item/ItemTypeList.h"

TArray<TSubclassOf<AActor>> UItemTypeList::GetItemsByType(EItemType type) const
{
	for (const FItemTypeData& data : itemTypeDataList)
	{
		if (data.itemType == type)
			return data.items;
	}

	return TArray<TSubclassOf<AActor>>();
}

int32 UItemTypeList::GetItemCountByType(EItemType type) const
{
	for (const FItemTypeData& data : itemTypeDataList)
	{
		if (data.itemType == type)
			return data.items.Num();
	}

	return 0;
}

