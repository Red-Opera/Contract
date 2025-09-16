// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "MoveToFriendlyTerritoryTask.generated.h"

// === 우호 지역 유형 열거형을 클래스 정의 전에 이동 ===
UENUM(BlueprintType)
enum class EFriendlyTerritoryType : uint8
{
    SpawnPoint          UMETA(DisplayName = "Spawn Point"),
    PatrolPoint         UMETA(DisplayName = "Patrol Point"),
    CoverPoint          UMETA(DisplayName = "Cover Point"),
    SafeZone            UMETA(DisplayName = "Safe Zone"),
    NearestAlly         UMETA(DisplayName = "Nearest Ally"),
    CustomLocation      UMETA(DisplayName = "Custom Location")
};

// === 이동 단계 열거형 ===
UENUM(BlueprintType)
enum class EMoveToTerritoryPhase : uint8
{
    FindingTerritory        UMETA(DisplayName = "Finding Territory"),
    MovingToTerritory       UMETA(DisplayName = "Moving to Territory"),
    WaitingAtTerritory      UMETA(DisplayName = "Waiting at Territory"),
    Completed               UMETA(DisplayName = "Movement Completed")
};

/**
 * UMoveToFriendlyTerritoryTask - 시스템용 우호 지역 이동 태스크
 * 전투 중 불리한 상황이거나 후퇴가 필요할 때 안전한 우호 지역으로 이동하는 태스크
 */
UCLASS()
class CONTRACT_API UMoveToFriendlyTerritoryTask : public UBTTaskNode
{
    GENERATED_BODY()
    
public:
    // 생성자
    UMoveToFriendlyTerritoryTask();

    // === 비헤이비어 트리 노드 오버라이드 ===
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
    // === 편집 가능한 속성들 ===
    
    // 우호 지역 유형
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory", meta = (AllowPrivateAccess = "true"))
    EFriendlyTerritoryType territoryType = EFriendlyTerritoryType::SpawnPoint;
    
    // === 🔧 새로 추가된 전투 상태 감지 옵션 ===
    // 전투 상태 감지 시 태스크 중단 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    bool shouldAbortOnCombat = true;
    
    // 커스텀 위치를 지정하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory", meta = (AllowPrivateAccess = "true", EditCondition = "TerritoryType == EFriendlyTerritoryType::CustomLocation"))
    struct FBlackboardKeySelector customLocationKey;
    
    // 현재 타겟 액터를 확인하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector targetActorKey;
    
    // 경계 상태를 나타내는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector isAlertKey;
    
    // 도착 허용 반경
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "50.0", ClampMax = "500.0"))
    float acceptanceRadius = 150.0f;
    
    // 최대 검색 반경 (우호 지역을 찾는 범위)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory", meta = (AllowPrivateAccess = "true", ClampMin = "500.0", ClampMax = "5000.0"))
    float maxSearchRadius = 2000.0f;
    
    // 최소 안전 거리 (타겟으로부터)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety", meta = (AllowPrivateAccess = "true", ClampMin = "200.0", ClampMax = "2000.0"))
    float minSafeDistance = 800.0f;
    
    // 최대 이동 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (AllowPrivateAccess = "true", ClampMin = "10.0", ClampMax = "120.0"))
    float maxMoveTime = 45.0f;
    
    // 이동 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", ClampMin = "200.0", ClampMax = "800.0"))
    float movementSpeed = 450.0f; // 빠른 후퇴
    
    // 우호 지역 도착 후 대기 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory", meta = (AllowPrivateAccess = "true", ClampMin = "2.0", ClampMax = "30.0"))
    float waitTimeAtTerritory = 5.0f;
    
    // 이동 중 적과의 교전 허용 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    bool isAllowCombatWhileMoving = false;
    
    // 우호 지역 태그 (Actor Tag로 우호 지역 식별)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory", meta = (AllowPrivateAccess = "true"))
    FName friendlyTerritoryTag = TEXT("FriendlyTerritory");
    
    // 안전 지역 태그
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory", meta = (AllowPrivateAccess = "true"))
    FName safeZoneTag = TEXT("SafeZone");
    
    // 엄폐물 태그
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory", meta = (AllowPrivateAccess = "true"))
    FName coverTag = TEXT("Cover");
    
    // 스폰 포인트 태그
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory", meta = (AllowPrivateAccess = "true"))
    FName spawnPointTag = TEXT("SpawnPoint");
    
    // 패트롤 포인트 태그
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory", meta = (AllowPrivateAccess = "true"))
    FName patrolPointTag = TEXT("PatrolPoint");
    
    // 후퇴 완료 후 경계 상태 해제 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State", meta = (AllowPrivateAccess = "true"))
    bool isClearAlertOnArrival = false;
    
    // 여러 후보 지역 중 가장 가까운 곳 선택 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory", meta = (AllowPrivateAccess = "true"))
    bool isChooseClosest = true;
    
    // 타겟으로부터 가장 먼 지역 선택 여부 (안전성 우선)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety", meta = (AllowPrivateAccess = "true"))
    bool isChooseFarthestFromTarget = false;

private:
    // === 내부 헬퍼 함수들 ===
    
    // 목표 영역 찾기 (메인 함수)
    bool FindTargetTerritory(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const;
    
    // 우호적인 OccupiedTerritory 찾기 (새로 추가)
    bool FindFriendlyOccupiedTerritory(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const;
    
    // 각 영역 타입별 검색 함수들
    bool FindSpawnPoint(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const;
    bool FindPatrolPoint(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const;
    bool FindCoverPoint(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const;
    bool FindSafeZone(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const;
    bool FindNearestAlly(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const;
    bool GetCustomLocation(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const;
    
    // 태그로 액터들 찾기
    TArray<AActor*> FindActorsWithTag(UBehaviorTreeComponent& OwnerComp, const FName& Tag) const;
    
    // 최적의 지역 선택
    FVector SelectBestTerritory(const TArray<FVector>& Candidates, UBehaviorTreeComponent& OwnerComp) const;
    
    // 위치가 안전한지 확인
    bool IsLocationSafe(const FVector& Location, UBehaviorTreeComponent& OwnerComp) const;
    
    // 네비게이션 가능한 위치로 조정
    FVector FindNavigablePosition(const FVector& DesiredPosition, UBehaviorTreeComponent& OwnerComp) const;
    
    // 현재 타겟 가져오기
    AActor* GetCurrentTarget(UBehaviorTreeComponent& OwnerComp) const;
    
    // AI를 특정 위치로 이동
    bool MoveToLocation(UBehaviorTreeComponent& OwnerComp, const FVector& TargetLocation) const;
    
    // 이동 완료 확인
    bool HasReachedDestination(UBehaviorTreeComponent& OwnerComp, const FVector& TargetLocation) const;
    
    // Enemy 참조 가져오기
    class AEnemy* GetControlledEnemy(UBehaviorTreeComponent& OwnerComp) const;
    
    // 이동 중 전투 처리
    void HandleCombatWhileMoving(UBehaviorTreeComponent& OwnerComp, struct FMoveToFriendlyTerritoryTaskMemory* TaskMemory) const;

    // 🔧 누락된 단계별 처리 함수들 추가
    // 우호 지역 찾기 처리
    void HandleFindingTerritory(UBehaviorTreeComponent& OwnerComp, struct FMoveToFriendlyTerritoryTaskMemory* TaskMemory, float CurrentTime);
    
    // 우호 지역으로 이동 처리
    void HandleMovingToTerritory(UBehaviorTreeComponent& OwnerComp, struct FMoveToFriendlyTerritoryTaskMemory* TaskMemory, float CurrentTime);
    
    // 우호 지역에서 대기 처리
    void HandleWaitingAtTerritory(UBehaviorTreeComponent& OwnerComp, struct FMoveToFriendlyTerritoryTaskMemory* TaskMemory, float CurrentTime, float PhaseElapsedTime);
};

// === 태스크 메모리 구조체 ===
struct FMoveToFriendlyTerritoryTaskMemory
{
    // 시작 시간
    float startTime = 0.0f;
    
    // 현재 단계 시작 시간
    float currentPhaseStartTime = 0.0f;
    
    // 현재 이동 단계
    EMoveToTerritoryPhase currentPhase = EMoveToTerritoryPhase::FindingTerritory;
    
    // 목표 우호 지역 위치
    FVector targetTerritoryLocation = FVector::ZeroVector;
    
    // 시작 위치
    FVector startLocation = FVector::ZeroVector;
    
    // 이동 중 여부
    bool isMoving = false;
    
    // 우호 지역 도착 여부
    bool isArrivedAtTerritory = false;
    
    // 이동 완료 여부
    bool isMovementCompleted = false;
    
    // 우호 지역을 찾았는지 여부
    bool isTerritoryFound = false;
    
    // 이동 중 전투 발생 여부
    bool isCombatOccurred = false;
    
    // 초기화 함수
    void Initialize()
    {
        startTime = 0.0f;
        currentPhaseStartTime = 0.0f;
        currentPhase = EMoveToTerritoryPhase::FindingTerritory;
        targetTerritoryLocation = FVector::ZeroVector;
        startLocation = FVector::ZeroVector;
        isMoving = false;
        isArrivedAtTerritory = false;
        isMovementCompleted = false;
        isTerritoryFound = false;
        isCombatOccurred = false;
    }
};
