#include "QuestItemUI.h"

#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Internationalization/Text.h"

bool UQuestItemUI::Initialize()
{
    bool isSuccess = Super::Initialize();

    if (!isSuccess)
        return false;

    // 필수 위젯 확인
    if (questNameText == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 이름 텍스트가 바인딩되지 않았습니다."));
        return false;
    }

	if (shortContentText == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 짧은 내용 텍스트가 바인딩되지 않았습니다."));

		return false;
	}

    if (questBorder == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 테두리 바인딩되지 않았습니다."));

        return false;
    }

    if (moneyIcon == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("돈 아이콘이 바인딩되지 않았습니다."));

        return false;
    }

    if (goldIconTexture == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("돈 아이콘 텍스처가 바인딩되지 않았습니다."));

        return false;
    }

    if (expIcon == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("경험치 아이콘이 바인딩되지 않았습니다."));

        return false;
    }

    if (expIconTexture == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("경험치 아이콘 텍스처가 바인딩되지 않았습니다."));

        return false;
    }

    if (questDropListUI == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 드롭 리스트 UI가 바인딩되지 않았습니다."));
		return false;
    }

    // 아이콘 설정
    moneyIcon->SetBrushFromTexture(goldIconTexture);
    expIcon->SetBrushFromTexture(expIconTexture);

    return true;
}

void UQuestItemUI::NativeConstruct()
{
    Super::NativeConstruct();

    if (!questInfo.questName.IsEmpty())
    {
        // 위젯이 생성될 때 텍스트 업데이트
        questNameText->SetText(FText::FromString(questInfo.questName));

        // 짧은 내용 설정
        if (shortContentText)
        {
            shortContentText->SetText(FText::FromString(questInfo.shortContent));
        }

        // 보상 정보 업데이트
        UpdateRewardsInfo();
    }
}

void UQuestItemUI::NativeOnListItemObjectSet(UObject* listItemObject)
{
    // ListView에서 이 위젯이 항목으로 설정될 때 호출됨
    UQuestItemUI* sourceItem = Cast<UQuestItemUI>(listItemObject);

    // 다른 QuestItemUI 객체로부터 정보 가져오기
    if (sourceItem)
        SetQuestInfo(sourceItem->GetQuestInfo());

    else
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("NativeOnListItemObjectSet: 지원되지 않는 항목 유형"));
}

void UQuestItemUI::SetQuestInfo(const FQuestInfo& inQuestInfo)
{
    questInfo = inQuestInfo;

    // 퀘스트 이름 설정
    questNameText->SetText(FText::FromString(questInfo.questName));

    // 짧은 내용 설정
    shortContentText->SetText(FText::FromString(questInfo.shortContent));

    // 짧은 내용이 없으면 숨기기
    if (questInfo.shortContent.IsEmpty())
        shortContentText->SetVisibility(ESlateVisibility::Collapsed);

    else
        shortContentText->SetVisibility(ESlateVisibility::Visible);

    // 보상 정보 업데이트
    UpdateRewardsInfo();
}

void UQuestItemUI::UpdateRewardsInfo()
{
    FNumberFormattingOptions numberFormat;  
    numberFormat.UseGrouping = true; // 1,000 단위로 구분자 사용

    // 돈 보상 설정
    FText formattedMoney = FText::AsNumber(questInfo.money, &numberFormat);
    moneyRewardText->SetText(FText::Format(FText::FromString(TEXT("{0} Gold")), formattedMoney));


    // 경험치 보상 설정
    FText formattedExp = FText::AsNumber(questInfo.experiencePoints, &numberFormat);
    expRewardText->SetText(FText::Format(FText::FromString(TEXT("{0} XP")), formattedExp));
}

FReply UQuestItemUI::NativeOnMouseMove(const FGeometry& inGeometry, const FPointerEvent& inMouseEvent)
{
    // 마우스가 이동할 때마다 드롭다운 위치 업데이트
    if (currentShowQuestDropListUI != nullptr && currentShowQuestDropListUI->GetVisibility() == ESlateVisibility::Visible)
    {
        FVector2D mousePosition = inMouseEvent.GetScreenSpacePosition();

        // 커서 위치에 바로 표시
        currentShowQuestDropListUI->ShowAtLocation(mousePosition);
    }

    return Super::NativeOnMouseMove(inGeometry, inMouseEvent);
}

void UQuestItemUI::NativeOnMouseEnter(const FGeometry& inGeometry, const FPointerEvent& inMouseEvent)
{
    Super::NativeOnMouseEnter(inGeometry, inMouseEvent);

    // rewards 배열이 비어있는지 확인
    if (questInfo.rewardItems.Num() == 0)
        return;

    // 드롭다운 UI가 아직 생성되지 않았으면 생성
    if (currentShowQuestDropListUI == nullptr)
    {
        currentShowQuestDropListUI = CreateWidget<UQuestDropListUI>(GetWorld(), questDropListUI);

        if (currentShowQuestDropListUI == nullptr)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("퀘스트 드롭 리스트 UI 인스턴스 생성 실패"));

            return;
        }

        currentShowQuestDropListUI->AddToViewport(10); // 높은 Z-Order로 추가

        // 아이템 개수 정보와 함께 전달
        currentShowQuestDropListUI->SetRewardItemsWithQuantity(questInfo.rewardItems);
    }

    // 마우스 위치 가져오기
    FVector2D mousePosition = inMouseEvent.GetScreenSpacePosition();

    // 커서 바로 위에 표시 (오프셋 조정)
    currentShowQuestDropListUI->ShowAtLocation(mousePosition);
}

void UQuestItemUI::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    if (currentShowQuestDropListUI == nullptr)
        return;

    // 마우스가 떠나면 드롭다운 UI 숨기기
    currentShowQuestDropListUI->Hide();
}