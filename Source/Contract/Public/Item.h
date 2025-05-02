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

	// ������ Mesh�� �����ϴ� �޼ҵ�
	UFUNCTION(BlueprintCallable, Category = "Item")
	void RemoveItemMesh();

	// �������� ȹ���� �� �ִ��� �����ϴ� �޼ҵ�
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetGetable(bool isGetable);

	// ���ͷ������� ������ ȹ�� �� ȣ���ϴ� ���� �޼ҵ�
	UFUNCTION(BlueprintCallable, Category = "Item")
	static AItem* GetClosestInteractableItem(ACharacter* FromCharacter);

	// �÷��̾�� ������ ���� �Ÿ� Ȯ�� �޼ҵ�
	float GetDistanceToPlayer() const;

	virtual void Tick(float DeltaTime) override;
	virtual void UseItem();							// ������ ��� �޼ҵ�

	// ������ Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item")
	UStaticMeshComponent* itemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float interactionDistance = 120.0f;

protected:
	virtual void BeginPlay() override;

	// �÷��̾ �����۰� ������� Ȯ���ϴ� �޼ҵ�
	bool CheckPlayerIsClose();

	class ACharacter* player;
	class APlayerController* playerController;
	class UInputComponent* playerInputComponent;
	class UPlayerInventory* playerInventory;

	bool isGetable = true;			// �������� ȹ���� �� �ִ��� ����
	bool isPlayerClose = false;

	// ��ȣ�ۿ� ó�� ������ Ȯ���ϴ� ���� ����
	static bool isInteractionInProgress;
};
