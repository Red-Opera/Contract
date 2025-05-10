#pragma once

#include "CoreMinimal.h"
#include "QuestList.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "QuestItemUI.generated.h"

// ����Ʈ �׸��� ���� ���� Ŭ����
UCLASS(BlueprintType, Blueprintable)
class CONTRACT_API UQuestItemUI : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual void NativeConstruct() override;

    // ����Ʈ ���� ����
    void SetQuestInfo(const FQuestInfo& inQuestInfo);

    // ���� ������ ����Ʈ ���� ��������
    const FQuestInfo& GetQuestInfo() const { return questInfo; }

protected:
    // ���� ���� ������Ʈ �Լ�
    void UpdateRewardsInfo();

    // ����Ʈ ���� ����
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    FQuestInfo questInfo;

    // ����Ʈ �̸� �ؽ�Ʈ
    UPROPERTY(meta = (BindWidget))
    UTextBlock* questNameText;

    // ����Ʈ ª�� ���� �ؽ�Ʈ
    UPROPERTY(meta = (BindWidget))
    UTextBlock* shortContentText;

    // ���� - �� �ؽ�Ʈ
    UPROPERTY(meta = (BindWidget))
    UTextBlock* moneyRewardText;

    // ���� - ����ġ �ؽ�Ʈ
    UPROPERTY(meta = (BindWidget))
    UTextBlock* expRewardText;

    // ����Ʈ �׸� ��� �׵θ�
    UPROPERTY(meta = (BindWidget))
    UBorder* questBorder;

    // ���� �����̳�
    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* rewardsContainer;

    // �� ������
    UPROPERTY(meta = (BindWidget))
    UImage* moneyIcon;

    // ����ġ ������
    UPROPERTY(meta = (BindWidget))
    UImage* expIcon;

    // ���� ���� (���õ� �׸� ���� ǥ��)
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    bool isSelected = false;

    // �⺻ ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor defaultBackgroundColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.7f);

    // ���õ� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor selectedBackgroundColor = FLinearColor(0.2f, 0.4f, 0.6f, 0.8f);

    // �� ������ �ؽ�ó
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    UTexture2D* goldIconTexture;

    // ����ġ ������ �ؽ�ó
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    UTexture2D* expIconTexture;
};