#include "QuestManager.h"
#include "ChatUI.h"

#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AQuestManager::AQuestManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AQuestManager::BeginPlay()
{
    Super::BeginPlay();

    // �÷��̾� ĳ���� ��������
    player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    playerController = GetWorld()->GetFirstPlayerController();

    if (player == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player does not exist."));
        return;
    }

    if (playerController == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player controller does not exist."));
        return;
    }

	if (chatUIClass == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ChatUI class is not set."));

		return;
	}

	// ��ȭ UI �ν��Ͻ� ����
    chatUIInstance = CreateWidget<UChatUI>(GetWorld()->GetFirstPlayerController(), chatUIClass);

	if (chatUIInstance == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ChatUI instance creation failed."));

		return;
	}

    playerInputComponent = player->InputComponent;

    if (playerInputComponent == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Input Component does not exist."));
        return;
    }

    EnableInput(playerController);

    // ��ȣ�ۿ� Ű ���ε�
    playerInputComponent->BindAction("Interaction", IE_Pressed, this, &AQuestManager::InteractWithPlayer);
}

void AQuestManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AQuestManager::InteractWithPlayer()
{
    // �÷��̾ ������ �ְ� ��ȭ UI�� ǥ�õ��� ���� ��쿡�� ��ȭ UI ǥ��
    if (IsPlayerClose() && !isChatUIVisible)
        ShowChatUI();

    // ��ȭ UI�� �̹� ǥ�õǾ� �ִ� ��� �����
    else if (isChatUIVisible)
        HideChatUI();
}

bool AQuestManager::IsPlayerClose() const
{
    float Distance = FVector::Dist(GetActorLocation(), player->GetActorLocation());

    return Distance <= interactionDistance;
}

void AQuestManager::ShowChatUI()
{
    // ��ȭ UI�� �̹� ǥ�õǰ� �ִ� ���
    if (isChatUIVisible)
        return;

    // ��ȭ ����
    chatUIInstance->StartDialogue();

    // UI ǥ��
    chatUIInstance->AddToViewport();
    isChatUIVisible = true;
}

void AQuestManager::HideChatUI()
{
    if (!isChatUIVisible)
        return;

    // ��ȭ ����
    chatUIInstance->StartDialogue();

    // UI ����
    chatUIInstance->RemoveFromViewport();
    isChatUIVisible = false;
}
