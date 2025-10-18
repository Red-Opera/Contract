#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "ItemShop.generated.h"

UCLASS()
class CONTRACT_API AItemShop : public AActor
{
	GENERATED_BODY()

public:
	AItemShop();

	virtual void Tick(float deltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* root;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision")
	UBoxComponent* shopBox;

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool isFromSweep, const FHitResult& sweepResult);

	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

private:
	bool isPlayerInside;

	void SetupPlayerInput();
	void OnInteractionPressed();

};