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

	void AddGrenade();	// ����ź�� �߰��ϴ� �޼ҵ�

protected:
	virtual void BeginPlay() override;
	virtual void UseItem() override;		// ������ ��� �޼ҵ�

private:
};
