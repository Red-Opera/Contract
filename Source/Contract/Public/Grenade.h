#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Grenade.generated.h"

UCLASS()
class CONTRACT_API AGrenade : public AItem
{
	GENERATED_BODY()

public:
	AGrenade() = default;

	void AddGrenade();		// 수류탄을 추가하는 메소드

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSubclassOf<AActor> explosionMesh;				// 폭파 메쉬

protected:
	virtual void BeginPlay() override;
	virtual void UseItem() override;				// 아이템 사용 메소드

private:
	void Explosion();								// 폭파 메소드
	void RemoveGrenade();							// 수류탄 제거 메소드

	class AActor* explosionActor;					// 폭파 액터

	FTimerHandle timerHandle;						// 타이머 핸들
};
