// QuestManager.h 수정
#pragma once

#include "CoreMinimal.h"
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

protected:
    // 상호작용 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    float interactionDistance = 200.0f;

    // 대화 UI 클래스 레퍼런스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UChatUI> chatUIClass;

    // 대화 UI 인스턴스
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    class UChatUI* chatUIInstance;

    class ACharacter* player;
    class APlayerController* playerController;
    class UInputComponent* playerInputComponent;

    // 대화 UI가 표시되어 있는지 여부
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    bool isChatUIVisible = false;
};
