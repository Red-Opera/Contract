#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ItemShopStartButtonWidget.generated.h"

UCLASS()
class CONTRACT_API UItemShopStartButtonWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(meta = (BindWidget))
	UButton* mainButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	int32 buttonIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	TSubclassOf<UUserWidget> itemShopWidgetClass;

	void SetButtonIndex(int32 index);

	UFUNCTION()
	void OnMainButtonClicked();

private:
	UPROPERTY()
	UUserWidget* itemShopWidgetInstance;
	
};
