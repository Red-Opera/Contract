#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ItemShopBuyButtonWidget.generated.h"

UCLASS()
class CONTRACT_API UItemShopBuyButtonWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	// 위젯 컴포넌트들
	UPROPERTY(meta = (BindWidget))
	UButton* itemButton;

	UPROPERTY(meta = (BindWidget))
	UImage* itemIconImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* itemNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* itemPriceText;

	// 아이템 정보 설정
	UFUNCTION(BlueprintCallable, Category = "ItemShop")
	void SetItemInfo(TSubclassOf<AActor> itemClass);

	// 현재 아이템 클래스
	UPROPERTY(BlueprintReadOnly, Category = "ItemShop")
	TSubclassOf<AActor> currentItemClass;

private:
	UFUNCTION()
	void OnItemButtonClicked();

	// 아이템 이름 정리 함수
	FString CleanItemName(const FString& rawName) const;
};
