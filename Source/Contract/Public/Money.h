#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Money.generated.h"

UCLASS()
class CONTRACT_API AMoney : public AItem
{
	GENERATED_BODY()

public:
	AMoney() = default;

protected:
	virtual void BeginPlay() override;

private:
	void AddMoney();				// �Ѿ��� �߰��ϴ� �޼ҵ�

	int addMoney = 100;
};
