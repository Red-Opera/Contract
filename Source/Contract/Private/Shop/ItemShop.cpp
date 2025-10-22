#include "Shop/ItemShop.h"
#include "Blueprint/UserWidget.h"

// static 변수 정의
bool AItemShop::isItemShopWidgetOpen = false;
AItemShop* AItemShop::currentItemShop = nullptr;

AItemShop::AItemShop()
{
	PrimaryActorTick.bCanEverTick = true;
	isPlayerInside = false;
	isShopOpen = false;
	playerController = nullptr;
	originalViewTarget = nullptr;
	escBinding = nullptr;

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

	// Camera Component 생성 및 설정
	shopCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ShopCamera"));
	shopCamera->SetupAttachment(root);
	shopCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));
	shopCamera->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
}

void AItemShop::BeginPlay()
{
	Super::BeginPlay();
	
	currentItemShop = this;

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

	playerController = GetWorld()->GetFirstPlayerController();

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 플레이어 컨트롤러를 찾을 수 없습니다."));

		return;
	}

	AActor* player = GetWorld()->GetFirstPlayerController()->GetPawn();

	if (otherActor == player)
	{
		isPlayerInside = true;
		SetupPlayerInput();
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
	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 플레이어 컨트롤러를 찾을 수 없습니다."));

		return;
	}

	EnableInput(playerController);

	InputComponent->BindAction("Interaction", IE_Pressed, this, &AItemShop::OnInteractionPressed);
	
	// ESC 바인딩 저장
	escBinding = &InputComponent->BindAction("ESC", IE_Pressed, this, &AItemShop::OnESCPressed);
}

void AItemShop::OnInteractionPressed()
{
	if (!isPlayerInside || isShopOpen)
		return;

	if (itemShopStartWidgetClass == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 아이템 상점 시작 위젯 클래스가 설정되지 않았습니다."));

		return;
	}

	// ItemShopWidgetStart 열기
	if (itemShopStartWidgetInstance == nullptr)
		itemShopStartWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), itemShopStartWidgetClass);

	if (itemShopStartWidgetInstance == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 위젯 인스턴스 생성에 실패했습니다."));

		return;
	}

	itemShopStartWidgetInstance->AddToViewport();
	isShopOpen = true;

	// 카메라를 Shop Camera로 전환
	SwitchToShopCamera();

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 플레이어 컨트롤러를 찾을 수 없습니다."));

		return;
	}

	// 마우스 커서 표시 및 입력 모드를 GameAndUI로 변경 (ESC 키가 작동하도록)
	playerController->SetShowMouseCursor(true);
	FInputModeGameAndUI inputMode;
	inputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	playerController->SetInputMode(inputMode);
}

void AItemShop::OnESCPressed()
{
	// ItemShopWidget이 열려있으면 ItemShop의 ESC는 작동하지 않음
	if (isItemShopWidgetOpen)
		return;

	if (isShopOpen)
		CloseShop();
}

void AItemShop::SwitchToShopCamera()
{
	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 플레이어 컨트롤러를 찾을 수 없습니다."));

		return;
	}

	// 원래 카메라 저장
	originalViewTarget = playerController->GetViewTarget();

	// Shop Camera로 전환
	playerController->SetViewTargetWithBlend(this, 0.5f);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Camera switched to Shop Camera"));
}

void AItemShop::RestoreOriginalCamera()
{
	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 플레이어 컨트롤러를 찾을 수 없습니다."));

		return;
	}

	if (originalViewTarget)
	{
		playerController->SetViewTargetWithBlend(originalViewTarget, 0.5f);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Camera restored to original"));
	}
}

void AItemShop::CloseShop()
{
	// UI 닫기
	if (itemShopStartWidgetInstance)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Closing Item Shop Widget"));

		itemShopStartWidgetInstance->RemoveFromParent();
	}

	// 카메라 복원
	RestoreOriginalCamera();

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 플레이어 컨트롤러를 찾을 수 없습니다."));

		return;
	}

	// 마우스 및 입력 모드 복원
	playerController->SetShowMouseCursor(false);
	playerController->SetInputMode(FInputModeGameOnly());

	isShopOpen = false;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Shop closed"));
}

void AItemShop::SetItemShopWidgetOpen(bool isOpen)
{
	isItemShopWidgetOpen = isOpen;
}

bool AItemShop::IsItemShopWidgetCurrentlyOpen()
{
	return isItemShopWidgetOpen;
}

void AItemShop::EnableESCBinding(bool enable)
{
	if (currentItemShop == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 현재 아이템 상점 인스턴스를 찾을 수 없습니다."));

		return;
	}

	if (currentItemShop->InputComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : 현재 아이템 상점의 입력 컴포넌트를 찾을 수 없습니다."));

		return;
	}

	if (enable)
	{
		// ESC 바인딩 활성화
		if (currentItemShop->escBinding == nullptr)
		{
			currentItemShop->escBinding = &currentItemShop->InputComponent->BindAction("ESC", IE_Pressed, currentItemShop, &AItemShop::OnESCPressed);
		}
	}

	else
	{
		// ESC 바인딩 비활성화
		if (currentItemShop->escBinding)
		{
			currentItemShop->InputComponent->RemoveActionBinding(currentItemShop->escBinding->GetActionName(), IE_Pressed);
			currentItemShop->escBinding = nullptr;
		}
	}
}

AItemShop* AItemShop::GetCurrentItemShop()
{
	return currentItemShop;
}