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

	// 플레이어 인벤토리에 수류탄 추가
	playerInventory->items.Add(AGrenade::StaticClass());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Got a grenade!"));

	Destroy();
}