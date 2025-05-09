#include "QuestItemUI.h"

#include "Components/TextBlock.h"
#include "Components/ListView.h"

bool UQuestItemUI::Initialize()
{
    bool isSuccess = Super::Initialize();

    if (!isSuccess)
        return false;

    if (questNameText == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 이름 텍스트가 바인딩되지 않았습니다."));

        return false;
    }

    return true;
}

void UQuestItemUI::SetQuestInfo(const FQuestInfo& inQuestInfo)
{
    questInfo = inQuestInfo;

    // 퀘스트 이름 설정
    questNameText->SetText(FText::FromString(questInfo.questName));
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("퀘스트 정보 설정 완료 : ") + questInfo.questName);
}
