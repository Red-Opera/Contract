#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QuestList.generated.h"

class AItem;

// 아이템과 개수를 함께 저장하는 구조체
USTRUCT(BlueprintType)
struct FItemCount
{
    GENERATED_BODY()

    FItemCount() : itemClass(nullptr), count(1)
    {

    }

    FItemCount(TSubclassOf<AItem> itemClass, int32 count = 1) 
        : itemClass(itemClass), count(count)
    {

    }

    // 아이템 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSubclassOf<AItem> itemClass;

    // 아이템 개수 (기본값 1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
    int32 count = 1;
};

USTRUCT(BlueprintType)
struct FQuestInfo
{
    GENERATED_BODY()

    // Quest name
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FString questName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FString shortContent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FString content;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FString conditions;

    // 보상 아이템과 개수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<FItemCount> rewardItems;

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