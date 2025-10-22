#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ItemShopWidget.generated.h"

UCLASS()
class CONTRACT_API UItemShopWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> itemShopStartWidgetClass;

private:
	UPROPERTY()
	UUserWidget* itemShopStartWidgetInstance;

	APlayerController* playerController;

	void SetupInput();
	void OnESCPressed();
};
