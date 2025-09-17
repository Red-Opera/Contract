#pragma once

#include "CoreMinimal.h"
#include "QuestList.h"
#include "GameFramework/Character.h"
#include "QuestManager.generated.h"

UCLASS()
class CONTRACT_API AQuestManager : public ACharacter
{
    GENERATED_BODY()

public:
    AQuestManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // 플레이어와의 상호작용 처리 함수
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void InteractWithPlayer();

    // 플레이어가 가까이 있는지 확인하는 함수
    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool IsPlayerClose() const;

    // 대화 UI 표시
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowChatUI();

    // 대화 UI 숨기기
    UFUNCTION(BlueprintCallable, Category = "UI")
    void HideChatUI();

    // 대화 UI가 표시되어 있는지 확인
    UFUNCTION(BlueprintPure, Category = "UI")
    bool GetChatUIVisible() const { return isChatUIVisible; }

    // 퀘스트 목록 가져오기
    UFUNCTION(BlueprintPure, Category = "Quest")
    const TArray<FQuestInfo>& GetQuests() const { return quests; }

protected:
    // 대화가 끝나면 퀘스트 추가
    void AddQuestsAfterDialogue();

    // 상호작용 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    float interactionDistance = 200.0f;

    // 대화 UI 클래스 레퍼런스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UChatUI> chatUIClass;

    // 대화 UI 인스턴스
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    class UChatUI* chatUIInstance;

    // 퀘스트 목록
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<FQuestInfo> quests;

    // 퀘스트 데이터 에셋
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    UQuestList* questListAsset;

    class ACharacter* player;
    class APlayerController* playerController;
    class UInputComponent* playerInputComponent;

    // 대화 UI가 표시되어 있는지 여부
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    bool isChatUIVisible = false;
};