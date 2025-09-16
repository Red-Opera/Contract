// Fill out your copyright notice in the Description page of Project Settings.

#include "LookAtTargetTask.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

ULookAtTargetTask::ULookAtTargetTask()
{
    // 노드 이름 설정
    NodeName = TEXT("Look At Target");
    
    // 이 태스크는 Tick을 사용함
    bNotifyTick = true;
    
    // 인스턴스별로 실행됨
    bCreateNodeInstance = true;
    
    // === 블랙보드 키 기본값 설정 ===
    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ULookAtTargetTask, TargetActorKey), AActor::StaticClass());
    LastKnownLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ULookAtTargetTask, LastKnownLocationKey));
    
    // === 기본값 설정 ===
    RotationSpeed = 180.0f;
    AcceptableAngle = 5.0f;
    MaxExecutionTime = 3.0f;
    bOnlyYawRotation = true;
    bInstantRotation = false;
}

EBTNodeResult::Type ULookAtTargetTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // === 메모리 초기화 ===
    FLookAtTargetTaskMemory* TaskMemory = reinterpret_cast<FLookAtTargetTaskMemory*>(NodeMemory);
    TaskMemory->Initialize();
    
    // === 필수 컴포넌트 확인 ===
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogTemp, Warning, TEXT("LookAtTargetTask: AI Controller가 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("LookAtTargetTask: 제어할 Pawn이 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("LookAtTargetTask: Blackboard Component가 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    // === 타겟 위치 계산 ===
    FVector targetLocation = CalculateTargetLocation(OwnerComp);
    
    if (targetLocation.IsZero())
    {
        UE_LOG(LogTemp, Warning, TEXT("LookAtTargetTask: 유효한 타겟 위치를 찾을 수 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    // === 메모리에 정보 저장 ===
    TaskMemory->TargetLocation = targetLocation;
    TaskMemory->StartTime = OwnerComp.GetWorld()->GetTimeSeconds();
    TaskMemory->StartRotation = ControlledPawn->GetActorRotation();
    TaskMemory->bHasValidTarget = true;
    
    // === 목표 회전값 계산 ===
    FVector currentLocation = ControlledPawn->GetActorLocation();
    TaskMemory->TargetRotation = CalculateTargetRotation(currentLocation, targetLocation);
    
    // === 즉시 회전 모드인 경우 ===
    if (bInstantRotation)
    {
        ControlledPawn->SetActorRotation(TaskMemory->TargetRotation);
        TaskMemory->bRotationCompleted = true;
        
        UE_LOG(LogTemp, Log, TEXT("LookAtTargetTask: 즉시 회전 완료"));
        return EBTNodeResult::Succeeded;
    }
    
    // === 회전이 이미 완료된 상태인지 확인 ===
    if (IsRotationComplete(TaskMemory->StartRotation, TaskMemory->TargetRotation))
    {
        TaskMemory->bRotationCompleted = true;
        UE_LOG(LogTemp, Log, TEXT("LookAtTargetTask: 이미 올바른 방향을 바라보고 있음"));
        return EBTNodeResult::Succeeded;
    }
    
    // === Tick에서 부드러운 회전 처리 ===
    UE_LOG(LogTemp, Log, TEXT("LookAtTargetTask: 부드러운 회전 시작 - Target: %s"), *targetLocation.ToString());
    return EBTNodeResult::InProgress;
}

void ULookAtTargetTask::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // === 메모리 및 컴포넌트 확인 ===
    FLookAtTargetTaskMemory* TaskMemory = reinterpret_cast<FLookAtTargetTaskMemory*>(NodeMemory);
    
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // === 최대 실행 시간 체크 ===
    float currentTime = OwnerComp.GetWorld()->GetTimeSeconds();
    float elapsedTime = currentTime - TaskMemory->StartTime;
    
    if (elapsedTime >= MaxExecutionTime)
    {
        UE_LOG(LogTemp, Warning, TEXT("LookAtTargetTask: 최대 실행 시간 초과 (%.2fs)"), elapsedTime);
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // === 현재 회전값 및 목표 회전값 ===
    FRotator currentRotation = ControlledPawn->GetActorRotation();
    FRotator targetRotation = TaskMemory->TargetRotation;
    
    // === 타겟 위치 업데이트 (동적 타겟의 경우) ===
    FVector updatedTargetLocation = CalculateTargetLocation(OwnerComp);
    if (!updatedTargetLocation.IsZero() && !updatedTargetLocation.Equals(TaskMemory->TargetLocation, 50.0f))
    {
        TaskMemory->TargetLocation = updatedTargetLocation;
        targetRotation = CalculateTargetRotation(ControlledPawn->GetActorLocation(), updatedTargetLocation);
        TaskMemory->TargetRotation = targetRotation;
    }
    
    // === 회전 완료 체크 ===
    if (IsRotationComplete(currentRotation, targetRotation))
    {
        TaskMemory->bRotationCompleted = true;
        UE_LOG(LogTemp, Log, TEXT("LookAtTargetTask: 회전 완료 (%.2fs)"), elapsedTime);
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // === 부드러운 회전 적용 ===
    FRotator newRotation = SafeRotationInterp(currentRotation, targetRotation, DeltaSeconds, RotationSpeed);
    ControlledPawn->SetActorRotation(newRotation);
    
    // === 디버그 정보 (필요시) ===
    #if WITH_EDITOR
    if (GEngine && GEngine->bEnableOnScreenDebugMessages)
    {
        float angleDifference = FMath::Abs(FRotator::NormalizeAxis(targetRotation.Yaw - currentRotation.Yaw));
        FString debugText = FString::Printf(TEXT("LookAtTarget: %.1f° remaining"), angleDifference);
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, debugText);
    }
    #endif
}

uint16 ULookAtTargetTask::GetInstanceMemorySize() const
{
    return sizeof(FLookAtTargetTaskMemory);
}

// === 내부 헬퍼 함수들 구현 ===

FVector ULookAtTargetTask::CalculateTargetLocation(UBehaviorTreeComponent& OwnerComp) const
{
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return FVector::ZeroVector;
    }
    
    // === 1. 타겟 액터가 있는지 확인 ===
    AActor* targetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
    if (targetActor && IsValid(targetActor))
    {
        // 캐릭터인 경우 약간 위쪽을 바라보도록 조정
        FVector targetLocation = targetActor->GetActorLocation();
        if (ACharacter* targetCharacter = Cast<ACharacter>(targetActor))
        {
            targetLocation.Z += 80.0f; // 캐릭터의 가슴 높이 정도
        }
        return targetLocation;
    }
    
    // === 2. 마지막으로 알려진 위치 사용 ===
    FVector lastKnownLocation = BlackboardComp->GetValueAsVector(LastKnownLocationKey.SelectedKeyName);
    if (!lastKnownLocation.IsZero())
    {
        return lastKnownLocation;
    }
    
    // === 3. 유효한 타겟을 찾을 수 없음 ===
    return FVector::ZeroVector;
}

FRotator ULookAtTargetTask::CalculateTargetRotation(const FVector& CurrentLocation, const FVector& TargetLocation) const
{
    FVector directionToTarget = (TargetLocation - CurrentLocation).GetSafeNormal();
    
    if (directionToTarget.IsZero())
    {
        return FRotator::ZeroRotator;
    }
    
    // === 타겟을 향한 회전값 계산 ===
    FRotator lookAtRotation = UKismetMathLibrary::FindLookAtRotation(CurrentLocation, TargetLocation);
    
    // === Z축 회전만 사용하는 경우 ===
    if (bOnlyYawRotation)
    {
        lookAtRotation.Pitch = 0.0f;
        lookAtRotation.Roll = 0.0f;
    }
    
    return lookAtRotation;
}

bool ULookAtTargetTask::IsRotationComplete(const FRotator& CurrentRotation, const FRotator& TargetRotation) const
{
    // === 각도 차이 계산 ===
    float yawDifference = FMath::Abs(FRotator::NormalizeAxis(TargetRotation.Yaw - CurrentRotation.Yaw));
    
    if (bOnlyYawRotation)
    {
        return yawDifference <= AcceptableAngle;
    }
    else
    {
        float pitchDifference = FMath::Abs(FRotator::NormalizeAxis(TargetRotation.Pitch - CurrentRotation.Pitch));
        float rollDifference = FMath::Abs(FRotator::NormalizeAxis(TargetRotation.Roll - CurrentRotation.Roll));
        
        return (yawDifference <= AcceptableAngle && 
                pitchDifference <= AcceptableAngle && 
                rollDifference <= AcceptableAngle);
    }
}

FRotator ULookAtTargetTask::SafeRotationInterp(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed) const
{
    // === 안전한 회전 보간 ===
    FRotator result = Current;
    
    // Yaw 보간
    float yawDiff = FRotator::NormalizeAxis(Target.Yaw - Current.Yaw);
    float yawStep = FMath::Sign(yawDiff) * FMath::Min(FMath::Abs(yawDiff), InterpSpeed * DeltaTime);
    result.Yaw = FRotator::NormalizeAxis(Current.Yaw + yawStep);
    
    // Z축 회전만 사용하지 않는 경우 Pitch, Roll도 보간
    if (!bOnlyYawRotation)
    {
        float pitchDiff = FRotator::NormalizeAxis(Target.Pitch - Current.Pitch);
        float pitchStep = FMath::Sign(pitchDiff) * FMath::Min(FMath::Abs(pitchDiff), InterpSpeed * DeltaTime);
        result.Pitch = FRotator::NormalizeAxis(Current.Pitch + pitchStep);
        
        float rollDiff = FRotator::NormalizeAxis(Target.Roll - Current.Roll);
        float rollStep = FMath::Sign(rollDiff) * FMath::Min(FMath::Abs(rollDiff), InterpSpeed * DeltaTime);
        result.Roll = FRotator::NormalizeAxis(Current.Roll + rollStep);
    }
    
    return result;
}

