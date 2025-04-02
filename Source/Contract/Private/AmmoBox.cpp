#include "AmmoBox.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AAmmoBox::AAmmoBox()
{
	PrimaryActorTick.bCanEverTick = true;

	mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = mesh;

	// mesh 중력 활성화
	mesh->SetSimulatePhysics(true);
	mesh->SetEnableGravity(true);
}

void AAmmoBox::BeginPlay()
{
	Super::BeginPlay();

	player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	playerController = GetWorld()->GetFirstPlayerController();

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

	UInputComponent* playerInputComponent = player->InputComponent;

	if (playerInputComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Input Component does not exist."));
		return;
	}

	EnableInput(playerController);
	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AAmmoBox::AddAmmo);
	
}

bool AAmmoBox::CheckPlayerIsClose()
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

void AAmmoBox::AddAmmo()
{
	if (!CheckPlayerIsClose())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player is not close."));
		return;
	}

	// 플레이어 인벤토리에 총알 추가
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Player is close."));
}

// Called every frame
void AAmmoBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

