#include "QuestManager.h"
#include "ChatUI.h"

#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AQuestManager::AQuestManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AQuestManager::BeginPlay()
{
    Super::BeginPlay();

    // 플레이어 캐릭터 가져오기
    player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    playerController = GetWorld()->GetFirstPlayerController();

    if (player == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 캐릭터가 존재하지 않음"));

        return;
    }

    if (playerController == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 컨트롤러가 존재하지 않음"));

        return;
    }

	if (chatUIClass == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ChatUI class를 설정하지 않음"));

		return;
	}

	// 대화 UI 인스턴스 생성
    chatUIInstance = CreateWidget<UChatUI>(GetWorld()->GetFirstPlayerController(), chatUIClass);

	if (chatUIInstance == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ChatUI instance 생성 실패"));

		return;
	}

    playerInputComponent = player->InputComponent;

    if (playerInputComponent == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Input Component가 존재하지 않음"));

        return;
    }

    EnableInput(playerController);

    // 상호작용 키 바인딩
    playerInputComponent->BindAction("Interaction", IE_Pressed, this, &AQuestManager::InteractWithPlayer);
}

void AQuestManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AQuestManager::InteractWithPlayer()
{
    // 플레이어가 가까이 있고 대화 UI가 표시되지 않은 경우에만 대화 UI 표시
    if (IsPlayerClose() && !isChatUIVisible)
        ShowChatUI();

    // 대화 UI가 이미 표시되어 있는 경우 숨기기
    else if (isChatUIVisible)
        HideChatUI();
}

bool AQuestManager::IsPlayerClose() const
{
    float Distance = FVector::Dist(GetActorLocation(), player->GetActorLocation());

    return Distance <= interactionDistance;
}

void AQuestManager::ShowChatUI()
{
    // 대화 UI가 이미 표시되고 있는 경우
    if (isChatUIVisible)
        return;

    // 대화 시작
    chatUIInstance->StartDialogue();

    // UI 표시
    chatUIInstance->AddToViewport();
    isChatUIVisible = true;
}

void AQuestManager::HideChatUI()
{
    if (!isChatUIVisible)
        return;

    // 대화 종료
    chatUIInstance->StartDialogue();

    // UI 제거
    chatUIInstance->RemoveFromViewport();
    isChatUIVisible = false;
}
