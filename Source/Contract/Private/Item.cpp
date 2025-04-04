#include "Item.h"
#include "PlayerInventory.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

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

void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

