#pragma once

#include "CoreMinimal.h"
#include "QuestList.h"
#include "QuestDropListUI.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "QuestItemUI.generated.h"

// 퀘스트 항목을 위한 위젯 클래스
UCLASS(BlueprintType, Blueprintable)
class CONTRACT_API UQuestItemUI : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual void NativeConstruct() override;

    // 퀘스트 정보 설정
    void SetQuestInfo(const FQuestInfo& inQuestInfo);

    // 현재 위젯의 퀘스트 정보 가져오기
    const FQuestInfo& GetQuestInfo() const { return questInfo; }

protected:
    // 보상 정보 업데이트 함수
    void UpdateRewardsInfo();

    // 마우스 이벤트 처리
    virtual FReply NativeOnMouseMove(const FGeometry& inGeometry, const FPointerEvent& inMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry& inGeometry, const FPointerEvent& inMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& inMouseEvent) override;

    // 아이템 드롭 리스트 UI
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UQuestDropListUI> questDropListUI;

    // 생성된 아이템 드롭 리스트 UI 인스턴스
    UQuestDropListUI* currentShowQuestDropListUI;

    // 퀘스트 정보 참조
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    FQuestInfo questInfo;

    // 퀘스트 이름 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* questNameText;

    // 퀘스트 짧은 내용 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* shortContentText;

    // 보상 - 돈 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* moneyRewardText;

    // 보상 - 경험치 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* expRewardText;

    // 퀘스트 항목 배경 테두리
    UPROPERTY(meta = (BindWidget))
    UBorder* questBorder;

    // 보상 컨테이너
    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* rewardsContainer;

    // 돈 아이콘
    UPROPERTY(meta = (BindWidget))
    UImage* moneyIcon;

    // 경험치 아이콘
    UPROPERTY(meta = (BindWidget))
    UImage* expIcon;

    // 선택 여부 (선택된 항목 강조 표시)
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    bool isSelected = false;

    // 선택된 배경색
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor selectedBackgroundColor = FLinearColor(0.2f, 0.4f, 0.6f, 0.8f);

    // 돈 아이콘 텍스처
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    UTexture2D* goldIconTexture;

    // 경험치 아이콘 텍스처
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    UTexture2D* expIconTexture;
};