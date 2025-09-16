#include "QuestDropListUI.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"

bool UQuestDropListUI::Initialize()
{
    bool isSuccess = Super::Initialize();

    if (!isSuccess)
        return false;

    if (itemListView == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ItemListView가 존재하지 않습니다."));

		return false;
	}

    if (noItemsText == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("noItemsText가 존재하지 않습니다."));

		return false;
    }

	if (dropItemClass == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ItemEntryWidgetClass가 존재하지 않습니다."));

		return false;
	}

    // 기본적으로 숨김 상태로 시작
    SetVisibility(ESlateVisibility::Collapsed);

    return true;
}

void UQuestDropListUI::SetRewardItems(const TArray<AItem*>& inItems)
{
    // 목록 초기화
    itemListView->ClearListItems();

    // 아이템이 있는지 확인
    if (inItems.Num() == 0)
    {
        // 아이템이 없으면 메시지 표시
        if (noItemsText)
        {
            noItemsText->SetText(FText::FromString(TEXT("보상 아이템이 없습니다.")));
            noItemsText->SetVisibility(ESlateVisibility::Visible);
        }

        if (itemListView)
            itemListView->SetVisibility(ESlateVisibility::Collapsed);

        return;
    }

    noItemsText->SetVisibility(ESlateVisibility::Collapsed);    // 아이템이 있으면 noItemsText 숨기기
    itemListView->SetVisibility(ESlateVisibility::Visible);     // ListView 표시

    // 아이템 추가
    for (AItem* item : inItems)
    {
        if (item != nullptr)
            itemListView->AddItem(item);
    }
}

void UQuestDropListUI::ShowAtLocation(const FVector2D& inScreenPosition)
{
    // UI를 표시
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    // 뷰포트 크기 가져오기
    FVector2D viewportSize;
    GEngine->GameViewport->GetViewportSize(viewportSize);

    // UI 사이즈 계산
    FVector2D widgetSize = GetDesiredSize();

    // 화면 밖으로 나가지 않도록 위치 조정
    // 커서 위치에서 약간 오른쪽 위로 오프셋 적용 (마우스 커서가 겹치지 않도록)
    FVector2D adjustedPosition = inScreenPosition ;

    // 화면 오른쪽 경계 확인
    if (adjustedPosition.X + widgetSize.X > viewportSize.X)
        adjustedPosition.X = viewportSize.X - widgetSize.X;

    // 화면 위쪽 경계 확인 (음수 좌표 방지)
    if (adjustedPosition.Y < 0)
        adjustedPosition.Y = inScreenPosition.Y + 5.0f; // 이 경우 마우스 아래에 표시

    // 위치 설정 (절대 좌표)
    SetPositionInViewport(adjustedPosition, false); // false = 절대 좌표 사용
}

void UQuestDropListUI::SetRewardItemsWithQuantity(const TArray<FItemCount>& InItemsWithCount)
{
    // 목록 초기화
    itemListView->ClearListItems();

    // 아이템이 있는지 확인
    if (InItemsWithCount.Num() == 0)
    {
        // 아이템이 없으면 메시지 표시
        if (noItemsText)
        {
            noItemsText->SetText(FText::FromString(TEXT("보상 아이템이 없습니다.")));
            noItemsText->SetVisibility(ESlateVisibility::Visible);
        }

        if (itemListView)
            itemListView->SetVisibility(ESlateVisibility::Collapsed);

        return;
    }
    
    noItemsText->SetVisibility(ESlateVisibility::Collapsed);    // 아이템이 있으면 noItemsText 숨기기
    itemListView->SetVisibility(ESlateVisibility::Visible);     // ListView 표시

    // 아이템 추가
    for (const FItemCount& itemWithCount : InItemsWithCount)
    {
        if (itemWithCount.itemClass == nullptr)
            continue;

        // 임시 아이템 인스턴스 생성 (월드에 배치되지 않음)
        AItem* tempItem = NewObject<AItem>(this, itemWithCount.itemClass);

        if (tempItem == nullptr)
            continue;

        // 아이템과 개수를 함께 저장하는 래퍼 객체 생성
        UQuestDropItemData* itemData = NewObject<UQuestDropItemData>(this);
        itemData->InitData(tempItem, itemWithCount.count);

        // 래퍼 객체를 리스트뷰에 추가
        itemListView->AddItem(itemData);
    }
}

void UQuestDropListUI::Hide()
{
    SetVisibility(ESlateVisibility::Collapsed);
}