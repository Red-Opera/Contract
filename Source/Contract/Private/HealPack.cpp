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

    // ���� ����� ��ȣ�ۿ� ������ �������� ã��
    AItem* closestItem = AItem::GetClosestInteractableItem(player);

    // �� �������� ���� ����� �������� �ƴϸ� ����
    if (closestItem != this)
        return;

    // �÷��̾� �κ��丮�� �� ������ �߰�
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

	// ���� �ùķ��̼� ��Ȱ��ȭ
	itemMesh->SetSimulatePhysics(false);

	// �޽� ǥ�� ��Ȱ��ȭ
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

    // ȸ�� Ÿ�Կ� ���� ȸ���� ���
    if (HealType == HealingType::Fixed)
        healAmount = fixedHealAmount;

    // PercentageBased
    else
        healAmount = maxHealth * healPercentage;

    // ���� ȸ�� ���� (�ִ� ü���� ���� �ʵ���)
    int newHealth = FMath::Min(currentHealth + healAmount, maxHealth);

    // ����� �޽����� ȸ�� ���� ���
	GEngine->AddOnScreenDebugMessage
    (
        -1, 5.f, FColor::Green,
		FString::Printf(TEXT("Healing applied: %d. Health: %d -> %d (Max: %d)"), healAmount, currentHealth, newHealth, maxHealth)
    );

    // ����Ʈ �� ���� ���
    if (healEffect)
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), healEffect, player->GetActorLocation(), FRotator::ZeroRotator, true);

    if (healSound)
        UGameplayStatics::PlaySoundAtLocation(this, healSound, player->GetActorLocation());

    // ���⼭ ������ ĳ������ ü���� ������Ʈ�ؾ� �մϴ�
    // player->UpdateHealth(newHealth); // ���� �ڵ�, ���� ���� �ʿ�

    currentHealth = newHealth;

    // ������ ��� �� ����
    Destroy();
}