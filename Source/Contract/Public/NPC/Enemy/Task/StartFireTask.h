// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "StartFireTask.generated.h"

/**
 * UStartFireTask - 시스템용 발사 시작 비헤이비어 트리 태스크
 * Enemy가 타겟을 향해 발사를 시작하도록 하는 태스크
 */
UCLASS()
class CONTRACT_API UStartFireTask : public UBTTaskNode
{
    GENERATED_BODY()
    
public:
    // 생성자
    UStartFireTask();

    // === 비헤이비어 트리 노드 오버라이드 ===
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
    // === 편집 가능한 속성들 ===
    
    // 발사할 타겟을 지정하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector TargetActorKey;
    
    // 타겟과의 거리를 확인하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector TargetDistanceKey;
    
    // 최대 발사 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", ClampMax = "3000.0"))
    float MaxFiringRange = 1500.0f;
    
    // 최소 발사 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true", ClampMin = "50.0", ClampMax = "1000.0"))
    float MinFiringRange = 200.0f;
    
    // 발사 지속 시간 (초) - 0이면 무제한
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "30.0"))
    float FiringDuration = 5.0f;
    
    // 발사 전 조준 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "3.0"))
    float AimingTime = 0.5f;
    
    // 타겟을 잃었을 때 발사 중단할지 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    bool bStopFireOnTargetLoss = true;
    
    // 탄약이 없을 때 자동 재장전할지 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    bool bAutoReloadOnEmpty = true;
    
    // 버스트 파이어 모드 사용 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    bool bUseBurstFire = false;
    
    // 발사 정확도 체크 여부 (타겟을 정확히 조준하고 있는지)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    bool bCheckAimAccuracy = true;
    
    // 조준 허용 각도 (도)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "45.0"))
    float AimAccuracyAngle = 10.0f;

private:
    // === 내부 헬퍼 함수들 ===
    
    // 타겟 정보 가져오기
    bool GetTargetInfo(UBehaviorTreeComponent& OwnerComp, AActor*& OutTargetActor, float& OutDistance) const;
    
    // 발사 가능 여부 확인
    bool CanFire(UBehaviorTreeComponent& OwnerComp, AActor* TargetActor, float Distance) const;
    
    // 조준 정확도 확인
    bool IsAimingAccurate(UBehaviorTreeComponent& OwnerComp, AActor* TargetActor) const;
    
    // Enemy 참조 가져오기
    class AEnemy* GetControlledEnemy(UBehaviorTreeComponent& OwnerComp) const;
    
    // 탄약 상태 확인
    bool HasAmmo(UBehaviorTreeComponent& OwnerComp) const;
    
    // 시야 확인 (타겟이 보이는지)
    bool HasLineOfSight(UBehaviorTreeComponent& OwnerComp, AActor* TargetActor) const;

    // 🔧 누락된 함수들 추가
    // 발사 시작
    void StartFiring(UBehaviorTreeComponent& OwnerComp, class AEnemy* ControlledEnemy, struct FStartFireTaskMemory* TaskMemory);
    
    // 발사 중단
    void StopFiring(UBehaviorTreeComponent& OwnerComp, class AEnemy* ControlledEnemy, struct FStartFireTaskMemory* TaskMemory);
};

// === 태스크 메모리 구조체 ===
struct FStartFireTaskMemory
{
    // 시작 시간
    float StartTime = 0.0f;
    
    // 조준 시작 시간
    float AimingStartTime = 0.0f;
    
    // 발사 시작 시간
    float FiringStartTime = 0.0f;
    
    // 현재 타겟
    TWeakObjectPtr<AActor> CurrentTarget = nullptr;
    
    // 발사 상태
    bool bIsFiring = false;
    
    // 조준 완료 여부
    bool bAimingCompleted = false;
    
    // 재장전 중 여부
    bool bIsReloading = false;
    
    // 마지막 탄약 확인 시간
    float LastAmmoCheckTime = 0.0f;
    
    // 초기화 함수
    void Initialize()
    {
        StartTime = 0.0f;
        AimingStartTime = 0.0f;
        FiringStartTime = 0.0f;
        CurrentTarget = nullptr;
        bIsFiring = false;
        bAimingCompleted = false;
        bIsReloading = false;
        LastAmmoCheckTime = 0.0f;
    }
};
