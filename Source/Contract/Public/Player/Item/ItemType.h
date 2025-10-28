#pragma once

#include "CoreMinimal.h"
#include "ItemType.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Rifle		UMETA(DisplayName = "Rifle"),
	HandGun		UMETA(DisplayName = "Hand Gun"),
	Item		UMETA(DisplayName = "Item"),
	Armor		UMETA(DisplayName = "Armor"),
	Consumable	UMETA(DisplayName = "Consumable")
};