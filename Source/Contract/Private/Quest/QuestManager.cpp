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

    chatUIInstance->SetUIOwner(this);

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
    float distance = FVector::Dist(GetActorLocation(), player->GetActorLocation());

    return distance <= interactionDistance;
}

void AQuestManager::ShowChatUI()
{
    // 대화 UI가 이미 표시되고 있는 경우
    if (isChatUIVisible)
        return;

    // UI 표시
    chatUIInstance->AddToViewport();
    isChatUIVisible = true;

    // 대화 시작
    chatUIInstance->StartDialogue();
}

void AQuestManager::HideChatUI()
{
    if (!isChatUIVisible)
        return;

    // UI 제거
    chatUIInstance->EndDialogue();
    isChatUIVisible = false;

    // 대화가 끝나면 퀘스트 추가
    AddQuestsAfterDialogue();
}

void AQuestManager::AddQuestsAfterDialogue()
{
    // 퀘스트 데이터 에셋이 없는 경우
    if (questListAsset == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("QuestList 에셋이 설정되지 않았습니다."));
        return;
    }

    // 추가할 퀘스트가 없는 경우
    if (quests.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("추가할 퀘스트가 없습니다."));
        return;
    }

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("총 %d개의 퀘스트 추가 시도 중..."), quests.Num()));

    int32 addedQuestCount = 0;
    int32 duplicateQuestCount = 0;

    // 모든 퀘스트를 questListAsset에 추가
    for (const FQuestInfo& quest : quests)
    {
        // 퀘스트 이름 로깅
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("퀘스트 추가 시도: %s"), *quest.questName));

        // 중복 확인
        if (questListAsset->HasQuest(quest.questName))
        {
            duplicateQuestCount++;
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("중복 퀘스트: %s"), *quest.questName));

            continue;
        }

        // 중복 퀘스트 확인 후 추가
        if (questListAsset->AddQuest(quest))
        {
            addedQuestCount++;
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("퀘스트 추가 성공: %s"), *quest.questName));
        }
    }

    // 결과 표시
    FString message;

    if (addedQuestCount > 0)
    {
        message = FString::Printf(TEXT("%d개의 새로운 퀘스트가 추가되었습니다."), addedQuestCount);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, message);
    }

    else
    {
        if (duplicateQuestCount > 0)
            message = FString::Printf(TEXT("모든 퀘스트(%d개)가 이미 존재합니다."), duplicateQuestCount);

        else
            message = TEXT("퀘스트 추가에 실패했습니다.");

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, message);
    }
}