#include "Shop/ItemShopBuyFrameWidget.h"
#include "Shop/ItemShopBuyButtonWidget.h"
#include "Player/Item/ItemType.h"

// 정적 변수 초기화
UItemShopBuyFrameWidget* UItemShopBuyFrameWidget::instance = nullptr;

void UItemShopBuyFrameWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 싱글톤 인스턴스 등록
	instance = this;
	
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, 
		TEXT("ItemShopBuyFrameWidget 인스턴스 등록 완료"));
	
	currentCategoryIndex = 0;
	UpdateItemListByCategory(0);
}

void UItemShopBuyFrameWidget::NativeDestruct()
{
	// 인스턴스 해제
	if (instance == this)
	{
		instance = nullptr;

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("ItemShopBuyFrameWidget 인스턴스 해제"));
	}
	
	Super::NativeDestruct();
}

UItemShopBuyFrameWidget* UItemShopBuyFrameWidget::GetInstance()
{
	return instance;
}

void UItemShopBuyFrameWidget::UpdateItemListByCategory(int32 categoryIndex)
{
	currentCategoryIndex = categoryIndex;
	
	ClearItemList();
	PopulateItemList(categoryIndex);
}

void UItemShopBuyFrameWidget::ClearItemList()
{
	if (itemListScrollBox == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : itemListScrollBox가 할당되지 않았습니다."));

		return;
	}

	itemListScrollBox->ClearChildren();
}

void UItemShopBuyFrameWidget::PopulateItemList(int32 categoryIndex)
{
	if (itemListScrollBox == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : itemListScrollBox가 할당되지 않았습니다."));

		return;
	}

	if (itemButtonWidgetClass == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : itemButtonWidgetClass가 할당되지 않았습니다."));

		return;
	}

	if (itemTypeList == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : itemTypeList가 할당되지 않았습니다."));

		return;
	}

	TArray<TSubclassOf<AActor>> items;

	// 0번 인덱스는 전체 아이템 표시
	if (categoryIndex == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("전체 아이템 로드 중..."));

		// 모든 타입의 아이템을 합치기
		items.Append(itemTypeList->GetItemsByType(EItemType::Rifle));
		items.Append(itemTypeList->GetItemsByType(EItemType::HandGun));
		items.Append(itemTypeList->GetItemsByType(EItemType::Item));
		items.Append(itemTypeList->GetItemsByType(EItemType::Armor));
		items.Append(itemTypeList->GetItemsByType(EItemType::Consumable));
	}
	else
	{
		// 카테고리 인덱스를 EItemType으로 변환 (1번부터 시작)
		EItemType itemType = GetItemTypeFromIndex(categoryIndex);
		items = itemTypeList->GetItemsByType(itemType);
	}

	int32 itemCount = items.Num();

	if (itemCount == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, 
			TEXT("해당 카테고리에 아이템이 없습니다."));
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
		FString::Printf(TEXT("카테고리 %d - 로드된 아이템 개수: %d"), categoryIndex, itemCount));

	// 각 아이템에 대해 버튼 생성
	for (int32 i = 0; i < itemCount; ++i)
	{
		UItemShopBuyButtonWidget* itemButton = CreateWidget<UItemShopBuyButtonWidget>(GetWorld(), itemButtonWidgetClass);

		if (itemButton == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				FString::Printf(TEXT("Error (Null Reference) : 아이템 버튼 위젯 생성 실패 - 인덱스 %d"), i));
			continue;
		}

		// 아이템 정보 설정
		itemButton->SetItemInfo(items[i]);
		itemListScrollBox->AddChild(itemButton);
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
		FString::Printf(TEXT("아이템 버튼 생성 완료 - 총 %d개"), itemCount));
}

EItemType UItemShopBuyFrameWidget::GetItemTypeFromIndex(int32 index) const
{
	// 카테고리 인덱스를 EItemType으로 매핑
	// 0: 전체, 1: Rifle, 2: HandGun, 3: Item, 4: Armor, 5: Consumable
	switch (index)
	{
	case 1: return EItemType::Rifle;
	case 2: return EItemType::HandGun;
	case 3: return EItemType::Item;
	case 4: return EItemType::Armor;
	case 5: return EItemType::Consumable;

	default: 
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
			FString::Printf(TEXT("Error (Invalid Index) : 잘못된 카테고리 인덱스 %d, 기본값 Item 반환"), index));

		return EItemType::Item;
	}
}