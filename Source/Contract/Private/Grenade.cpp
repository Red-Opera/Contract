#include "Grenade.h"
#include "PlayerInventory.h"

void AGrenade::BeginPlay()
{
	Super::BeginPlay();

	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AGrenade::AddGrenade);
}

void AGrenade::UseItem()
{
	// 수류탄 사용 시 폭발 효과를 추가할 수 있습니다.
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Grenade used!"));

	GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &AGrenade::Explosion, 3.0f, false);
}

void AGrenade::Explosion()
{
	// 폭발 효과를 위한 메쉬 스폰
	if (explosionMesh)
	{
		FVector spawnLocation = GetActorLocation();
		FRotator spawnRotation = GetActorRotation();
		explosionActor = GetWorld()->SpawnActor<AActor>(explosionMesh, spawnLocation, spawnRotation);
	}
	
	GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &AGrenade::RemoveGrenade, 2.2f, false);
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
	if (!CheckPlayerIsClose())
		return;

	// 플레이어 인벤토리에 수류탄 추가
	playerInventory->items.Add(AGrenade::StaticClass());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Got a grenade!"));

	Destroy();
}