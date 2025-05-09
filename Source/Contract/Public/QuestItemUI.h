#pragma once

#include "CoreMinimal.h"
#include "QuestList.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "QuestItemUI.generated.h"

// ����Ʈ �׸��� ���� ���� Ŭ����
UCLASS(BlueprintType, Blueprintable)
class CONTRACT_API UQuestItemUI : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    // ����Ʈ ���� ����
    void SetQuestInfo(const FQuestInfo& inQuestInfo);

    // ���� ������ ����Ʈ ���� ��������
    const FQuestInfo& GetQuestInfo() const { return questInfo; }

protected:
    // ����Ʈ ���� ����
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    FQuestInfo questInfo;

    // ����Ʈ �̸� �ؽ�Ʈ
    UPROPERTY(meta = (BindWidget))
    UTextBlock* questNameText;
};
