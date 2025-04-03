#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerItem.generated.h"

UCLASS()
class CONTRACT_API UPlayerItem : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	int32 maxBullets = 30;
	
private:
};
