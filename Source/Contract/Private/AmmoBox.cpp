#include "AmmoBox.h"
#include "PlayerInventory.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AAmmoBox::AAmmoBox()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAmmoBox::BeginPlay()
{
	Super::BeginPlay();

	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AAmmoBox::AddAmmo);
}

void AAmmoBox::AddAmmo()
{
    if (!CheckPlayerIsClose() || !isGetable)
        return;

    // 가장 가까운 상호작용 가능한 아이템을 찾기
    AItem* closestItem = AItem::GetClosestInteractableItem(player);

    // 이 아이템이 가장 가까운 아이템이 아니면 무시
    if (closestItem != this)
        return;

    // 플레이어 인벤토리에 총알 추가
    playerInventory->bulletCount += ammoCount;
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Got a bullet! Current number of bullets: %d"), playerInventory->bulletCount));

    Destroy();
}


// Called every frame
void AAmmoBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}