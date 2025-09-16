// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "StrafeAroundTargetTask.generated.h"

// === 🔧 스트레이프 패턴 열거형을 클래스 정의 전에 이동===
UENUM(BlueprintType)
enum class EStrafePattern : uint8
{
    Random          UMETA(DisplayName = "Random Direction"),
    Clockwise       UMETA(DisplayName = "Clockwise"),
    CounterClockwise UMETA(DisplayName = "Counter-Clockwise"),
    Adaptive        UMETA(DisplayName = "Adaptive (Avoid LOS)")
};

/**
 * UStrafeAroundTargetTask - 시스템용 타겟 주변을 스트레이프하는 비헤이비어 트리 태스크
 * 타겟을 중심으로 원형으로 이동하며 최적의 전투 거리를 유지하는 태스크
 */
UCLASS()
class CONTRACT_API UStrafeAroundTargetTask : public UBTTaskNode
{
    GENERATED_BODY()

public:
    // 생성자
    UStrafeAroundTargetTask();

    // === 비헤이비어 트리 노드 오버라이드 ===
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
    // === 편집 가능한 속성들 ===
    
    // 스트레이프할 타겟을 지정하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector targetActorKey;
    
    // 타겟이 없을 때 스트레이프할 위치를 지정하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector lastKnownLocationKey;
    
    // 타겟과의 최적 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", ClampMax = "2000.0"))
    float optimalDistance = 800.0f;
    
    // 거리 허용 오차 (이 범위 내면 거리 조정하지 않음)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "50.0", ClampMax = "500.0"))
    float distanceTolerance = 100.0f;
    
    // 스트레이프 속도 (도/초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "10.0", ClampMax = "180.0"))
    float strafeSpeed = 45.0f;
    
    // 스트레이프 방향 변경 간격 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "10.0"))
    float directionChangeInterval = 3.0f;
    
    // 최대 실행 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "30.0"))
    float maxExecutionTime = 10.0f;
    
    // 이동 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", ClampMax = "1000.0"))
    float movementSpeed = 400.0f;
    
    // 🔧 스트레이프 패턴 - enum class 사용
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    EStrafePattern strafePattern = EStrafePattern::Adaptive;
    
    // 타겟 시야각 회피 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    bool isAvoidTargetLOS = true;
    
    // 장애물 회피 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation", meta = (AllowPrivateAccess = "true", ClampMin = "50.0", ClampMax = "300.0"))
    float obstacleAvoidanceRadius = 150.0f;

private:
    // === 내부 헬퍼 함수들 ===
    
    // 타겟 위치 및 거리 계산
    bool CalculateTargetInfo(UBehaviorTreeComponent& OwnerComp, FVector& OutTargetLocation, float& OutCurrentDistance) const;
    
    // 스트레이프 목표 위치 계산
    FVector CalculateStrafePosition(const FVector& CurrentLocation, const FVector& TargetLocation, float CurrentDistance, bool bClockwise) const;
    
    // 거리 조정 위치 계산
    FVector CalculateDistanceAdjustmentPosition(const FVector& CurrentLocation, const FVector& TargetLocation, float CurrentDistance) const;
    
    // 네비게이션 가능한 위치 찾기
    FVector FindNavigablePosition(const FVector& DesiredPosition, UBehaviorTreeComponent& OwnerComp) const;
    
    // 장애물 체크
    bool IsPathClear(const FVector& Start, const FVector& End, UBehaviorTreeComponent& OwnerComp) const;
    
    // 타겟의 시야각 내에 있는지 확인
    bool IsInTargetLOS(const FVector& Position, const FVector& TargetLocation, const FVector& TargetForward) const;
};

// === 태스크 메모리 구조체 ===
struct FStrafeAroundTargetTaskMemory
{
    // 시작 시간
    float startTime = 0.0f;
    
    // 현재 스트레이프 방향 (시계방향: true, 반시계방향: false)
    bool isCurrentlyClockwise = true;
    
    // 마지막 방향 변경 시간
    float lastDirectionChangeTime = 0.0f;
    
    // 현재 목표 위치
    FVector currentTargetPosition = FVector::ZeroVector;
    
    // 이동 요청 ID
    FAIRequestID moveRequestID;
    
    // 타겟 위치
    FVector targetLocation = FVector::ZeroVector;
    
    // 마지막 타겟 거리
    float lastTargetDistance = 0.0f;
    
    // 이동 상태
    bool isMoving = false;
    
    // 거리 조정 모드
    bool isDistanceAdjustmentMode = false;
    
    // 초기화 함수
    void Initialize()
    {
        startTime = 0.0f;
        isCurrentlyClockwise = FMath::RandBool();
        lastDirectionChangeTime = 0.0f;
        currentTargetPosition = FVector::ZeroVector;
        moveRequestID = FAIRequestID::InvalidRequest;
        targetLocation = FVector::ZeroVector;
        lastTargetDistance = 0.0f;
        isMoving = false;
        isDistanceAdjustmentMode = false;
    }
};
