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
	TSubclassOf<AActor> fireMesh;		// ���� �޽�
	
	// �浹 ������Ʈ �߰�
	UPROPERTY(EditAnywhere)
	class UCapsuleComponent* collisionComponent;

protected:
	virtual void BeginPlay() override;
	virtual void UseItem() override;	// ������ ��� �޼ҵ�

private:
	void AddMolotovCocktail();			// ȭ������ �߰��ϴ� �޼ҵ�
	void RemoveActor();					// ȭ���� ���� �޼ҵ�	
	void SpawnFireActor(const FVector& SpawnLocation);  // ȭ�� ���� ���� �޼���

	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	class AActor* fireActor;			// ���� ����

	FTimerHandle timerHandle;			// Ÿ�̸� �ڵ�

	bool isUse;
};
