#include "Grenade.h"
#include "PlayerInventory.h"
#include "IDToItem.h"

void AGrenade::BeginPlay()
{
	Super::BeginPlay();

	if (playerInputComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 입력 컴포넌트가 존재하지 않습니다."));

		return;
	}

	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AGrenade::AddGrenade);
}

void AGrenade::UseItem()
{
	Super::UseItem();

	// 수류탄 사용 시 폭발 효과를 추가할 수 있습니다.
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("수류탄 사용!"));

	GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &AGrenade::Explosion, 3.0f, false);
}

void AGrenade::Explosion()
{
	RemoveItemMesh();

	// 폭발 효과를 위한 메쉬 스폰
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

	// 가장 가까운 상호작용 가능한 아이템을 찾기
	AItem* closestItem = AItem::GetClosestInteractableItem(player);

	// 이 아이템이 가장 가까운 아이템이 아니면 무시
	if (closestItem != this)
		return;

	// 플레이어 인벤토리에 수류탄 추가
	playerInventory->AddItem(0);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("수류탄 추가!"));

	Destroy();
}
