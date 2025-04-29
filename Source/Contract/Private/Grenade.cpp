#include "Grenade.h"
#include "PlayerInventory.h"

void AGrenade::BeginPlay()
{
	Super::BeginPlay();

	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AGrenade::AddGrenade);
}

void AGrenade::UseItem()
{
	// ����ź ��� �� ���� ȿ���� �߰��� �� �ֽ��ϴ�.
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Grenade used!"));

	GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &AGrenade::Explosion, 3.0f, false);
}

void AGrenade::Explosion()
{
	RemoveItemMesh();

	// ���� ȿ���� ���� �޽� ����
	if (explosionMesh)
	{
		FVector spawnLocation = GetActorLocation();
		FRotator spawnRotation = GetActorRotation();
		explosionActor = GetWorld()->SpawnActor<AActor>(explosionMesh, spawnLocation, spawnRotation);
	}
	
	GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &AGrenade::RemoveGrenade, 5.0f, false);
}

void AGrenade::RemoveGrenade()
{
	if (explosionActor)
	{
		explosionActor->Destroy();
		explosionActor = nullptr;
	}

	Destroy();
}

void AGrenade::AddGrenade()
{
	if (!CheckPlayerIsClose() || !isGetable)
		return;

	// ���� ����� ��ȣ�ۿ� ������ �������� ã��
	AItem* closestItem = AItem::GetClosestInteractableItem(player);

	// �� �������� ���� ����� �������� �ƴϸ� ����
	if (closestItem != this)
		return;

	// �÷��̾� �κ��丮�� ����ź �߰�
	playerInventory->items.Add(AGrenade::StaticClass());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Got a grenade!"));

	Destroy();
}
