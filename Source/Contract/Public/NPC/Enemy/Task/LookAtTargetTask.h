// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "LookAtTargetTask.generated.h"

/**
 * ULookAtTargetTask - 타겟을 바라보는 비헤이비어 트리 태스크
 * 지정된 타겟이나 마지막으로 알려진 위치를 부드럽게 바라보도록 하는 태스크
 */
UCLASS()
class CONTRACT_API ULookAtTargetTask : public UBTTaskNode
{
    GENERATED_BODY()
    
public:
    // 생성자
    ULookAtTargetTask();

    // === 비헤이비어 트리 노드 오버라이드 ===
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override;

protected:
    // === 편집 가능한 속성들 ===
    
    // 바라볼 타겟 액터를 지정하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector TargetActorKey;
    
    // 타겟이 없을 때 바라볼 위치를 지정하는 블랙보드 키
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (AllowPrivateAccess = "true"))
    struct FBlackboardKeySelector LastKnownLocationKey;
    
    // 회전 속도 (도/초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "1000.0"))
    float RotationSpeed = 180.0f;
    
    // 타겟을 향한 회전 허용 각도 (이 각도 이내면 완료로 간주)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "90.0"))
    float AcceptableAngle = 5.0f;
    
    // 최대 실행 시간 (초) - 이 시간이 지나면 성공으로 간주
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "10.0"))
    float MaxExecutionTime = 3.0f;
    
    // Z축 회전만 사용할지 여부 (일반적으로 캐릭터는 Z축만 회전)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (AllowPrivateAccess = "true"))
    bool bOnlyYawRotation = true;
    
    // 즉시 회전할지, 부드럽게 회전할지 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (AllowPrivateAccess = "true"))
    bool bInstantRotation = false;

private:
    // === 내부 헬퍼 함수들 ===
    
    // 타겟 위치 계산
    FVector CalculateTargetLocation(UBehaviorTreeComponent& OwnerComp) const;
    
    // 목표 회전값 계산
    FRotator CalculateTargetRotation(const FVector& CurrentLocation, const FVector& TargetLocation) const;
    
    // 회전 완료 여부 확인
    bool IsRotationComplete(const FRotator& CurrentRotation, const FRotator& TargetRotation) const;
    
    // 안전한 회전 보간
    FRotator SafeRotationInterp(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed) const;
};

// === 태스크 메모리 구조체 ===
struct FLookAtTargetTaskMemory
{
    // 시작 시간
    float StartTime = 0.0f;
    
    // 목표 회전값
    FRotator TargetRotation = FRotator::ZeroRotator;
    
    // 시작 회전값
    FRotator StartRotation = FRotator::ZeroRotator;
    
    // 타겟 위치
    FVector TargetLocation = FVector::ZeroVector;
    
    // 회전 완료 여부
    bool bRotationCompleted = false;
    
    // 유효한 타겟이 있는지 여부
    bool bHasValidTarget = false;
    
    // 초기화 함수
    void Initialize()
    {
        StartTime = 0.0f;
        TargetRotation = FRotator::ZeroRotator;
        StartRotation = FRotator::ZeroRotator;
        TargetLocation = FVector::ZeroVector;
        bRotationCompleted = false;
        bHasValidTarget = false;
    }
};
