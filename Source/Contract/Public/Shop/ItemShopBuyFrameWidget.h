#pragma once

#include "CoreMinimal.h"
#include "ItemShopBuyButtonWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScrollBox.h"
#include "Player/Item/ItemTypeList.h"
#include "ItemShopBuyFrameWidget.generated.h"

UCLASS()
class CONTRACT_API UItemShopBuyFrameWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UPROPERTY(meta = (BindWidget))
	UScrollBox* itemListScrollBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UItemShopBuyButtonWidget> itemButtonWidgetClass;

	// ItemTypeList 데이터 에셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	UItemTypeList* itemTypeList;

	// 카테고리별 아이템 목록 업데이트
	UFUNCTION(BlueprintCallable, Category = "ItemShop")
	void UpdateItemListByCategory(int32 categoryIndex);

	// 현재 선택된 카테고리 인덱스
	UPROPERTY(BlueprintReadOnly, Category = "ItemShop")
	int32 currentCategoryIndex;

	// 싱글톤 접근자
	static UItemShopBuyFrameWidget* GetInstance();

private:
	void ClearItemList();
	void PopulateItemList(int32 categoryIndex);
	
	// 카테고리 인덱스를 EItemType으로 변환
	EItemType GetItemTypeFromIndex(int32 index) const;

	// 싱글톤 인스턴스
	static UItemShopBuyFrameWidget* instance;
};
