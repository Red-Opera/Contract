#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "StrafeAroundTargetTask.generated.h"

// 플레이어가 가까이 올 때 후퇴하는 비헤이비어 트리 태스크
UCLASS()
class CONTRACT_API UStrafeAroundTargetTask : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UStrafeAroundTargetTask();

    // 비헤이비어 트리 노드 오버라이드
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
    // 편집 가능한 속성들
    
    // 후퇴할 타겟을 지정하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector targetActorKey;
    
    // 자동으로 플레이어를 타겟으로 설정할지 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    bool bAutoFindPlayer = true;
    
    // 우선순위가 높은 블랙보드 키들 (순서대로 검색)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    TArray<FString> priorityTargetKeys = {"PlayerCharacter", "Player", "TargetActor", "Target", "PlayerPawn"};
    
    // FireDistance를 확인하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector fireDistanceKey;
    
    // FireDistance 사용 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions", meta = (AllowPrivateAccess = "true"))
    bool isUseFireDistanceCondition = false;
    
    // FireDistance 실행 임계값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", ClampMax = "1500.0"))
    float fireDistanceThreshold = 800.0f;
    
    // 후퇴 시작 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retreat", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", ClampMax = "1000.0"))
    float retreatStartDistance = 700.0f;
    
    // 후퇴 목표 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retreat", meta = (AllowPrivateAccess = "true", ClampMin = "200.0", ClampMax = "1500.0"))
    float retreatTargetDistance = 900.0f;
    
    // 후퇴 완료 거리 (목표 거리보다 더 멀리)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retreat", meta = (AllowPrivateAccess = "true", ClampMin = "300.0", ClampMax = "2000.0"))
    float retreatCompleteDistance = 1100.0f;
    
    // 최소 후퇴 이동 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retreat", meta = (AllowPrivateAccess = "true", ClampMin = "200.0", ClampMax = "800.0"))
    float minimumRetreatMoveDistance = 300.0f;
    
    // 후퇴 상태 유지 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retreat", meta = (AllowPrivateAccess = "true", ClampMin = "2.0", ClampMax = "10.0"))
    float retreatStateDuration = 5.0f;
    
    // 후퇴 이동 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "200.0", ClampMax = "1000.0"))
    float retreatSpeed = 600.0f;
    
    // 목표 위치 허용 오차 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "10.0", ClampMax = "200.0"))
    float distanceTolerance = 80.0f;
    
    // 최대 실행 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (AllowPrivateAccess = "true", ClampMin = "5.0", ClampMax = "30.0"))
    float maxExecutionTime = 15.0f;

private:
    // 헬퍼 함수들
    bool CheckFireDistanceCondition(UBehaviorTreeComponent& ownerComp) const;                                                   // FireDistance 조건 확인
    bool GetTargetInfo(UBehaviorTreeComponent& ownerComp, FVector& outTargetLocation, float& outCurrentDistance) const;         // 타겟 정보 계산
    AActor* FindValidTarget(UBehaviorTreeComponent& ownerComp) const;                                                        // 유효한 타겟 찾기
    bool SetupTargetInBlackboard(UBehaviorTreeComponent& ownerComp, AActor* targetActor) const;                            // 블랙보드에 타겟 설정

    FVector CalculateRetreatPosition(const FVector& currentLocation, const FVector& targetLocation, float currentDistance) const; // 후퇴 위치 계산
    FVector FindNavigablePosition(const FVector& desiredPosition, UBehaviorTreeComponent& ownerComp) const;                     // 네비게이션 가능한 위치 찾기

    bool ShouldRetreat(float currentDistance) const;                               // 후퇴가 필요한지 확인 (개선됨)
    bool IsRetreatComplete(float currentDistance) const;                          // 후퇴가 완료되었는지 확인
};

// 태스크 메모리 구조체
struct FStrafeAroundTargetTaskMemory
{
    FVector currentTargetPosition = FVector::ZeroVector;    // 현재 목표 위치
    FVector previousTargetPosition = FVector::ZeroVector;   // 이전 목표 위치
    FAIRequestID moveRequestID;                             // 이동 요청 ID
    int32 consecutiveFailCount = 0;                         // 연속 이동 실패 카운트

    float startTime = 0.0f;                 // 시작 시간
    float lastMoveTime = 0.0f;              // 마지막 이동 시간
    float lastStatusCheckTime = 0.0f;       // 마지막 이동 상태 확인 시간
    bool isMoving = false;                  // 이동 상태
    bool needsForceRestart = false;         // 강제 이동 재시작이 필요한지 여부
    float lastSuccessfulMoveTime = 0.0f;    // 마지막으로 성공한 이동 시간

    bool isInRetreatState = false;          // 후퇴 상태 여부
    float retreatStateStartTime = 0.0f;     // 후퇴 상태 시작 시간
    float lastRetreatDistance = 0.0f;       // 마지막 후퇴 거리 기록
    
    // 초기화 함수
    void Initialize()
    {
        startTime = 0.0f;
        lastMoveTime = 0.0f;
        lastStatusCheckTime = 0.0f;

        currentTargetPosition = FVector::ZeroVector;
        previousTargetPosition = FVector::ZeroVector;
        moveRequestID = FAIRequestID::InvalidRequest;

        isMoving = false;
        needsForceRestart = false;
        lastSuccessfulMoveTime = 0.0f;
        consecutiveFailCount = 0;

        isInRetreatState = false;
        retreatStateStartTime = 0.0f;
        lastRetreatDistance = 0.0f;
    }
};
