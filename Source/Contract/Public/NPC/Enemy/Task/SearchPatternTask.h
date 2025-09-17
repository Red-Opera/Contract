// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SearchPatternTask.generated.h"

// === 검색 패턴 열거형을 클래스 정의 전에 이동 ===
UENUM(BlueprintType)
enum class ESearchPattern : uint8
{
    Random          UMETA(DisplayName = "Random Search"),
    Circular        UMETA(DisplayName = "Circular Pattern"),
    Grid            UMETA(DisplayName = "Grid Pattern"),
    Spiral          UMETA(DisplayName = "Spiral Pattern"),
    Linear          UMETA(DisplayName = "Linear Back and Forth"),
    Custom          UMETA(DisplayName = "Custom Points")
};

// === 검색 단계 열거형 ===
UENUM(BlueprintType)
enum class ESearchPhase : uint8
{
    MovingToSearchPoint     UMETA(DisplayName = "Moving to Search Point"),
    SearchingAtPoint        UMETA(DisplayName = "Searching at Point"),
    ReturningToStart        UMETA(DisplayName = "Returning to Start"),
    Completed               UMETA(DisplayName = "Search Completed")
};

/**
 * USearchPatternTask - 시스템용 패턴 검색 태스크
 * 타겟을 잃었을 때 다양한 패턴으로 넓은 영역을 체계적으로 수색하는 태스크
 */
UCLASS()
class CONTRACT_API USearchPatternTask : public UBTTaskNode
{
    GENERATED_BODY()
    
public:
    // 생성자
    USearchPatternTask();

    // === 비헤이비어 트리 노드 오버라이드 ===
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
    // === 편집 가능한 속성들 ===
    
    // 검색 중심점을 지정하는 블랙보드 키 (Enemy의 현재 위치 또는 마지막 알려진 위치)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector SearchCenterKey;
    
    // 현재 타겟 액터를 확인하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector TargetActorKey;
    
    // 경계 상태를 나타내는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector IsAlertKey;
    
    // 검색 패턴 유형
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search Pattern", meta = (AllowPrivateAccess = "true"))
    ESearchPattern SearchPattern = ESearchPattern::Circular;
    
    // 검색 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search Pattern", meta = (AllowPrivateAccess = "true", ClampMin = "200.0", ClampMax = "2000.0"))
    float SearchRadius = 800.0f;
    
    // 검색 포인트 개수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search Pattern", meta = (AllowPrivateAccess = "true", ClampMin = "3", ClampMax = "20"))
    int32 SearchPointCount = 8;
    
    // 각 검색 포인트에서의 대기 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search Pattern", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "10.0"))
    float WaitTimeAtPoint = 3.0f;
    
    // 도착 허용 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "50.0", ClampMax = "300.0"))
    float AcceptanceRadius = 100.0f;
    
    // 최대 검색 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (AllowPrivateAccess = "true", ClampMin = "10.0", ClampMax = "120.0"))
    float MaxSearchTime = 60.0f;
    
    // 이동 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", ClampMax = "600.0"))
    float MovementSpeed = 350.0f;
    
    // 그리드 패턴용 - 가로 세로 포인트 수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Pattern", meta = (AllowPrivateAccess = "true", ClampMin = "2", ClampMax = "6", EditCondition = "SearchPattern == ESearchPattern::Grid"))
    int32 GridSize = 3;
    
    // 나선형 패턴용 - 나선 회전 수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spiral Pattern", meta = (AllowPrivateAccess = "true", ClampMin = "1", ClampMax = "5", EditCondition = "SearchPattern == ESearchPattern::Spiral"))
    int32 SpiralTurns = 2;
    
    // 선형 패턴용 - 선의 개수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Linear Pattern", meta = (AllowPrivateAccess = "true", ClampMin = "2", ClampMax = "8", EditCondition = "SearchPattern == ESearchPattern::Linear"))
    int32 LinearLines = 3;
    
    // 커스텀 패턴용 - 상대적 검색 포인트들
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Pattern", meta = (AllowPrivateAccess = "true", EditCondition = "SearchPattern == ESearchPattern::Custom"))
    TArray<FVector> CustomSearchPoints;
    
    // 검색 중 타겟을 재발견했을 때 즉시 종료할지 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search", meta = (AllowPrivateAccess = "true"))
    bool bStopOnTargetFound = true;
    
    // 검색 실패 시 경계 상태 해제 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search", meta = (AllowPrivateAccess = "true"))
    bool bClearAlertOnFailure = true;
    
    // 검색 포인트들을 무작위로 섞을지 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search", meta = (AllowPrivateAccess = "true"))
    bool bRandomizeOrder = false;
    
    // 검색 완료 후 원래 위치로 돌아갈지 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search", meta = (AllowPrivateAccess = "true"))
    bool bReturnToStart = false;

private:
    // === 내부 헬퍼 함수들 ===
    
    // 검색 중심점 가져오기
    bool GetSearchCenter(UBehaviorTreeComponent& OwnerComp, FVector& OutCenter) const;
    
    // 패턴에 따른 검색 포인트들 생성
    TArray<FVector> GenerateSearchPoints(const FVector& CenterLocation, UBehaviorTreeComponent& OwnerComp) const;
    
    // 원형 패턴 검색 포인트 생성
    TArray<FVector> GenerateCircularPoints(const FVector& Center) const;
    
    // 그리드 패턴 검색 포인트 생성
    TArray<FVector> GenerateGridPoints(const FVector& Center) const;
    
    // 나선형 패턴 검색 포인트 생성
    TArray<FVector> GenerateSpiralPoints(const FVector& Center) const;
    
    // 선형 패턴 검색 포인트 생성
    TArray<FVector> GenerateLinearPoints(const FVector& Center) const;
    
    // 랜덤 패턴 검색 포인트 생성
    TArray<FVector> GenerateRandomPoints(const FVector& Center, UBehaviorTreeComponent& OwnerComp) const;
    
    // 커스텀 패턴 검색 포인트 생성
    TArray<FVector> GenerateCustomPoints(const FVector& Center) const;
    
    // 네비게이션 가능한 위치로 조정
    FVector FindNavigablePosition(const FVector& DesiredPosition, UBehaviorTreeComponent& OwnerComp) const;
    
    // 현재 타겟이 있는지 확인
    bool HasCurrentTarget(UBehaviorTreeComponent& OwnerComp) const;
    
    // AI를 특정 위치로 이동
    bool MoveToLocation(UBehaviorTreeComponent& OwnerComp, const FVector& TargetLocation) const;
    
    // 이동 완료 확인
    bool HasReachedDestination(UBehaviorTreeComponent& OwnerComp, const FVector& TargetLocation) const;
    
    // Enemy 참조 가져오기
    class AEnemy* GetControlledEnemy(UBehaviorTreeComponent& OwnerComp) const;
    
    // 시야각 내 타겟 스캔
    bool ScanForTargetInArea(UBehaviorTreeComponent& OwnerComp, const FVector& SearchLocation) const;

    // 🔧 누락된 단계별 처리 함수들 추가
    // 검색 포인트로 이동 처리
    void HandleMovingToSearchPoint(UBehaviorTreeComponent& OwnerComp, struct FSearchPatternTaskMemory* TaskMemory, float CurrentTime);
    
    // 검색 포인트에서 검색 처리
    void HandleSearchingAtPoint(UBehaviorTreeComponent& OwnerComp, struct FSearchPatternTaskMemory* TaskMemory, float CurrentTime, float PhaseElapsedTime);
    
    // 시작점으로 복귀 처리
    void HandleReturningToStart(UBehaviorTreeComponent& OwnerComp, struct FSearchPatternTaskMemory* TaskMemory, float CurrentTime);
};

// === 태스크 메모리 구조체 ===
struct FSearchPatternTaskMemory
{
    // 시작 시간
    float StartTime = 0.0f;
    
    // 현재 단계 시작 시간
    float CurrentPhaseStartTime = 0.0f;
    
    // 현재 검색 단계
    ESearchPhase CurrentPhase = ESearchPhase::MovingToSearchPoint;
    
    // 검색 중심점
    FVector SearchCenter = FVector::ZeroVector;
    
    // 시작 위치 (돌아갈 위치)
    FVector StartLocation = FVector::ZeroVector;
    
    // 검색 포인트들
    TArray<FVector> SearchPoints;
    
    // 현재 검색 포인트 인덱스
    int32 CurrentSearchIndex = 0;
    
    // 현재 목표 위치
    FVector CurrentTargetLocation = FVector::ZeroVector;
    
    // 이동 중 여부
    bool bIsMoving = false;
    
    // 검색 완료 여부
    bool bSearchCompleted = false;
    
    // 타겟 재발견 여부
    bool bTargetFound = false;
    
    // 총 검색한 포인트 수
    int32 PointsSearched = 0;
    
    // 초기화 함수
    void Initialize()
    {
        StartTime = 0.0f;
        CurrentPhaseStartTime = 0.0f;
        CurrentPhase = ESearchPhase::MovingToSearchPoint;
        SearchCenter = FVector::ZeroVector;
        StartLocation = FVector::ZeroVector;
        SearchPoints.Empty();
        CurrentSearchIndex = 0;
        CurrentTargetLocation = FVector::ZeroVector;
        bIsMoving = false;
        bSearchCompleted = false;
        bTargetFound = false;
        PointsSearched = 0;
    }
};
