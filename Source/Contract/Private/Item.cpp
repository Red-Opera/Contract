#include "Item.h"
#include "PlayerInventory.h"
#include "PlayerEquidItem.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

bool AItem::isInteractionInProgress = false;

AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = true;

	itemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = itemMesh;

	// mesh 중력 활성화
	itemMesh->SetSimulatePhysics(true);
	itemMesh->SetEnableGravity(true);

	FString assetPath = TEXT("DataAsset'/Game/PlayerInventory/PlayerInventory.PlayerInventory'");
	playerInventory = Cast<UPlayerInventory>(StaticLoadObject(UPlayerInventory::StaticClass(), nullptr, *assetPath));
}

void AItem::RemoveItemMesh()
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
}

void AItem::BeginPlay()
{
	Super::BeginPlay();
	
	player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	playerController = GetWorld()->GetFirstPlayerController();

	// playerData가 null인지 확인
	if (playerInventory == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Inventory is null! Check asset path."));
		return;
	}

	if (player == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player does not exist."));
		return;
	}

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player controller does not exist."));
		return;
	}

	playerInputComponent = player->InputComponent;

	if (playerInputComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Input Component does not exist."));
		return;
	}

	EnableInput(playerController);
}

bool AItem::CheckPlayerIsClose()
{
	if (player == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player does not exist."));
		return false;
	}

	// 플레이어와 해당 오브젝트 위치를 구함
	FVector position = GetActorLocation();
	FVector playerPosition = player->GetActorLocation();

	// 플레이어와 해당 오브젝트 거리를 구함
	float distance = FVector::Dist(position, playerPosition);

	return distance <= interactionDistance;
}

float AItem::GetDistanceToPlayer() const
{
	if (player == nullptr)
		return MAX_FLT;

	FVector position = GetActorLocation();
	FVector playerPosition = player->GetActorLocation();

	return FVector::Dist(position, playerPosition);
}

void AItem::SetGetable(bool getable)
{
	this->isGetable = getable;
}

AItem* AItem::GetClosestInteractableItem(ACharacter* fromCharacter)
{
	if (fromCharacter == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("FromCharacter is null!"));
		return nullptr;
	}

	// 이미 상호작용 처리 중이면 시간 확인 후 처리
	static float lastInteractionTime = 0.0f;
	float currentTime = fromCharacter->GetWorld()->GetTimeSeconds();

	if (isInteractionInProgress && currentTime - lastInteractionTime < 0.15f)
		return nullptr;

	// 모든 Item 액터 찾기
	TArray<AActor*> foundItems;
	UGameplayStatics::GetAllActorsOfClass(fromCharacter->GetWorld(), AItem::StaticClass(), foundItems);

	AItem* closestItem = nullptr;
	float closestDistance = MAX_FLT;

	// 획득 가능하고 거리 내에 있는 가장 가까운 아이템 찾기
	for (AActor* actor : foundItems)
	{
		AItem* item = Cast<AItem>(actor);

		if (item != nullptr && item->isGetable)
		{
			float Distance = item->GetDistanceToPlayer();

			if (Distance <= item->interactionDistance && Distance < closestDistance)
			{
				closestDistance = Distance;
				closestItem = item;
			}
		}
	}

	if (closestItem == nullptr)
		return nullptr;

	// 가장 가까운 아이템이 있으면 상호작용 처리 중으로 설정
	isInteractionInProgress = true;
	lastInteractionTime = currentTime;

	// 잠시 후 상호작용 처리 상태 초기화
	FTimerHandle unlockTimerHandle;
	fromCharacter->GetWorld()->GetTimerManager().SetTimer
	(
		unlockTimerHandle,
		[]() { isInteractionInProgress = false; },
		0.15f, // 이전보다 약간 더 긴 딜레이로 조정
		false
	);

	return closestItem;
}


void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AItem::UseItem()
{
	// 플레이어에서 PlayerEquidItem 컴포넌트 가져오기
	if (player)
	{
		UPlayerEquidItem* playerEquidItem = player->FindComponentByClass<UPlayerEquidItem>();

		// itemSelectIndex 값을 0으로 설정
		if (playerEquidItem)
			playerEquidItem->itemSelectIndex = 0;

		else
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerEquidItem component not found!"));
	}

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player is null!"));
}
