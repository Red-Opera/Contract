#include "Shop/ItemShop.h"

AItemShop::AItemShop()
{
	PrimaryActorTick.bCanEverTick = true;
	isPlayerInside = false;

	// Root Scene Component 생성
	root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = root;

	// Box Component 생성 및 설정
	shopBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ShopBox"));
	shopBox->SetupAttachment(root);
	
	// Mobility 설정 - 에디터에서 이동 가능하도록
	shopBox->SetMobility(EComponentMobility::Movable);
	
	// Box 크기 설정 (인스펙터에서 수정 가능)
	shopBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	
	// 충돌 설정
	shopBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	shopBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	shopBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	shopBox->SetGenerateOverlapEvents(true);
}

void AItemShop::BeginPlay()
{
	Super::BeginPlay();
	
	// 충돌 이벤트 바인딩
	if (shopBox)
	{
		shopBox->OnComponentBeginOverlap.AddDynamic(this, &AItemShop::OnBoxBeginOverlap);
		shopBox->OnComponentEndOverlap.AddDynamic(this, &AItemShop::OnBoxEndOverlap);
	}
}

void AItemShop::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

}

void AItemShop::OnBoxBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool isFromSweep, const FHitResult& sweepResult)
{
	if (otherActor == nullptr)
		return;

	AActor* player = GetWorld()->GetFirstPlayerController()->GetPawn();

	if (otherActor == player)
	{
		isPlayerInside = true;
		SetupPlayerInput();
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Player entered the shop area. Press G to interact."));
	}
}

void AItemShop::OnBoxEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex)
{
	if (otherActor == nullptr)
		return;

	AActor* player = GetWorld()->GetFirstPlayerController()->GetPawn();

	if (otherActor == player)
		isPlayerInside = false;
}

void AItemShop::SetupPlayerInput()
{
	APlayerController* playerController = GetWorld()->GetFirstPlayerController();

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Reference null) : 플레이어 컨트롤러를 찾을 수 없습니다."));

		return;
	}

	EnableInput(playerController);

	InputComponent->BindAction("Interaction", IE_Pressed, this, &AItemShop::OnInteractionPressed);
}

void AItemShop::OnInteractionPressed()
{
	if (isPlayerInside)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Interaction key pressed! Opening shop..."));

	}
}

