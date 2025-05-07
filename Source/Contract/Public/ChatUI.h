#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "ChatUI.generated.h"

// 대화 정보를 저장하는 구조체
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
    // 위젯 초기화
    virtual bool Initialize() override;

    // Tick 함수 재정의
    virtual void NativeTick(const FGeometry& myGeometry, float deltaTime) override;

    // 대화 시작 메소드
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void StartDialogue();

    // 다음 대화 내용으로 전환하는 메소드
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void NextMessage();

    // 대화 종료 메소드
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void EndDialogue();

    // 현재 대화 중인지 확인
    UFUNCTION(BlueprintPure, Category = "Chat")
    bool GetDialogueActive() const { return isDialogueActive; }

    // 대화 메시지 표시 업데이트
    void UpdateDialogueDisplay();

    // 현재 대화 메시지 배열
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat")
    TArray<FChatMessage> dialogueMessages;

protected:
    // 스페이스 키 입력 처리
    void HandleSpaceKeyPress();

    // 현재 메시지 인덱스
    UPROPERTY(BlueprintReadOnly, Category = "Chat")
    int32 currentMessageIndex = -1;

    // 대화 중인지 여부를 나타내는 플래그
    UPROPERTY(BlueprintReadOnly, Category = "Chat")
    bool isDialogueActive = false;

    // ===================================
    // UI 요소들
    // ===================================

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
    class UImage* backgroundImage;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
    class UBorder* dialogueBorder;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
    class UTextBlock* speakerNameText;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
    class UTextBlock* messageText;

    // 배경 이미지 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat|UI")
    UTexture2D* defaultBackgroundTexture;

    // 대화창 색상 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat|UI")
    FLinearColor dialogueBorderColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.8f);

    // 계속하려면 스페이스 텍스트 표시
    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "UI")
    class UTextBlock* pressSpaceText;

    UPROPERTY(BlueprintReadOnly, Category = "Chat|Typing")
    FString currentDisplayedText;                           // 현재 표시된 텍스트

    UPROPERTY(BlueprintReadOnly, Category = "Chat|Typing")
    FString fullTextToDisplay;                              // 표시해야 할 전체 텍스트

    UPROPERTY(BlueprintReadOnly, Category = "Chat|Typing")
    bool isTypingEffect = false;                            // 타이핑 효과 진행 중인지 여부

    UPROPERTY(BlueprintReadOnly, Category = "Chat|Typing")
    float typingTimer = 0.0f;                               // 타이핑 

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chat|Typing")
    float typingSpeed = 0.03f;                              // 글자가 나타나는 간격(초)

	class APlayerController* playerController;                      // 플레이어 컨트롤러   
    class UCharacterMovementComponent* playerMovementComponent;		// 플레이어 이동관련 컴포넌트
};
