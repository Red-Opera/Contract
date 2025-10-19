#include "Shop/ItemShopWidget.h"
#include "Shop/ItemShop.h"

void UItemShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// ItemShop에 ItemShopWidget이 열렸음을 알림
	AItemShop::SetItemShopWidgetOpen(true);

	// ItemShop의 ESC 바인딩 비활성화
	AItemShop::EnableESCBinding(false);

	// 입력 설정
	SetupInput();
}

void UItemShopWidget::NativeDestruct()
{
	// ItemShop에 ItemShopWidget이 닫혔음을 알림
	AItemShop::SetItemShopWidgetOpen(false);

	// ItemShop의 ESC 바인딩 재활성화
	AItemShop::EnableESCBinding(true);

	// 입력 비활성화
	if (playerController)
		playerController->PopInputComponent(InputComponent);

	Super::NativeDestruct();
}

void UItemShopWidget::SetupInput()
{
	playerController = GetWorld()->GetFirstPlayerController();

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 플레이어 컨트롤러를 찾을 수 없습니다."));

		return;
	}

	if (!InputComponent)
	{
		InputComponent = NewObject<UInputComponent>(this);
		InputComponent->bBlockInput = false;
	}

	InputComponent->BindAction("ESC", IE_Pressed, this, &UItemShopWidget::OnESCPressed);
	playerController->PushInputComponent(InputComponent);
}

void UItemShopWidget::OnESCPressed()
{
	// 현재 위젯 닫기
	if (IsInViewport())
		RemoveFromParent();
}