#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "OccupiedTerritory.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITypes.h"
#include "EnemyAI.generated.h"

// 전방 선언
class AEnemy;
class AOccupiedTerritory;
class UAIPerceptionComponent;

/**
 * AEnemyAI - TPS Kit GASP 시스템을 지원하는 적 캐릭터 AI 컨트롤러
 * Combat, Alert, Patrol 상태를 관리하며 비헤이비어 트리와 연동
 */
UCLASS()
class CONTRACT_API AEnemyAI : public ADetourCrowdAIController
{
    GENERATED_BODY()
    
public:
    // 생성자 및 기본 함수
    AEnemyAI();
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    // === TPS Kit GASP 시스템 - 상태 관리 ===
    UFUNCTION(BlueprintCallable, Category = "GASP System")
    void EnterCombatState(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "GASP System")
    void EnterAlertState(const FVector& LastKnownLocation);

    UFUNCTION(BlueprintCallable, Category = "GASP System")
    void EnterPatrolState();

    UFUNCTION(BlueprintCallable, Category = "GASP System")
    void ClearAllStates();

    // === 아군 영역 탐색 기능 (블루프린트 전용) ===
    UFUNCTION(BlueprintCallable, Category = "AI Movement")
    AOccupiedTerritory* FindNearestFriendlyTerritory();

    // === 전투 관련 함수 ===
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartAttack();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StopAttack();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartBurstFire();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StopBurstFire();

    // === 상태 확인 함수 ===
    UFUNCTION(BlueprintPure, Category = "GASP System")
    bool GetInCombat() const;

    UFUNCTION(BlueprintPure, Category = "GASP System")
    bool GetAlert() const;

    UFUNCTION(BlueprintPure, Category = "GASP System")
    bool GetPatrolling() const;

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool GetIsAttacking() const { return isAttacking; }

    // === 🔧 이동 관련 함수 제거 ===
    // bool GetIsMovingToTerritory() const - 제거

    // === 🔧 비헤이비어 트리 상태 확인 함수 추가 ===
    UFUNCTION(BlueprintPure, Category = "AI System")
    bool HasBehaviorTree() const { return BehaviorTree != nullptr && GetBlackboardComponent() != nullptr; }

    UFUNCTION(BlueprintPure, Category = "AI System")
    bool IsAISystemActive() const { return HasBehaviorTree(); }

    // === 인식 시스템 이벤트 핸들러 ===
    UFUNCTION()
    void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    // === AI 컴포넌트들 ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent* AIPerceptionComp;

    // === 비헤이비어 트리 시스템 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    UBehaviorTree* BehaviorTree;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    UBlackboardData* blackboardData;

    // === AI 이동 파라미터 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Movement")
    float searchRadius = 2000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Movement")
    float acceptanceRadius = 100.0f;

    // === TPS Kit GASP 시스템 - 전투 설정 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASP Combat")
    float combatEngagementRange = 1200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASP Combat")
    float combatDisengagementRange = 1500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASP Combat")
    float optimalFiringDistance = 800.0f;

    // === Alert 시스템 설정 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASP Alert")
    float alertDuration = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASP Alert")
    float alertSearchRadius = 500.0f;

    // === 인식 시스템 설정 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Perception")
    float sightRadius = 1500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Perception")
    float loseSightRadius = 2000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Perception")
    float peripheralVisionAngle = 90.0f;

protected:
    // === 기본 라이프사이클 함수 ===
    virtual void BeginPlay() override;
    virtual void Tick(float deltaTime) override;
    
    // === TPS Kit GASP 시스템 - 업데이트 함수들 (비활성화됨) ===
    // 🔧 이제 이 함수들은 비헤이비어 트리에서만 제어됨
    void UpdateGASPSystem(float DeltaTime);  // 비활성화됨
    void UpdateCombatBehavior(float DeltaTime);  // 비활성화됨
    void UpdateAlertBehavior(float DeltaTime);  // 비활성화됨
    void UpdatePatrolBehavior(float DeltaTime);  // 비활성화됨

    // === 블랙보드 동기화 ===
    UFUNCTION()
    void SyncEnemyStateWithBlackboard();
    
    UFUNCTION()
    void UpdateTargetDistance();

    UFUNCTION()
    void UpdateBlackboardKeys();

    // === 🔧 전투 해제 조건 확인 함수 추가 ===
    void CheckCombatDisengagementConditions(UBlackboardComponent* blackboardComp);

    // === 현재 제어 중인 액터들 ===
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    APawn* controlledPawn;

    UPROPERTY(BlueprintReadOnly, Category = "AI")
    AEnemy* controlledEnemy;

    UPROPERTY(BlueprintReadOnly, Category = "AI")
    AOccupiedTerritory* currentTargetTerritory;

    // === 🔧 상태 변수들 (읽기 전용으로 변경 - 비헤이비어 트리에서만 제어) ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP System")
    bool isInCombat = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP System")
    bool isAlert = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP System")
    bool isPatrolling = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP System")
    bool isBurstFiring = false;

    // === 타겟 추적 변수들 (감지용으로만 사용) ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Tracking")
    AActor* currentTarget = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Tracking")
    FVector lastKnownTargetLocation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Tracking")
    float lastTargetSeenTime = 0.0f;

    // === 전투 관련 변수 ===
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool isAttacking = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float timeSinceLastAttackDecision = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float timeSinceLastBurstFire = 0.0f;

    // === 🔧 AI 설정 매개변수들에서 사용하지 않는 변수 제거 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Settings")
    float attackDecisionUpdateInterval = 1.0f;

    // attackRange, territorySearchInterval, rotationInterpSpeed, burstFireCooldown 제거

    // === 타이머 변수들 ===
    float alertTimer = 0.0f;
    float combatTimer = 0.0f;

private:
    // === 유틸리티 함수들 ===
    bool IsInAttackRange() const;
    bool CanSeeTarget(AActor* Target) const;

    // === 디버그 정보 ===
    void DisplayDebugInfo();

    // === 블랙보드 키 이름들 (문자열 최적화) ===
    static const FName BB_IsInCombat;
    static const FName BB_IsAlert;
    static const FName BB_IsBurstFiring;
    static const FName BB_TargetActor;
    static const FName BB_LastKnownPlayerLocation;
    static const FName BB_FireDistance;
    static const FName BB_SelfActor;
};
