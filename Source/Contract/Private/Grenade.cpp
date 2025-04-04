#include "Grenade.h"
#include "PlayerInventory.h"

void AGrenade::BeginPlay()
{
	Super::BeginPlay();

	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AGrenade::AddGrenade);
}

void AGrenade::AddGrenade()
{
	if (!CheckPlayerIsClose())
		return;

	// �÷��̾� �κ��丮�� ����ź �߰�
	playerInventory->items.Add(AGrenade::StaticClass());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Got a grenade!"));

	Destroy();
}