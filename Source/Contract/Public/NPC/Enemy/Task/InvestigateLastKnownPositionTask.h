// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "InvestigateLastKnownPositionTask.generated.h"

// === 조사 단계 열거형을 클래스 정의 전에 이동 ===
UENUM(BlueprintType)
enum class EInvestigationPhase : uint8
{
    MovingToLastKnownLocation   UMETA(DisplayName = "Moving to Last Known Location"),
    InvestigatingAtLocation     UMETA(DisplayName = "Investigating at Location"), 
    MovingToSearchPoint         UMETA(DisplayName = "Moving to Search Point"),
    SearchingAtPoint            UMETA(DisplayName = "Searching at Point"),
    Completed                   UMETA(DisplayName = "Investigation Completed")
};

/**
 * UInvestigateLastKnownPositionTask - 마지막 알려진 위치 조사 태스크
 * 타겟을 잃었을 때 마지막으로 알려진 위치로 이동하여 주변을 수색하는 태스크
 */
UCLASS()
class CONTRACT_API UInvestigateLastKnownPositionTask : public UBTTaskNode
{
    GENERATED_BODY()
    
public:
    // 생성자
    UInvestigateLastKnownPositionTask();

    // === 비헤이비어 트리 노드 오버라이드 ===
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
    // === 편집 가능한 속성들 ===
    
    // 조사할 마지막 위치를 지정하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector LastKnownLocationKey;
    
    // 현재 타겟 액터를 확인하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector TargetActorKey;
    
    // 경계 상태를 나타내는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector IsAlertKey;
    
    // 🔧 전투 상태를 나타내는 블랙보드 키 추가
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector isInCombatKey;
    
    // 도착 허용 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "50.0", ClampMax = "500.0"))
    float AcceptanceRadius = 100.0f;
    
    // 조사 지속 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "30.0"))
    float investigationDuration = 8.0f;
    
    // 추가 수색 포인트 개수 (마지막 위치 주변을 추가로 수색)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "8"))
    int32 additionalSearchPoints = 3;
    
    // 수색 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", ClampMax = "1000.0"))
    float searchRadius = 300.0f;
    
    // 각 수색 포인트에서의 대기 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "10.0"))
    float waitTimeAtSearchPoint = 2.0f;
    
    // 최대 실행 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (AllowPrivateAccess = "true", ClampMin = "5.0", ClampMax = "60.0"))
    float maxExecutionTime = 30.0f;
    
    // 이동 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", ClampMax = "800.0"))
    float movementSpeed = 300.0f;
    
    // 조사 중 타겟을 재발견했을 때 즉시 종료할지 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (AllowPrivateAccess = "true"))
    bool isStopOnTargetFound = true;
    
    // 조사 실패 시 경계 상태 해제 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (AllowPrivateAccess = "true"))
    bool isClearAlertOnFailure = true;
    
    // 🔧 전투 상태 해제 시간 추가
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (AllowPrivateAccess = "true", ClampMin = "5.0", ClampMax = "60.0"))
    float combatClearTime = 10.0f;

private:
    // === 내부 헬퍼 함수들 ===
    
    // 마지막 알려진 위치 가져오기
    bool GetLastKnownLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const;
    
    // 수색 포인트들 생성
    TArray<FVector> GenerateSearchPoints(const FVector& CenterLocation, UBehaviorTreeComponent& OwnerComp) const;
    
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

    // 🔧 누락된 단계별 처리 함수들 추가
    // 마지막 알려진 위치로 이동 처리
    void HandleMovingToLastKnownLocation(UBehaviorTreeComponent& OwnerComp, struct FInvestigateLastKnownPositionTaskMemory* TaskMemory, float CurrentTime);
    
    // 위치에서 조사 처리
    void HandleInvestigatingAtLocation(UBehaviorTreeComponent& OwnerComp, struct FInvestigateLastKnownPositionTaskMemory* TaskMemory, float CurrentTime, float PhaseElapsedTime);
    
    // 수색 포인트로 이동 처리
    void HandleMovingToSearchPoint(UBehaviorTreeComponent& OwnerComp, struct FInvestigateLastKnownPositionTaskMemory* TaskMemory, float CurrentTime);
    
    // 수색 포인트에서 수색 처리
    void HandleSearchingAtPoint(UBehaviorTreeComponent& OwnerComp, struct FInvestigateLastKnownPositionTaskMemory* TaskMemory, float CurrentTime, float PhaseElapsedTime);
};

// === 태스크 메모리 구조체 ===
struct FInvestigateLastKnownPositionTaskMemory
{
    // 시작 시간
    float StartTime = 0.0f;
    
    // 현재 단계 시작 시간
    float CurrentPhaseStartTime = 0.0f;
    
    // 현재 조사 단계
    EInvestigationPhase CurrentPhase = EInvestigationPhase::MovingToLastKnownLocation;
    
    // 마지막 알려진 위치
    FVector LastKnownLocation = FVector::ZeroVector;
    
    // 수색 포인트들
    TArray<FVector> SearchPoints;
    
    // 현재 수색 포인트 인덱스
    int32 CurrentSearchPointIndex = 0;
    
    // 현재 목표 위치
    FVector CurrentTargetLocation = FVector::ZeroVector;
    
    // 이동 중 여부
    bool bIsMoving = false;
    
    // 조사 완료 여부
    bool bInvestigationCompleted = false;
    
    // 타겟 재발견 여부
    bool bTargetRediscovered = false;
    
    // 🔧 전투 상태 해제 여부
    bool bCombatStateCleared = false;
    
    // 초기화 함수
    void Initialize()
    {
        StartTime = 0.0f;
        CurrentPhaseStartTime = 0.0f;
        CurrentPhase = EInvestigationPhase::MovingToLastKnownLocation;
        LastKnownLocation = FVector::ZeroVector;
        SearchPoints.Empty();
        CurrentSearchPointIndex = 0;
        CurrentTargetLocation = FVector::ZeroVector;
        bIsMoving = false;
        bInvestigationCompleted = false;
        bTargetRediscovered = false;
        bCombatStateCleared = false;  // 🔧 초기화 추가
    }
};
