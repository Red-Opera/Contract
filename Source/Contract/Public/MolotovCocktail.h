#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "MolotovCocktail.generated.h"

UCLASS()
class CONTRACT_API AMolotovCocktail : public AItem
{
	GENERATED_BODY()

public:
	AMolotovCocktail() = default;

protected:
	virtual void BeginPlay() override;

private:
	void AddMolotovCocktail();	// 수류탄을 추가하는 메소드
};
