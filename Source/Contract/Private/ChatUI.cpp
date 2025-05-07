#include "ChatUI.h"
#include "Components/Border.h"
#include "Components/InputComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

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

	ACharacter* player = playerController->GetCharacter();

	if (player == nullptr)
	{
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 캐릭터가 존재하지 않습니다."));

        return false;
    }

	playerMovementComponent = Cast<UCharacterMovementComponent>(player->GetMovementComponent());

    if (playerMovementComponent == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 이동 컴포넌트가 존재하지 않습니다."));

		return false;
    }

    // 처음에는 대화창 숨기기
    SetVisibility(ESlateVisibility::Hidden);

    return true;
}

void UChatUI::NativeTick(const FGeometry& myGeometry, float deltaTime)
{
    Super::NativeTick(myGeometry, deltaTime);

    // 타이핑 효과 업데이트
    if (isTypingEffect && messageText != nullptr)
    {
        typingTimer += deltaTime;

        if (typingTimer >= typingSpeed)
        {
            typingTimer = 0.0f;

            // 표시할 다음 글자 추가
            if (currentDisplayedText.Len() < fullTextToDisplay.Len())
            {
                currentDisplayedText.AppendChar(fullTextToDisplay[currentDisplayedText.Len()]);
                messageText->SetText(FText::FromString(currentDisplayedText));
            }

            else
            {
                // 타이핑 효과 완료
                isTypingEffect = false;

                // 타이핑 효과가 끝났으므로 '계속하려면 스페이스' 텍스트 표시
                if (pressSpaceText != nullptr)
                    pressSpaceText->SetVisibility(ESlateVisibility::Visible);

            }
        }
    }

    // 대화가 활성화되어 있을 때만 입력 처리
    if (isDialogueActive && playerController)
    {
        if (playerController->WasInputKeyJustPressed(EKeys::SpaceBar))
            HandleSpaceKeyPress();

        if (playerController->WasInputKeyJustPressed(EKeys::Escape))
            EndDialogue();
    }
}

void UChatUI::HandleSpaceKeyPress()
{
    // 타이핑 효과가 진행 중이면 즉시 모든 텍스트 표시
    if (isTypingEffect)
    {
        isTypingEffect = false;
        currentDisplayedText = fullTextToDisplay;

        if (messageText != nullptr)
            messageText->SetText(FText::FromString(currentDisplayedText));


        // '계속하려면 스페이스' 텍스트 표시
        if (pressSpaceText != nullptr)
            pressSpaceText->SetVisibility(ESlateVisibility::Visible);
    }

    // 다음 메시지로 이동
    else
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

	// UI 배경 및 대화창 색상 설정
    playerMovementComponent->DisableMovement();
    playerController->SetIgnoreLookInput(true);

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

	// 플레이어 이동 및 입력 복원
    playerMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
    playerController->SetIgnoreLookInput(false);

    RemoveFromViewport();
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

    // 타이핑 효과 시작
    if (messageText != nullptr)
    {
        // \\n을 \n으로 변환하여 줄바꿈 처리
        FString formattedMessage = currentMessage.message.Replace(TEXT("\\n"), TEXT("\n"));

        // 타이핑 효과 변수 초기화
        fullTextToDisplay = formattedMessage;
        currentDisplayedText = TEXT("");
        isTypingEffect = true;
        typingTimer = 0.0f;

        // 텍스트 초기화
        messageText->SetText(FText::FromString(TEXT("")));


        // 타이핑 효과 진행 중에는 '계속하려면 스페이스' 텍스트 숨기기
        if (pressSpaceText != nullptr)
        {
            pressSpaceText->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    // '계속하려면 스페이스' 텍스트 내용 설정 (타이핑 효과가 끝나면 표시됨)
    if (pressSpaceText != nullptr)
    {
        // 마지막 메시지인지 확인
        bool isLastMessage = (currentMessageIndex == dialogueMessages.Num() - 1);
        FString continueText = isLastMessage ? TEXT("종료하려면 스페이스") : TEXT("계속하려면 스페이스");

        pressSpaceText->SetText(FText::FromString(continueText));
    }
}