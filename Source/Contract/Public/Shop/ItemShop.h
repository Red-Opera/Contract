#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "ItemShopWidgetStart.h"
#include "ItemShop.generated.h"

UCLASS()
class CONTRACT_API AItemShop : public AActor
{
	GENERATED_BODY()

public:
	AItemShop();

	virtual void Tick(float deltaTime) override;

	static void SetItemShopWidgetOpen(bool isOpen);
	static bool IsItemShopWidgetCurrentlyOpen();
	static void EnableESCBinding(bool enable);
	static AItemShop* GetCurrentItemShop();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* root;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision")
	UBoxComponent* shopBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* shopCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> itemShopStartWidgetClass;

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool isFromSweep, const FHitResult& sweepResult);

	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

private:
	bool isPlayerInside;
	bool isShopOpen;

	static bool isItemShopWidgetOpen;

	UPROPERTY()
	UUserWidget* itemShopStartWidgetInstance;

	APlayerController* playerController;
	AActor* originalViewTarget;

	FInputActionBinding* escBinding;

	void SetupPlayerInput();
	void SwitchToShopCamera();
	void RestoreOriginalCamera();
	void CloseShop();

	void OnInteractionPressed();
	void OnESCPressed();

private:
	static AItemShop* currentItemShop;
};
