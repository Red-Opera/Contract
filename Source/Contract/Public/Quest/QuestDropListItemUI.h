#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Item.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "QuestDropListItemUI.generated.h"

// 아이템과 개수를 함께 전달하기 위한 래퍼 클래스
UCLASS()
class CONTRACT_API UQuestDropItemData : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    AItem* Item;

    UPROPERTY()
    int32 Quantity;

    void InitData(AItem* InItem, int32 InQuantity)
    {
        Item = InItem;
        Quantity = InQuantity;
    }
};

// 아이템 드롭 리스트에 표시될 각 아이템 항목 위젯
UCLASS()
class CONTRACT_API UQuestDropListItemUI : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    // ListView에서 아이템 항목이 설정될 때 호출
    virtual void NativeOnListItemObjectSet(UObject* listItemObject) override;
    virtual void NativeConstruct() override;

    // 아이템 정보 설정
    void SetItemInfo(AItem* InItem, int32 InQuantity = 1);

protected:
	FString RemovePrefixSuffix(FString itemName);

    // 아이템 아이콘
    UPROPERTY(meta = (BindWidget))
    UImage* itemIcon;

    // 아이템 이름 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* itemNameText;

    // 아이템 개수 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* itemQuantityText;

    // 항목 배경 테두리
    UPROPERTY(meta = (BindWidget))
    UBorder* itemBorder;

    // 아이템 참조
    UPROPERTY()
    AItem* item;

    // 아이템 개수
    UPROPERTY()
    int32 quantity;

    // 기본 배경색
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor defaultBackgroundColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.7f);

    // 아이템 아이콘 텍스처 (기본값)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    UTexture2D* defaultItemIcon;
};