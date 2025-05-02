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
	// 기본 생성자
	UPlayerInventory();

	// 아이템 ID로 아이템 개수 가져오기
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemCount(int32 ItemID) const;

	// 아이템 ID로 아이템 개수 설정하기
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemCount(int32 ItemID, int32 Count);

	// 아이템 ID로 아이템 추가하기
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(int32 ItemID, int32 Count = 1);

	// 아이템 ID로 아이템 제거하기
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(int32 ItemID, int32 Count = 1);

	// 아이템 ID를 클래스로 변환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TSubclassOf<AItem> GetItemClassByID(int32 ItemID) const;

	// 아이템 클래스를 ID로 변환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemIDByClass(TSubclassOf<AItem> itemClass) const;

	// 아이템 매핑 데이터 애셋 참조
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSoftObjectPtr<UIDToItem> idToItemAsset;

	// 아이템 개수 맵 (아이템 ID를 키로 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TMap<int32, int32> itemCount;

	// 총알 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	int32 bulletCount = 0;

	// 돈 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	int32 money = 0;
};
