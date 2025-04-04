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
	if (!CheckPlayerIsClose())
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