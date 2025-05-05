#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "ChatUI.generated.h"

// ��ȭ ������ �����ϴ� ����ü
USTRUCT(BlueprintType)
struct FChatMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat")
    FString speakerName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat")
    FString message;

    FChatMessage() : speakerName(TEXT("")), message(TEXT("")) {}

    FChatMessage(const FString& inSpeakerName, const FString& inMessage)
        : speakerName(inSpeakerName), message(inMessage)
    {

    }
};

UCLASS()
class CONTRACT_API UChatUI : public UUserWidget
{
    GENERATED_BODY()

public:
    // ���� �ʱ�ȭ
    virtual bool Initialize() override;

    // Tick �Լ� ������
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ��ȭ ���� �޼ҵ�
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void StartDialogue();

    // ���� ��ȭ �������� ��ȯ�ϴ� �޼ҵ�
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void NextMessage();

    // ��ȭ ���� �޼ҵ�
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void EndDialogue();

    // ���� ��ȭ ������ Ȯ��
    UFUNCTION(BlueprintPure, Category = "Chat")
    bool GetDialogueActive() const { return isDialogueActive; }

    // ��ȭ �޽��� ǥ�� ������Ʈ
    void UpdateDialogueDisplay();

    // ���� ��ȭ �޽��� �迭
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat")
    TArray<FChatMessage> dialogueMessages;

protected:
    // �����̽� Ű �Է� ó��
    void HandleSpaceKeyPress();

    // ���� �޽��� �ε���
    UPROPERTY(BlueprintReadOnly, Category = "Chat")
    int32 currentMessageIndex = -1;

    // ��ȭ ������ ���θ� ��Ÿ���� �÷���
    UPROPERTY(BlueprintReadOnly, Category = "Chat")
    bool isDialogueActive = false;

    // ===================================
    // UI ��ҵ�
    // ===================================

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
    class UImage* backgroundImage;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
    class UBorder* dialogueBorder;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
    class UTextBlock* speakerNameText;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
    class UTextBlock* messageText;

    // ��� �̹��� ����
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat|UI")
    UTexture2D* defaultBackgroundTexture;

    // ��ȭâ ���� ����
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat|UI")
    FLinearColor dialogueBorderColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.8f);

    // ǥ�� Ʈ������ ȿ�� Ȱ��ȭ
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat|UI")
    bool bEnableTextTransition = true;

    // ����Ϸ��� �����̽� �ؽ�Ʈ ǥ��
    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
    class UTextBlock* pressSpaceText;

	class APlayerController* playerController;
};
