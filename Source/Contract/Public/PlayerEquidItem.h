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

	// 플레이어 입력을 처리하는 함수
	UFUNCTION(BlueprintCallable, Category = "Player Equipment")
	bool PlayerInventoryDataLoad();

	// 아이템 스폰 함수
	UFUNCTION(BlueprintCallable, Category = "Player Equipment")
	AActor* SpawnItemAtSocket(TSubclassOf<AActor> itemClass, FName socketName);

protected:
	virtual void BeginPlay() override;

	// 인벤토리 데이터 애셋 경로
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSoftObjectPtr<UPlayerInventory> InventoryDataAsset = TSoftObjectPtr<UPlayerInventory>(FSoftObjectPath(TEXT("/Game/PlayerInventory/PlayerInventory.PlayerInventory")));

	// 수류탄 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSubclassOf<AActor> GrenadeClass;

	// 화염병 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSubclassOf<AActor> MolotovClass;

	// 아이템을 부착할 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Equipment")
	FName AttachSocketName = FName("index_01_r");

private:
	// 인벤토리 데이터 로드
	void LoadInventoryData();

	// 아이템 투척
	void ThrowItemTrigger();
	void ThrowItem();

	// 수류탄 스폰
	void SpawnGrenade();

	// 화염병 스폰
	void SpawnMolotov();

	class ACharacter* player;
	class APlayerController* playerController;
	class UInputComponent* playerInputComponent;
	class USkeletalMeshComponent* targetSkeletalMesh;	// 아이템을 부착할 스켈레탈 메시 컴포넌트
	class UAnimInstance* playerAnimInstance;			// 플레이어 애니메이션 블루프린트

	// 로드된 인벤토리 데이터 애셋 참조
	UPlayerInventory* PlayerInventoryData;

	// 현재 장착된 아이템
	UPROPERTY()
	AActor* currentEquippedItem;

	// 헤더 파일에 타이머 핸들 선언
	FTimerHandle timerHandle_AutoThrow;
};
