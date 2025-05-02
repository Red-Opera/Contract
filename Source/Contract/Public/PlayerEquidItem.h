#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerInventory.h"
#include "PlayerEquidItem.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CONTRACT_API UPlayerEquidItem : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerEquidItem();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// �÷��̾� �Է��� ó���ϴ� �Լ�
	UFUNCTION(BlueprintCallable, Category = "Player Equipment")
	bool PlayerInventoryDataLoad();

	// ������ ���� �Լ�
	UFUNCTION(BlueprintCallable, Category = "Player Equipment")
	AActor* SpawnItemAtSocket(TSubclassOf<AActor> itemClass, FName socketName);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Equipment")
	int itemSelectIndex = 0;

protected:
	virtual void BeginPlay() override;

	// �κ��丮 ������ �ּ� ���
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSoftObjectPtr<UPlayerInventory> InventoryDataAsset = TSoftObjectPtr<UPlayerInventory>(FSoftObjectPath(TEXT("/Game/PlayerInventory/PlayerInventory.PlayerInventory")));

	// ����ź Ŭ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSubclassOf<AActor> GrenadeClass;

	// ȭ���� Ŭ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSubclassOf<AActor> molotovClass;

	// ���� �� �� Ŭ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSubclassOf<AActor> smallHealPackClass;
	
	// ���� �� �� Ŭ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSubclassOf<AActor> largeHealPackClass;

	// �������� ������ ���� �̸�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Equipment")
	FName attachSocketName = FName("index_01_r");

private:
	// �κ��丮 ������ �ε�
	void LoadInventoryData();

	// ������ ��ô
	void ThrowItemTrigger();
	void ThrowItem();

	
	void SpawnGrenade();		// ����ź ����
	void SpawnMolotov();		// ȭ���� ����
	void SpawnSmallHealPack();	// ���� �� �� ����
	void SpawnLargeHealPack();	// ���� �� �� ����

	class ACharacter* player;
	class APlayerController* playerController;
	class UInputComponent* playerInputComponent;
	class USkeletalMeshComponent* targetSkeletalMesh;	// �������� ������ ���̷�Ż �޽� ������Ʈ
	class UAnimInstance* playerAnimInstance;			// �÷��̾� �ִϸ��̼� �������Ʈ

	// �ε�� �κ��丮 ������ �ּ� ����
	UPlayerInventory* playerInventoryData;

	// ���� ������ ������
	UPROPERTY()
	AActor* currentEquippedItem;

	// ��� ���Ͽ� Ÿ�̸� �ڵ� ����
	FTimerHandle timerHandle_AutoThrow;
};
