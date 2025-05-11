#include "QuestDropListItemUI.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Internationalization/Text.h"

bool UQuestDropListItemUI::Initialize()
{
    bool isSuccess = Super::Initialize();

    if (!isSuccess)
        return false;

    // 필수 위젯 확인
    if (itemIcon == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("아이템 아이콘이 바인딩되지 않았습니다."));
        return false;
    }

    if (itemNameText == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("아이템 이름 텍스트가 바인딩되지 않았습니다."));
        return false;
    }

    if (itemQuantityText == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("아이템 개수 텍스트가 바인딩되지 않았습니다."));
        return false;
    }

    if (itemBorder == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("아이템 테두리가 바인딩되지 않았습니다."));
        return false;
    }

    // 기본 스타일 설정
    itemBorder->SetBrushColor(defaultBackgroundColor);

    // 기본 아이콘 설정
    if (defaultItemIcon)
        itemIcon->SetBrushFromTexture(defaultItemIcon);

    return true;
}

void UQuestDropListItemUI::NativeConstruct()
{
    Super::NativeConstruct();

}

void UQuestDropListItemUI::NativeOnListItemObjectSet(UObject* listItemObject)
{
    // ListView에서 이 위젯이 항목으로 설정될 때 호출됨
    UQuestDropItemData* itemData = Cast<UQuestDropItemData>(listItemObject);

    if (itemData != nullptr && itemData->Item != nullptr)
    {
        SetItemInfo(itemData->Item, itemData->Quantity);

        return;
    }

    // 기존 방식 지원 (하위 호환성)
    AItem* sourceItem = Cast<AItem>(listItemObject);

    if (sourceItem != nullptr)
    {
        SetItemInfo(sourceItem);

        return;
    }

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("NativeOnListItemObjectSet : 지원되지 않는 항목 유형"));
}

void UQuestDropListItemUI::SetItemInfo(AItem* inItem, int32 inQuantity)
{
    item = inItem;
    quantity = inQuantity;

    if (item == nullptr)
        return;

    // 아이템 이름 설정 (아이템 클래스에서 이름 가져오기)
    FString itemName = item->GetClass()->GetName();

    // 클래스 접두사(A) 제거
    if (itemName.StartsWith(TEXT("A")))
        itemName.RemoveAt(0, 1);

    itemNameText->SetText(FText::FromString(itemName));

    // 아이템 개수 설정
    FNumberFormattingOptions NumberFormat;
    NumberFormat.UseGrouping = true; // 1,000 단위로 구분자 사용

    FText formattedQuantity = FText::AsNumber(quantity, &NumberFormat);
    itemQuantityText->SetText(formattedQuantity);

    // 기본 아이콘 사용
    if (defaultItemIcon != nullptr)
        itemIcon->SetBrushFromTexture(defaultItemIcon);
}