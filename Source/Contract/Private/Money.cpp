#include "Money.h"
#include "PlayerInventory.h"

void AMoney::BeginPlay()
{
	Super::BeginPlay();

	// 플레이어와의 상호작용을 위한 입력 바인딩
	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AMoney::AddMoney);
}

void AMoney::AddMoney()
{
	if (!CheckPlayerIsClose())
		return;

	// 플레이어 인벤토리에 돈 추가
	playerInventory->money += addMoney;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Got a money! Current number of money: %d"), playerInventory->money));

	Destroy();
}
