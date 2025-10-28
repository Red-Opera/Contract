#include "Shop/ItemShopCategoryButtonWidget.h"
#include "Shop/ItemShopBuyFrameWidget.h"

void UItemShopCategoryButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (categoryButton)
	{
		categoryButton->OnClicked.AddDynamic(this, &UItemShopCategoryButtonWidget::OnCategoryButtonClicked);
	}
}

void UItemShopCategoryButtonWidget::SetListIndex(int32 index)
{
	listIndex = index;
}

void UItemShopCategoryButtonWidget::OnCategoryButtonClicked()
{
	// 싱글톤 인스턴스를 통해 ItemShopBuyFrameWidget에 접근
	UItemShopBuyFrameWidget* buyFrame = UItemShopBuyFrameWidget::GetInstance();

	if (buyFrame == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
			TEXT("Error (Null Reference) : ItemShopBuyFrameWidget 인스턴스를 찾을 수 없습니다."));

		return;
	}

	buyFrame->UpdateItemListByCategory(listIndex);
}