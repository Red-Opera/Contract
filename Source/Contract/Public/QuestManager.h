// QuestManager.h ����
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

    // �÷��̾���� ��ȣ�ۿ� ó�� �Լ�
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void InteractWithPlayer();

    // �÷��̾ ������ �ִ��� Ȯ���ϴ� �Լ�
    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool IsPlayerClose() const;

    // ��ȭ UI ǥ��
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowChatUI();

    // ��ȭ UI �����
    UFUNCTION(BlueprintCallable, Category = "UI")
    void HideChatUI();

    // ��ȭ UI�� ǥ�õǾ� �ִ��� Ȯ��
    UFUNCTION(BlueprintPure, Category = "UI")
    bool GetChatUIVisible() const { return isChatUIVisible; }

protected:
    // ��ȣ�ۿ� �Ÿ�
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    float interactionDistance = 200.0f;

    // ��ȭ UI Ŭ���� ���۷���
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UChatUI> chatUIClass;

    // ��ȭ UI �ν��Ͻ�
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    class UChatUI* chatUIInstance;

    class ACharacter* player;
    class APlayerController* playerController;
    class UInputComponent* playerInputComponent;

    // ��ȭ UI�� ǥ�õǾ� �ִ��� ����
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    bool isChatUIVisible = false;
};
