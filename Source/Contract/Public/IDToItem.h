#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Item.h"
#include "IDToItem.generated.h"


UCLASS()
class CONTRACT_API UIDToItem : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TMap<FString, TSubclassOf<AItem>> stringToitems; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TMap<int, TSubclassOf<AItem>> intToitems;
};
