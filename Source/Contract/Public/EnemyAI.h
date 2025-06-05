#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "OccupiedTerritory.h"
#include "Perception/AIPerceptionComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITypes.h"
#include "EnemyAI.generated.h"

/**
 * AEnemyAI - 적 캐릭터를 제어하는 AI 컨트롤러 클래스
 * 아군 영역을 찾아 이동하고 공격하는 AI 로직 구현
 */
UCLASS()
class CONTRACT_API AEnemyAI : public ADetourCrowdAIController
{
    GENERATED_BODY()
    
public:
    // 위젯 초기화
    AEnemyAI();
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    // 아군 영역 탐색 및 이동 기능
    UFUNCTION(BlueprintCallable, Category = "AI Movement")
    AOccupiedTerritory* FindNearestFriendlyTerritory();

    UFUNCTION(BlueprintCallable, Category = "AI Movement")
    void MoveToFriendlyTerritory();

    UFUNCTION(BlueprintCallable, Category = "AI Movement")
    void MoveToTargetLocation(FVector targetLocation);

    // 전투 관련 함수
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartAttack();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StopAttack();

    // 현재 공격 중인지 확인
    UFUNCTION(BlueprintPure, Category = "Combat")
    bool GetIsAttacking() const { return isAttacking; }

    // 현재 이동 중인지 확인
    UFUNCTION(BlueprintPure, Category = "AI Movement")
    bool GetIsMovingToTerritory() const { return isMovingToTerritory; }

    // 인식 시스템 이벤트 핸들러
    UFUNCTION()
    void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    // AI 인식 시스템
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent* AIPerceptionComp;

    // AI 행동 결정 시스템
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    UBehaviorTree* BehaviorTree;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    UBlackboardData* blackboardData;

    // AI 이동 파라미터
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    float searchRadius = 2000.0f;                           // 영역 검색 반경

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    float acceptanceRadius = 100.0f;                        // 목적지 도달 허용 거리

protected:
    // 기본 라이프사이클 함수
    virtual void BeginPlay() override;
    virtual void Tick(float deltaTime) override;
    
    // 이동 완료 시 호출
    virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

    // 스페이스 키 입력 처리에 해당하는 AI 상태 업데이트 함수들
    void UpdateTerritorySearchState(float DeltaTime);       // 영역 탐색 로직 업데이트
    void UpdateCombatState(float DeltaTime);                // 전투 로직 업데이트
    void UpdateMovementDirection(float DeltaTime);          // 이동 방향 업데이트

    // 현재 제어 중인 폰
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    APawn* controlledPawn;

    // 현재 목표 영역
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    AOccupiedTerritory* currentTargetTerritory;

    // 전투 관련 변수
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool isAttacking = false;                               // 현재 공격 중인지 여부

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float timeSinceLastAttackDecision = 0.0f;               // 마지막 공격 결정 이후 경과 시간

    // 이동 상태 변수
    UPROPERTY(BlueprintReadOnly, Category = "AI Movement")
    bool isMovingToTerritory = false;                       // 영역으로 이동 중인지 여부

    UPROPERTY(BlueprintReadOnly, Category = "AI Movement")
    float timeSinceLastTerritorySearch = 0.0f;              // 마지막 영역 검색 이후 경과 시간

    // ===================================
    // AI 설정 매개변수들
    // ===================================

    // 전투 관련 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Combat")
    float attackDecisionUpdateInterval = 1.0f;              // 공격 결정 업데이트 간격

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Combat")
    float attackRange = 500.0f;                            // 공격 범위

    // 이동 관련 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Movement")
    float territorySearchInterval = 5.0f;                   // 영역 재검색 간격

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Movement")
    float rotationInterpSpeed = 5.0f;                       // 회전 보간 속도

    // 이동 방향 계산 변수 (AllyNPCAI와 동일하게 추가)
    FVector lastPosition;                          // 이전 위치
    FVector currentMovementDirection;              // 현재 이동 방향
    float movementDirectionUpdateInterval = 0.005f; // 방향 업데이트 간격 (0.01f -> 0.005f로 더 자주 업데이트)
    float timeSinceLastDirectionUpdate;            // 마지막 방향 업데이트 이후 경과 시간

private:
    // 전투 유틸리티 함수
    bool IsInAttackRange() const;                           // 공격 가능 거리 확인

    // 내비게이션 헬퍼 함수
    FVector FindNearestNavigableLocation(FVector targetLocation);
    bool IsLocationNavigable(FVector location);
};
