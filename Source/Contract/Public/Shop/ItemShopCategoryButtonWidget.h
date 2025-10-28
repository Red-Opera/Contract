#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ItemShopCategoryButtonWidget.generated.h"

UCLASS()
class CONTRACT_API UItemShopCategoryButtonWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Category")
	int32 listIndex;

	UPROPERTY(meta = (BindWidget))
	UButton* categoryButton;

	void SetListIndex(int32 index);
	int32 GetListIndex() const { return listIndex; }

private:
	UFUNCTION()
	void OnCategoryButtonClicked();
};
