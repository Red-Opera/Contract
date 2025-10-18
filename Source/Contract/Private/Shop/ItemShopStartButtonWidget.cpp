#include "Shop/ItemShopStartButtonWidget.h"

void UItemShopStartButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (mainButton == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Reference Error) : 메인 버튼 위젯이 바인딩되지 않았습니다."));

		return;
	}
	
	mainButton->OnClicked.AddDynamic(this, &UItemShopStartButtonWidget::OnMainButtonClicked);
}

void UItemShopStartButtonWidget::SetButtonIndex(int32 index)
{
	buttonIndex = index;
}

void UItemShopStartButtonWidget::OnMainButtonClicked()
{
	FString message = FString::Printf(TEXT("Button Index %d clicked!"), buttonIndex);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, message);
}

