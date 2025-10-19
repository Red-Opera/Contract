#include "Shop/ItemShopStartButtonWidget.h"
#include "Shop/ItemShopWidget.h"
#include "Blueprint/UserWidget.h"

void UItemShopStartButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (mainButton == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 메인 버튼 위젯이 바인딩되지 않았습니다."));

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

	if (itemShopWidgetClass == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 아이템 상점 위젯 클래스가 설정되지 않았습니다."));

		return;
	}

	// ItemShopWidget 열기
	if (itemShopWidgetInstance == nullptr)
		itemShopWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), itemShopWidgetClass);

	if (itemShopWidgetInstance == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 아이템 상점 위젯 인스턴스 생성에 실패했습니다."));

		return;
	}

	// 현재 위젯(ItemShopWidgetStart) 숨기기
	if (IsInViewport())
	{
		RemoveFromParent();
	}

	itemShopWidgetInstance->AddToViewport();
}