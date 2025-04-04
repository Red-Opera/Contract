#include "Money.h"
#include "PlayerInventory.h"

void AMoney::BeginPlay()
{
	Super::BeginPlay();

	// �÷��̾���� ��ȣ�ۿ��� ���� �Է� ���ε�
	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AMoney::AddMoney);
}

void AMoney::AddMoney()
{
	if (!CheckPlayerIsClose())
		return;

	// �÷��̾� �κ��丮�� �� �߰�
	playerInventory->money += addMoney;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Got a money! Current number of money: %d"), playerInventory->money));

	Destroy();
}
