#pragma once

#include "CoreMinimal.h"
#include "QuestList.h"

#include "Blueprint/UserWidget.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "QuestUI.generated.h"

UCLASS()
class CONTRACT_API UQuestUI : public UUserWidget
{
    GENERATED_BODY()

public:
    // 위젯 초기화
    virtual bool Initialize() override;

    // Tick 함수 재정의
    virtual void NativeTick(const FGeometry& myGeometry, float deltaTime) override;

    // 퀘스트 목록 설정
    void SetQuestList(UQuestList* InQuestList);

    // 퀘스트 UI 표시
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void ShowQuestUI();

    // 퀘스트 UI 숨기기
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void HideQuestUI();

    // P 키로 퀘스트 UI 토글
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void ToggleQuestUI();

    // 현재 퀘스트 UI 활성화 여부 확인
    UFUNCTION(BlueprintPure, Category = "Quest")
    bool GetQuestUIActive() const { return isQuestUIActive; }

protected:
    // 퀘스트 선택 이벤트 처리
    UFUNCTION()
    void OnQuestSelected(UObject* item);

    // 닫기 버튼 이벤트 처리
    UFUNCTION()
    void OnCloseButtonClicked();

    // 퀘스트 목록 갱신
    void RefreshQuestList();

    // 퀘스트 상세 정보 표시
    void DisplayQuestDetails(const FQuestInfo& questInfo);

    // 퀘스트 UI가 활성화되어 있는지 여부
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    bool isQuestUIActive = false;

    // ===================================
    // UI 요소들
    // ===================================

    // 퀘스트 목록 데이터
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    UQuestList* questListData;

    // 퀘스트 목록 뷰
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
    UListView* questListView;

    // 퀘스트 세부 정보 영역
    UPROPERTY(meta = (BindWidget))
    UBorder* questDetailPanel;

    // 퀘스트 이름 표시
    UPROPERTY(meta = (BindWidget))
    UTextBlock* detailQuestNameText;

    // 퀘스트 내용 표시
    UPROPERTY(meta = (BindWidget))
    UTextBlock* questContentText;

    // 퀘스트 조건 표시
    UPROPERTY(meta = (BindWidget))
    UTextBlock* questConditionsText;

    // ===================================
    // 회색 보상 영역 UI 요소들
    // ===================================

    // 보상 영역 배경 패널
    UPROPERTY(meta = (BindWidget))
    UBorder* rewardsBgPanel;

    // 보상 제목 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* rewardsTitleText;

    // 골드 보상 컨테이너
    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* goldRewardContainer;

    // 경험치 보상 컨테이너
    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* expRewardContainer;

    // 골드 아이콘
    UPROPERTY(meta = (BindWidget))
    UImage* goldIcon;

    // 경험치 아이콘
    UPROPERTY(meta = (BindWidget))
    UImage* expIcon;

    // 퀘스트 보상 - 돈
    UPROPERTY(meta = (BindWidget))
    UTextBlock* questMoneyRewardText;

    // 퀘스트 보상 - 경험치
    UPROPERTY(meta = (BindWidget))
    UTextBlock* questExpRewardText;

    // 아이템 보상 목록 컨테이너
    UPROPERTY(meta = (BindWidget))
    UVerticalBox* itemRewardsBox;

    // 닫기 버튼
    UPROPERTY(meta = (BindWidget))
    UButton* closeButton;

    // 퀘스트 항목 위젯 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TSubclassOf<class UQuestItemUI> questItemWidgetClass;

    // ===================================
    // 아이콘 및 스타일 설정
    // ===================================

    // 골드 아이콘 텍스처
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    UTexture2D* goldIconTexture;

    // 경험치 아이콘 텍스처
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    UTexture2D* expIconTexture;

    // 제목 텍스트 색상
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor titleTextColor = FLinearColor(0.5f, 0.8f, 0.2f, 1.0f);

    class APlayerController* playerController;                  // 플레이어 컨트롤러
    class UCharacterMovementComponent* playerMovementComponent; // 플레이어 이동관련 컴포넌트

    class UQuestItemUI* currentSelectedQuestItem = nullptr;     // 현재 선택된 퀘스트 아이템 추적
};