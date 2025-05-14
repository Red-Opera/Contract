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

	// 아이템 이름에서 접두사 및 접미사 제거
	itemName = RemovePrefixSuffix(itemName);
    itemNameText->SetText(FText::FromString(itemName));

    // 아이템 개수 설정
    FNumberFormattingOptions NumberFormat;
    NumberFormat.UseGrouping = true; // 1,000 단위로 구분자 사용

    FText formattedQuantity = FText::AsNumber(quantity, &NumberFormat);
    itemQuantityText->SetText(formattedQuantity);

    // 아이템 아이콘 설정 (itemIcon 프로퍼티를 우선적으로 사용)
    if (item->itemIcon != nullptr)
    {
        // 아이템에 설정된 아이콘이 있으면 사용
        itemIcon->SetBrushFromTexture(item->itemIcon);
        return;
    }

    // 아이템 아이콘이 없는 경우 메시의 머티리얼에서 텍스처를 가져와 사용
    UStaticMeshComponent* meshComponent = item->itemMesh;

    if (meshComponent != nullptr && meshComponent->GetNumMaterials() > 0)
    {
        UMaterialInterface* material = meshComponent->GetMaterial(0);

        if (material != nullptr)
        {
            // 머티리얼의 첫 번째 텍스처 파라미터를 찾아 사용
            TArray<FMaterialParameterInfo> parameterInfos;
            TArray<FGuid> parameterGuids;

            material->GetAllTextureParameterInfo(parameterInfos, parameterGuids);

            for (const FMaterialParameterInfo& paramInfo : parameterInfos)
            {
                UTexture* texture = nullptr;

                if (material->GetTextureParameterValue(paramInfo, texture) && texture != nullptr)
                {
                    // 텍스처를 찾았으면 아이콘으로 설정
                    itemIcon->SetBrushFromTexture(Cast<UTexture2D>(texture));

                    return;
                }
            }
        }
    }

    // 텍스처를 찾지 못했거나 메시가 없는 경우 기본 아이콘 사용
    if (defaultItemIcon != nullptr)
        itemIcon->SetBrushFromTexture(defaultItemIcon);
}

FString UQuestDropListItemUI::RemovePrefixSuffix(FString itemName)
{
    // 클래스 접두사(A) 제거
    if (itemName.StartsWith(TEXT("A")))
        itemName.RemoveAt(0, 1);

    // 블루프린트 클래스 접미사 제거 (_Blueprint_C 또는 _C)
    int32 suffixIndex;

    if (itemName.FindLastChar('_', suffixIndex) && suffixIndex > 0)
    {
        // _Blueprint_C 또는 _C 접미사 제거
        FString suffix = itemName.Mid(suffixIndex);

        bool isBlueprintSuffix = suffix.Equals(TEXT("_C")) || suffix.Equals(TEXT("_Blueprint_C")) || suffix.Equals(TEXT("Blueprint_C"));

        if (isBlueprintSuffix)
        {
            // 블루프린트 접미사 제거
            itemName = itemName.Left(suffixIndex);

            // _Blueprint 부분도 제거
            if (itemName.EndsWith(TEXT("_Blueprint")))
                itemName.RemoveFromEnd(TEXT("_Blueprint"));

            else if (itemName.EndsWith(TEXT("Blueprint")))
                itemName.RemoveFromEnd(TEXT("Blueprint"));
        }
    }

	return itemName;
}