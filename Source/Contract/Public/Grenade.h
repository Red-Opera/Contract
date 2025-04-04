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

protected:
	virtual void BeginPlay() override;

private:
	
	void AddGrenade();	// 수류탄을 추가하는 메소드
};
