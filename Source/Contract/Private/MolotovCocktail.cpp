#include "MolotovCocktail.h"
#include "PlayerInventory.h"
#include "GameFramework/Character.h"

#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"

AMolotovCocktail::AMolotovCocktail()
{
	// �浹 ������Ʈ ����
	collisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("collisionComponent"));
	collisionComponent->SetupAttachment(itemMesh);
}

void AMolotovCocktail::BeginPlay()
{
	Super::BeginPlay();

	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AMolotovCocktail::AddMolotovCocktail);

	// �浹 ������Ʈ ����
	if (collisionComponent)
	{
		// Hit �̺�Ʈ�� Overlap �̺�Ʈ ��� Ȱ��ȭ
		collisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		collisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);

		// ��� ä�ο� ���� Block���� �����ϰ� Ư�� ä�θ� Overlap���� ����
		collisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		collisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

		collisionComponent->SetNotifyRigidBodyCollision(true); // Hit �̺�Ʈ Ȱ��ȭ
		collisionComponent->SetGenerateOverlapEvents(true);    // Overlap �̺�Ʈ Ȱ��ȭ

		// Hit �̺�Ʈ ���ε�
		collisionComponent->OnComponentHit.AddDynamic(this, &AMolotovCocktail::OnComponentHit);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Molotov collision setup complete"));
	}

	// ������ �޽� ����
	if (itemMesh)
	{
		// ���� �ùķ��̼� Ȱ��ȭ
		itemMesh->SetSimulatePhysics(true);
		itemMesh->SetNotifyRigidBodyCollision(true);
		itemMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

		// Hit �̺�Ʈ ���ε�
		itemMesh->OnComponentHit.AddDynamic(this, &AMolotovCocktail::OnComponentHit);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Molotov mesh setup complete"));
	}

	isUse = false;
}

// Hit �̺�Ʈ�� ó���ϴ� �� �Լ� �߰�
void AMolotovCocktail::OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// isUse�� true�̰�, �ٸ� ���Ϳ� �浹���� ���� ȭ�� ���� ����
	if (isUse && OtherActor && OtherActor != this)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Molotov Cocktail exploded")));

		// ������ �޽� ����
		RemoveItemMesh();

		// �浹 ��ġ�� ȭ�� ���� ����
		FVector SpawnLocation = Hit.ImpactPoint;
		SpawnFireActor(SpawnLocation);

		isUse = false;

		// ����� �޽��� ���
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Molotov Cocktail exploded at %s"), *SpawnLocation.ToString()));

		// ȭ���� ��ü�� ����
		GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &AMolotovCocktail::RemoveActor, 5.0f, false);
	}
}

void AMolotovCocktail::UseItem()
{
	Super::UseItem();

	isUse = true;

	// ȭ���� ������ ���� �߰�
	//if (itemMesh && isUse)
	//{
	//	// �÷��̾� �������� ���� ����
	//	FVector ThrowDirection = player->GetActorForwardVector();
	//	float ThrowForce = 1500.0f; // ������ �� ����
	//
	//	// ���� �ùķ��̼� Ȱ��ȭ Ȯ��
	//	if (!itemMesh->IsSimulatingPhysics())
	//		itemMesh->SetSimulatePhysics(true);
	//
	//	// ȭ������ ���� ����
	//	itemMesh->AddImpulse(ThrowDirection * ThrowForce);
	//
	//	// ����׿� �޽���
	//	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Molotov thrown!"));
	//}
}

void AMolotovCocktail::AddMolotovCocktail()
{
	// �⺻ �Ÿ� üũ�� ������ �ʿ�
	if (!CheckPlayerIsClose() || !isGetable)
		return;

	// ���� ����� ��ȣ�ۿ� ������ �������� ã��
	AItem* closestItem = AItem::GetClosestInteractableItem(player);

	// �� �������� ���� ����� �������� �ƴϸ� ����
	if (closestItem != this)
		return;

	// �÷��̾� �κ��丮�� ȭ���� �߰�
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
	// fireMesh�� �����Ǿ� �ִ��� Ȯ��
	if (!fireMesh)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No explosion mesh class specified!"));
		return;
	}

	// ȭ�� ���� ����
	UWorld* World = GetWorld();
	if (World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// ȭ�� ���� ����, �ణ ���� �÷��� ����
		FVector AdjustedLocation = SpawnLocation + FVector(0, 0, 10.0f);
		FRotator SpawnRotation = FRotator::ZeroRotator;

		fireActor = World->SpawnActor<AActor>(fireMesh, AdjustedLocation, SpawnRotation, SpawnParams);

		if (fireActor)
		{
			// ȭ�� ���� ���� ���� �޽���
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
				FString::Printf(TEXT("Fire actor spawned at %s"), *AdjustedLocation.ToString()));

			// 10�� �Ŀ� ȭ�� ���Ϳ� �Բ� ����
			GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &AMolotovCocktail::RemoveActor, 10.0f, false);
		}
	}
}