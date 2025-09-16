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
	void AddMoney();				// 총알을 추가하는 메소드

	int addMoney = 100;
};
