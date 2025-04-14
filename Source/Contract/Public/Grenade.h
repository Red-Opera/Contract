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

	void AddGrenade();		// ����ź�� �߰��ϴ� �޼ҵ�

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
	TSubclassOf<AActor> explosionMesh;				// ���� �޽�

protected:
	virtual void BeginPlay() override;
	virtual void UseItem() override;				// ������ ��� �޼ҵ�

private:
	void Explosion();								// ���� �޼ҵ�
	void RemoveGrenade();							// ����ź ���� �޼ҵ�

	class AActor* explosionActor;					// ���� ����

	FTimerHandle timerHandle;						// Ÿ�̸� �ڵ�
};
