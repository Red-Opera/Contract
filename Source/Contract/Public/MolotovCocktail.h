#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "MolotovCocktail.generated.h"

UCLASS()
class CONTRACT_API AMolotovCocktail : public AItem
{
	GENERATED_BODY()

public:
	AMolotovCocktail();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSubclassOf<AActor> fireMesh;		// 폭파 메쉬
	
	// 충돌 컴포넌트 추가
	UPROPERTY(EditAnywhere)
	class UCapsuleComponent* collisionComponent;

protected:
	virtual void BeginPlay() override;
	virtual void UseItem() override;	// 아이템 사용 메소드

private:
	void AddMolotovCocktail();			// 화염병을 추가하는 메소드
	void RemoveActor();					// 화염병 제거 메소드	
	void SpawnFireActor(const FVector& SpawnLocation);  // 화염 액터 생성 메서드

	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	class AActor* fireActor;			// 폭파 액터

	FTimerHandle timerHandle;			// 타이머 핸들

	bool isUse;
};
