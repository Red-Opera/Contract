#include "MolotovCocktail.h"
#include "PlayerInventory.h"
#include "GameFramework/Character.h"

#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"

AMolotovCocktail::AMolotovCocktail()
{
	// 충돌 컴포넌트 생성
	collisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("collisionComponent"));
	collisionComponent->SetupAttachment(itemMesh);
}

void AMolotovCocktail::BeginPlay()
{
	Super::BeginPlay();

	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AMolotovCocktail::AddMolotovCocktail);

	// 충돌 컴포넌트 설정
	if (collisionComponent)
	{
		// Hit 이벤트와 Overlap 이벤트 모두 활성화
		collisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		collisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);

		// 모든 채널에 대해 Block으로 설정하고 특정 채널만 Overlap으로 설정
		collisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		collisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

		collisionComponent->SetNotifyRigidBodyCollision(true); // Hit 이벤트 활성화
		collisionComponent->SetGenerateOverlapEvents(true);    // Overlap 이벤트 활성화

		// Hit 이벤트 바인딩
		collisionComponent->OnComponentHit.AddDynamic(this, &AMolotovCocktail::OnComponentHit);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Molotov collision setup complete"));
	}

	// 아이템 메시 설정
	if (itemMesh)
	{
		// 물리 시뮬레이션 활성화
		itemMesh->SetSimulatePhysics(true);
		itemMesh->SetNotifyRigidBodyCollision(true);
		itemMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

		// Hit 이벤트 바인딩
		itemMesh->OnComponentHit.AddDynamic(this, &AMolotovCocktail::OnComponentHit);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Molotov mesh setup complete"));
	}

	isUse = false;
}

// Hit 이벤트를 처리하는 새 함수 추가
void AMolotovCocktail::OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// isUse가 true이고, 다른 액터와 충돌했을 때만 화재 액터 생성
	if (isUse && OtherActor && OtherActor != this)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Molotov Cocktail exploded")));

		// 아이템 메시 제거
		RemoveItemMesh();

		// 충돌 위치에 화재 액터 생성
		FVector SpawnLocation = Hit.ImpactPoint;
		SpawnFireActor(SpawnLocation);

		isUse = false;

		// 디버그 메시지 출력
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Molotov Cocktail exploded at %s"), *SpawnLocation.ToString()));

		// 화염병 자체는 제거
		GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &AMolotovCocktail::RemoveActor, 5.0f, false);
	}
}

void AMolotovCocktail::UseItem()
{
	Super::UseItem();

	isUse = true;

	// 화염병 던지기 로직 추가
	//if (itemMesh && isUse)
	//{
	//	// 플레이어 전방으로 힘을 가함
	//	FVector ThrowDirection = player->GetActorForwardVector();
	//	float ThrowForce = 1500.0f; // 던지는 힘 조절
	//
	//	// 물리 시뮬레이션 활성화 확인
	//	if (!itemMesh->IsSimulatingPhysics())
	//		itemMesh->SetSimulatePhysics(true);
	//
	//	// 화염병에 힘을 가함
	//	itemMesh->AddImpulse(ThrowDirection * ThrowForce);
	//
	//	// 디버그용 메시지
	//	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Molotov thrown!"));
	//}
}

void AMolotovCocktail::AddMolotovCocktail()
{
	// 기본 거리 체크는 여전히 필요
	if (!CheckPlayerIsClose() || !isGetable)
		return;

	// 가장 가까운 상호작용 가능한 아이템을 찾기
	AItem* closestItem = AItem::GetClosestInteractableItem(player);

	// 이 아이템이 가장 가까운 아이템이 아니면 무시
	if (closestItem != this)
		return;

	// 플레이어 인벤토리에 화염병 추가
	playerInventory->items.Add(AMolotovCocktail::StaticClass());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Got a Molotov Cocktail!"));

	Destroy();
}


void AMolotovCocktail::RemoveActor()
{
	if (fireActor)
	{
		fireActor->Destroy();
		fireActor = nullptr;
	}

	Destroy();
}

void AMolotovCocktail::SpawnFireActor(const FVector& SpawnLocation)
{
	// fireMesh가 설정되어 있는지 확인
	if (!fireMesh)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No explosion mesh class specified!"));
		return;
	}

	// 화재 액터 생성
	UWorld* World = GetWorld();
	if (World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 화재 액터 생성, 약간 위로 올려서 생성
		FVector AdjustedLocation = SpawnLocation + FVector(0, 0, 10.0f);
		FRotator SpawnRotation = FRotator::ZeroRotator;

		fireActor = World->SpawnActor<AActor>(fireMesh, AdjustedLocation, SpawnRotation, SpawnParams);

		if (fireActor)
		{
			// 화재 액터 생성 성공 메시지
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
				FString::Printf(TEXT("Fire actor spawned at %s"), *AdjustedLocation.ToString()));

			// 10초 후에 화재 액터와 함께 제거
			GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &AMolotovCocktail::RemoveActor, 10.0f, false);
		}
	}
}