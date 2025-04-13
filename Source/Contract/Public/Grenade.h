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

	void AddGrenade();	// 수류탄을 추가하는 메소드

protected:
	virtual void BeginPlay() override;
	virtual void UseItem() override;		// 아이템 사용 메소드

private:
};
