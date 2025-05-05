#include "ChatUI.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

bool UChatUI::Initialize()
{
    bool isSuccess = Super::Initialize();

    if (!isSuccess)
        return false;

    // UI 요소 초기화
    if (dialogueBorder != nullptr)
        dialogueBorder->SetBrushColor(dialogueBorderColor);

    if (backgroundImage != nullptr && defaultBackgroundTexture != nullptr)
        backgroundImage->SetBrushFromTexture(defaultBackgroundTexture);

	playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (playerController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 컨트롤러가 존재하지 않습니다."));

		return false;
	}

    // 처음에는 대화창 숨기기
    SetVisibility(ESlateVisibility::Hidden);

    return true;
}

void UChatUI::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 대화가 활성화되어 있을 때만 입력 처리
    if (isDialogueActive)
    {
        if (playerController != nullptr && playerController->WasInputKeyJustPressed(EKeys::SpaceBar))
            HandleSpaceKeyPress();
    }
}

void UChatUI::HandleSpaceKeyPress()
{
    NextMessage();
}

void UChatUI::StartDialogue()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("대화 시작"));

    // 대화 시작 설정
    isDialogueActive = true;
    currentMessageIndex = -1;

    // UI 표시
    SetVisibility(ESlateVisibility::Visible);

    // 첫 메시지 표시
    NextMessage();
}

void UChatUI::NextMessage()
{
    currentMessageIndex++;

    // 모든 메시지를 표시했는지 확인
    if (currentMessageIndex >= dialogueMessages.Num())
    {
        // 대화 종료
        EndDialogue();

        return;
    }

    // 현재 메시지 표시
    UpdateDialogueDisplay();
}

void UChatUI::EndDialogue()
{
    // 대화 종료
    isDialogueActive = false;

    // UI 숨김
    SetVisibility(ESlateVisibility::Hidden);

    // 변수 초기화
    currentMessageIndex = -1;
}

void UChatUI::UpdateDialogueDisplay()
{
    // 현재 메시지 인덱스가 유효한지 확인
    if (currentMessageIndex < 0 || currentMessageIndex >= dialogueMessages.Num())
        return;

    // 현재 메시지 가져오기
    const FChatMessage& currentMessage = dialogueMessages[currentMessageIndex];

    // UI 업데이트
    if (speakerNameText != nullptr)
        speakerNameText->SetText(FText::FromString(currentMessage.speakerName));

    if (messageText != nullptr)
        messageText->SetText(FText::FromString(currentMessage.message));

    // '계속하려면 스페이스' 텍스트 표시
    if (pressSpaceText != nullptr)
    {
        // 마지막 메시지인지 확인
        bool isLastMessage = (currentMessageIndex == dialogueMessages.Num() - 1);
        FString continueText = isLastMessage ? TEXT("종료하려면 스페이스") : TEXT("계속하려면 스페이스");

        pressSpaceText->SetText(FText::FromString(continueText));
    }
}
