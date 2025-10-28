#include "Shop/ItemShopBuyButtonWidget.h"
#include "Player/Item/Item.h"
#include "Player/Item/Gun.h"

void UItemShopBuyButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (itemButton)
	{
		itemButton->OnClicked.AddDynamic(this, &UItemShopBuyButtonWidget::OnItemButtonClicked);
	}
}

void UItemShopBuyButtonWidget::SetItemInfo(TSubclassOf<AActor> itemClass)
{
	if (itemClass == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("Error (Null Reference) : itemClass가 유효하지 않습니다."));

		return;
	}

	currentItemClass = itemClass;

	// CDO(Class Default Object)를 통해 아이템 정보 가져오기
	AActor* itemCDO = itemClass->GetDefaultObject<AActor>();

	if (itemCDO == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("Error (Null Reference) : itemCDO가 유효하지 않습니다."));

		return;
	}

	UTexture2D* iconTexture = nullptr;
	FString rawName = itemClass->GetName();

	// AItem 타입 확인
	if (AItem* item = Cast<AItem>(itemCDO))
	{
		iconTexture = item->itemIcon;
	}
	// AGun 타입 확인
	else if (AGun* gun = Cast<AGun>(itemCDO))
	{
		iconTexture = gun->itemIcon;
	}

	// 아이템 이름 정리 및 설정
	if (itemNameText != nullptr)
	{
		FString cleanName = CleanItemName(rawName);
		itemNameText->SetText(FText::FromString(cleanName));
	}

	if (itemIconImage == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
			FString::Printf(TEXT("Error (Null Reference) : 아이콘 이미지 또는 텍스처가 유효하지 않습니다. 아이템 클래스: %s"), *rawName));
	}

	// 아이콘 이미지 설정
	itemIconImage->SetBrushFromTexture(iconTexture);

	// 가격 설정 (임시로 고정값 사용)
	if (itemPriceText == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
			FString::Printf(TEXT("Error (Null Reference) : itemPriceText가 유효하지 않습니다. 아이템 클래스: %s"), *rawName));
	}

	itemPriceText->SetText(FText::FromString(TEXT("$1000")));
}

FString UItemShopBuyButtonWidget::CleanItemName(const FString& rawName) const
{
	FString cleanName = rawName;

	// 1. 접두사 제거 (대소문자 구분 없이)
	TArray<FString> removePrefixes = {
		TEXT("Gun")
	};

	for (const FString& prefix : removePrefixes)
	{
		if (cleanName.StartsWith(prefix, ESearchCase::IgnoreCase))
		{
			cleanName = cleanName.RightChop(prefix.Len());
		}
	}

	// 2. 접미사 제거 (대소문자 구분 없이)
	TArray<FString> removeSuffixes = {
		TEXT("Blueprint"),
		TEXT("_C")
	};

	for (const FString& suffix : removeSuffixes)
	{
		cleanName = cleanName.Replace(*suffix, TEXT(""), ESearchCase::IgnoreCase);
	}

	// 3. 언더스코어를 공백으로 변경
	cleanName = cleanName.Replace(TEXT("_"), TEXT(" "));

	// 4. 연속된 공백 제거
	while (cleanName.Contains(TEXT("  ")))
	{
		cleanName = cleanName.Replace(TEXT("  "), TEXT(" "));
	}

	// 5. 앞뒤 공백 제거
	cleanName.TrimStartAndEndInline();

	return cleanName;
}

void UItemShopBuyButtonWidget::OnItemButtonClicked()
{
	if (currentItemClass)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, 
			FString::Printf(TEXT("아이템 구매 클릭: %s"), *currentItemClass->GetName()));
		
		// TODO: 실제 구매 로직 구현
	}
}

