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

	virtual void Tick(float DeltaTime) override;

	// 아이템 Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	UStaticMeshComponent* itemMesh;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float interactionDistance = 120.0f;

	// 아이템 Mesh를 제거하는 메소드
	UFUNCTION(BlueprintCallable, Category = "Item")
	void RemoveItemMesh();
	
	// 아이템을 획득할 수 있는지 설정하는 메소드
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetGetable(bool isGetable);	

	virtual void UseItem() {};		// 아이템 사용 메소드

protected:
	virtual void BeginPlay() override;

	bool CheckPlayerIsClose();			// 플레이어가 시계와 가까운지 확인하는 메소드

	class ACharacter* player;
	class APlayerController* playerController;
	class UInputComponent* playerInputComponent;
	class UPlayerInventory* playerInventory;

	bool isGetable = true;			// 아이템을 획득할 수 있는지 여부
	bool isPlayerClose = false;
};
