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

	// ������ Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	UStaticMeshComponent* itemMesh;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float interactionDistance = 120.0f;

	virtual void UseItem() {};		// ������ ��� �޼ҵ�

protected:
	virtual void BeginPlay() override;

	bool CheckPlayerIsClose();		// �÷��̾ �ð�� ������� Ȯ���ϴ� �޼ҵ�

	class ACharacter* player;
	class APlayerController* playerController;
	class UInputComponent* playerInputComponent;
	class UPlayerInventory* playerInventory;

	bool isPlayerClose = false;
};
