#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "ItemType.h"
#include "Engine/DataAsset.h"
#include "ItemTypeList.generated.h"

// 아이템 타입별 목록을 저장하는 구조체
USTRUCT(BlueprintType)
struct FItemTypeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemType itemType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TArray<TSubclassOf<AActor>> items;
};

UCLASS()
class CONTRACT_API UItemTypeList : public UDataAsset
{
	GENERATED_BODY()

public:
	// 모든 아이템 타입별 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TArray<FItemTypeData> itemTypeDataList;

	// 특정 타입의 아이템 목록 가져오기
	UFUNCTION(BlueprintCallable, Category = "Item")
	TArray<TSubclassOf<AActor>> GetItemsByType(EItemType type) const;

	// 특정 타입의 아이템 개수 가져오기
	UFUNCTION(BlueprintCallable, Category = "Item")
	int32 GetItemCountByType(EItemType type) const;
};
