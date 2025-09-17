#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "QuestList.h"

#include "Blueprint/UserWidget.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "QuestDropListItemUI.h"
#include "QuestDropListUI.generated.h"

UCLASS()
class CONTRACT_API UQuestDropListUI : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    // 보상 아이템 목록 설정
    void SetRewardItems(const TArray<AItem*>& InItems);

    // UI 표시 함수
    void ShowAtLocation(const FVector2D& InScreenPosition);

    // 보상 아이템 목록 설정 (아이템 개수 포함)
    void SetRewardItemsWithQuantity(const TArray<FItemCount>& InItemsWithCount);

    // UI 숨기기 함수
    void Hide();

protected:
    // 아이템 목록 뷰
    UPROPERTY(meta = (BindWidget))
    UListView* itemListView;

    // 아이템이 없을 경우 표시할 메시지
    UPROPERTY(meta = (BindWidget))
    UTextBlock* noItemsText;

    // 아이템 목록 위젯 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UQuestDropListItemUI> dropItemClass;
};