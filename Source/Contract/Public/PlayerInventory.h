#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Item.h"
#include "PlayerInventory.generated.h"

UCLASS()
class CONTRACT_API UPlayerInventory : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<TSubclassOf<AItem>> items;									// 인벤토리 아이템 배열

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	int32 bulletCount = 0;												// 총알 개수

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	int32 money = 0;													// 돈 개수
};
