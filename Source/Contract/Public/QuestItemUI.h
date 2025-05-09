#pragma once

#include "CoreMinimal.h"
#include "QuestList.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "QuestItemUI.generated.h"

// 퀘스트 항목을 위한 위젯 클래스
UCLASS(BlueprintType, Blueprintable)
class CONTRACT_API UQuestItemUI : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    // 퀘스트 정보 설정
    void SetQuestInfo(const FQuestInfo& inQuestInfo);

    // 현재 위젯의 퀘스트 정보 가져오기
    const FQuestInfo& GetQuestInfo() const { return questInfo; }

protected:
    // 퀘스트 정보 참조
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    FQuestInfo questInfo;

    // 퀘스트 이름 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* questNameText;
};
