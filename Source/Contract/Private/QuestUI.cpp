#include "QuestUI.h"
#include "QuestItemUI.h"
#include "InputCoreTypes.h"

#include "Components/Border.h"
#include "Components/InputComponent.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

bool UQuestUI::Initialize()
{
    bool isSuccess = Super::Initialize();

    if (!isSuccess)
        return false;

    currentSelectedQuestItem = nullptr;

    playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    if (playerController == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 컨트롤러가 존재하지 않습니다."));
        return false;
    }

    ACharacter* player = playerController->GetCharacter();

    if (player == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 캐릭터가 존재하지 않습니다."));
        return false;
    }

    playerMovementComponent = Cast<UCharacterMovementComponent>(player->GetMovementComponent());

    if (playerMovementComponent == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 이동 컴포넌트가 존재하지 않습니다."));
        return false;
    }

    if (questDetailPanel == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 상세 정보 패널이 설정되지 않았습니다."));

        return false;
    }

    if (closeButton == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("닫기 버튼이 설정되지 않았습니다."));

        return false;
    }

    if (questDetailPanel == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 상세 정보 패널이 설정되지 않았습니다."));

        return false;
    }

    if (questListView == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 목록 뷰가 설정되지 않았습니다."));

        return false;
    }

    if (questItemWidgetClass == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 항목 위젯 클래스가 설정되지 않았습니다."));

        return false;
    }

    if (questListData == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 목록 데이터가 설정되지 않았습니다."));

        return false;
    }

    // ==========================================
    // UI 요소 초기화
    // ==========================================

    // 리스트뷰 설정
    questListView->OnItemSelectionChanged().AddUObject(this, &UQuestUI::OnQuestSelected);

    // 닫기 버튼 설정
    closeButton->OnClicked.AddDynamic(this, &UQuestUI::OnCloseButtonClicked);

    // 초기에는 상세 정보 패널 숨기기
    questDetailPanel->SetVisibility(ESlateVisibility::Hidden);

    // 초기에 UI는 숨겨진 상태
    SetVisibility(ESlateVisibility::Hidden);

    return true;
}

void UQuestUI::NativeTick(const FGeometry& myGeometry, float deltaTime)
{
    Super::NativeTick(myGeometry, deltaTime);

    if (!isQuestUIActive)
        return;

    // 퀘스트 UI가 활성화되어 있을 때만 입력 처리
    if (playerController->WasInputKeyJustPressed(EKeys::Escape))
        HideQuestUI();

}

void UQuestUI::SetQuestList(UQuestList* inQuestList)
{
    questListData = inQuestList;

    RefreshQuestList();
}

void UQuestUI::ShowQuestUI()
{
    // 퀘스트 UI 시작 설정
    isQuestUIActive = true;

    // 현재 선택된 항목 초기화
    currentSelectedQuestItem = nullptr;

    // UI 표시
    SetVisibility(ESlateVisibility::Visible);

    // 마우스 커서 표시
    playerController->SetShowMouseCursor(true);

    // 게임과 UI 모두 입력을 받을 수 있는 모드로 설정
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // 마우스를 뷰포트에 가두지 않음
    InputMode.SetHideCursorDuringCapture(false); // 마우스 캡처 중에도 커서 표시
    playerController->SetInputMode(InputMode);

    // 카메라 회전 무시
    playerController->SetIgnoreLookInput(true);

    // 목록 갱신
    RefreshQuestList();
}

void UQuestUI::HideQuestUI()
{
    // 퀘스트 UI 비활성화
    isQuestUIActive = false;

    // 현재 선택된 항목 초기화
    currentSelectedQuestItem = nullptr;

    // 마우스 커서 숨김
    playerController->SetShowMouseCursor(false);

    // 게임 전용 입력 모드로 설정
    FInputModeGameOnly InputMode;
    playerController->SetInputMode(InputMode);

    // 카메라 회전 복원
    playerController->SetIgnoreLookInput(false);

    // UI 숨기기
    SetVisibility(ESlateVisibility::Hidden);
}

void UQuestUI::ToggleQuestUI()
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("P 키가 눌렸습니다. 퀘스트 UI를 토글합니다."));

    if (GetVisibility() == ESlateVisibility::Visible)
        HideQuestUI();

    else
        ShowQuestUI();
}

void UQuestUI::RefreshQuestList()
{
    // 리스트 초기화
    questListView->ClearListItems();

    // 모든 퀘스트 항목 추가
    for (const FQuestInfo& questInfo : questListData->quests)
    {
        UQuestItemUI* questItemWidget = CreateWidget<UQuestItemUI>(this, questItemWidgetClass);

        if (questItemWidget)
        {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("퀘스트 항목 위젯 생성 성공"));

            questItemWidget->SetQuestInfo(questInfo); 
            questListView->AddItem(questItemWidget);
			questListView->RequestRefresh();
        }
    }
}

void UQuestUI::OnQuestSelected(UObject* item)
{
    UQuestItemUI* questItemWidget = Cast<UQuestItemUI>(item);

    if (questItemWidget)
    {
        FString questName = questItemWidget->GetQuestInfo().questName;

        // 이미 선택된 항목을 다시 클릭했는지 확인
        if (currentSelectedQuestItem == questItemWidget)
        {
            ESlateVisibility currentVisibility = questDetailPanel->GetVisibility();

            // 이미 표시 중인 경우 세부 정보 패널 숨기기
            if (currentVisibility == ESlateVisibility::Visible)
            {
                questDetailPanel->SetVisibility(ESlateVisibility::Hidden);

                // 선택 해제
                currentSelectedQuestItem = nullptr;
            }

            else
            {
                // 숨겨져 있는 경우 다시 표시
                questDetailPanel->SetVisibility(ESlateVisibility::Visible);

                // 선택된 퀘스트의 상세 정보 표시
                DisplayQuestDetails(questItemWidget->GetQuestInfo());
            }
        }

        else
        {
            // 새로운 항목 선택
            currentSelectedQuestItem = questItemWidget;

            // 선택된 퀘스트의 상세 정보 표시
            DisplayQuestDetails(questItemWidget->GetQuestInfo());
        }

        // 다음 클릭을 위해 ListView의 선택 해제
        GetWorld()->GetTimerManager().SetTimerForNextTick([this]() { questListView->SetSelectedItem(nullptr); });
    }
}

void UQuestUI::OnCloseButtonClicked()
{
    HideQuestUI();
}

void UQuestUI::DisplayQuestDetails(const FQuestInfo& QuestInfo)
{
    // 상세 정보 패널 표시
    if (questDetailPanel)
        questDetailPanel->SetVisibility(ESlateVisibility::Visible);

    // 퀘스트 이름 설정
    if (detailQuestNameText)
        detailQuestNameText->SetText(FText::FromString(QuestInfo.questName));

    // 퀘스트 내용 설정
    if (questContentText)
        questContentText->SetText(FText::FromString(QuestInfo.content));

    // 퀘스트 조건 설정
    if (questConditionsText)
        questConditionsText->SetText(FText::FromString(QuestInfo.conditions));

    // 보상 - 돈 설정
    if (questMoneyRewardText)
        questMoneyRewardText->SetText(FText::FromString(FString::Printf(TEXT("%d Gold"), QuestInfo.money)));

    // 보상 - 경험치 설정
    if (questExpRewardText)
        questExpRewardText->SetText(FText::FromString(FString::Printf(TEXT("%d XP"), QuestInfo.experiencePoints)));
}