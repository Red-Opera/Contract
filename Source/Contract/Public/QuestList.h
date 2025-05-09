#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QuestList.generated.h"

class AItem;

USTRUCT(BlueprintType)
struct FQuestInfo
{
	GENERATED_BODY()

	// Quest name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString questName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString content;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FString conditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<AItem*> rewards;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int money;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int experiencePoints;
};

UCLASS()
class CONTRACT_API UQuestList : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FQuestInfo> quests;
};
