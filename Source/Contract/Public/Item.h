#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item.generated.h"

UCLASS()
class CONTRACT_API AItem : public AActor
{
	GENERATED_BODY()

public:
	AItem();

	// 아이템 Mesh를 제거하는 메소드
	UFUNCTION(BlueprintCallable, Category = "Item")
	void RemoveItemMesh();

	// 아이템을 획득할 수 있는지 설정하는 메소드
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetGetable(bool isGetable);

	// 인터랙션으로 아이템 획득 시 호출하는 공통 메소드
	UFUNCTION(BlueprintCallable, Category = "Item")
	static AItem* GetClosestInteractableItem(ACharacter* FromCharacter);

	// 플레이어와 아이템 간의 거리 확인 메소드
	float GetDistanceToPlayer() const;

	virtual void Tick(float DeltaTime) override;
	virtual void UseItem();							// 아이템 사용 메소드

	// 아이템 Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item")
	UStaticMeshComponent* itemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float interactionDistance = 120.0f;

protected:
	virtual void BeginPlay() override;

	// 플레이어가 아이템과 가까운지 확인하는 메소드
	bool CheckPlayerIsClose();

	class ACharacter* player;
	class APlayerController* playerController;
	class UInputComponent* playerInputComponent;
	class UPlayerInventory* playerInventory;

	bool isGetable = true;			// 아이템을 획득할 수 있는지 여부
	bool isPlayerClose = false;

	// 상호작용 처리 중인지 확인하는 정적 변수
	static bool isInteractionInProgress;
};
