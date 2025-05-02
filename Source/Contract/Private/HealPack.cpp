#include "HealPack.h"
#include "PlayerInventory.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

int AHealPack::currentHealth = 75.0f;

void AHealPack::AddHealPack()
{
    if (!CheckPlayerIsClose() || !isGetable)
        return;

    // 가장 가까운 상호작용 가능한 아이템을 찾기
    AItem* closestItem = AItem::GetClosestInteractableItem(player);

    // 이 아이템이 가장 가까운 아이템이 아니면 무시
    if (closestItem != this)
        return;

    // 플레이어 인벤토리에 힐 아이템 추가
    playerInventory->AddItem(healItemID);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Heal Pack added to inventory!"));

    Destroy();
}

void AHealPack::BeginPlay()
{
    Super::BeginPlay();

    playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AHealPack::AddHealPack);
}

void AHealPack::RemoveHealPack()
{
	if (itemMesh == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Item mesh does not exist!"));
		return;
	}

	// 물리 시뮬레이션 비활성화
	itemMesh->SetSimulatePhysics(false);

	// 메시 표시 비활성화
	itemMesh->SetVisibility(false);
	itemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Destroy();
}

void AHealPack::UseItem()
{
    if (!player)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player character not found!"));
        return;
    }

    Super::UseItem();

    int healAmount = 0;

    // 회복 타입에 따라 회복량 계산
    if (HealType == HealingType::Fixed)
        healAmount = fixedHealAmount;

    // PercentageBased
    else
        healAmount = maxHealth * healPercentage;

    // 실제 회복 적용 (최대 체력을 넘지 않도록)
    int newHealth = FMath::Min(currentHealth + healAmount, maxHealth);

    // 디버그 메시지로 회복 정보 출력
	GEngine->AddOnScreenDebugMessage
    (
        -1, 5.f, FColor::Green,
		FString::Printf(TEXT("Healing applied: %d. Health: %d -> %d (Max: %d)"), healAmount, currentHealth, newHealth, maxHealth)
    );

    // 이펙트 및 사운드 재생
    if (healEffect)
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), healEffect, player->GetActorLocation(), FRotator::ZeroRotator, true);

    if (healSound)
        UGameplayStatics::PlaySoundAtLocation(this, healSound, player->GetActorLocation());

    // 여기서 실제로 캐릭터의 체력을 업데이트해야 합니다
    // player->UpdateHealth(newHealth); // 예시 코드, 실제 구현 필요

    currentHealth = newHealth;

    // 아이템 사용 후 제거
    Destroy();
}