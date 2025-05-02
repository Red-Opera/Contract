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

	// mesh �߷� Ȱ��ȭ
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

	// ���� �ùķ��̼� ��Ȱ��ȭ
	itemMesh->SetSimulatePhysics(false);

	// �޽� ǥ�� ��Ȱ��ȭ
	itemMesh->SetVisibility(false);
	itemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AItem::BeginPlay()
{
	Super::BeginPlay();
	
	player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	playerController = GetWorld()->GetFirstPlayerController();

	// playerData�� null���� Ȯ��
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

	// �÷��̾�� �ش� ������Ʈ ��ġ�� ����
	FVector position = GetActorLocation();
	FVector playerPosition = player->GetActorLocation();

	// �÷��̾�� �ش� ������Ʈ �Ÿ��� ����
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

	// �̹� ��ȣ�ۿ� ó�� ���̸� �ð� Ȯ�� �� ó��
	static float lastInteractionTime = 0.0f;
	float currentTime = fromCharacter->GetWorld()->GetTimeSeconds();

	if (isInteractionInProgress && currentTime - lastInteractionTime < 0.15f)
		return nullptr;

	// ��� Item ���� ã��
	TArray<AActor*> foundItems;
	UGameplayStatics::GetAllActorsOfClass(fromCharacter->GetWorld(), AItem::StaticClass(), foundItems);

	AItem* closestItem = nullptr;
	float closestDistance = MAX_FLT;

	// ȹ�� �����ϰ� �Ÿ� ���� �ִ� ���� ����� ������ ã��
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

	// ���� ����� �������� ������ ��ȣ�ۿ� ó�� ������ ����
	isInteractionInProgress = true;
	lastInteractionTime = currentTime;

	// ��� �� ��ȣ�ۿ� ó�� ���� �ʱ�ȭ
	FTimerHandle unlockTimerHandle;
	fromCharacter->GetWorld()->GetTimerManager().SetTimer
	(
		unlockTimerHandle,
		[]() { isInteractionInProgress = false; },
		0.15f, // �������� �ణ �� �� �����̷� ����
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
	// �÷��̾�� PlayerEquidItem ������Ʈ ��������
	if (player)
	{
		UPlayerEquidItem* playerEquidItem = player->FindComponentByClass<UPlayerEquidItem>();

		// itemSelectIndex ���� 0���� ����
		if (playerEquidItem)
			playerEquidItem->itemSelectIndex = 0;

		else
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerEquidItem component not found!"));
	}

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player is null!"));
}
