// Fill out your copyright notice in the Description page of Project Settings.

#include "StartFireTask.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

// Enemy 및 Gun 클래스 포함
#include "Enemy.h"
#include "Gun.h"

UStartFireTask::UStartFireTask()
{
    // 노드 이름 설정
    NodeName = TEXT("Start Fire");
    
    // 이 태스크는 Tick을 사용함
    bNotifyTick = true;
    
    // 인스턴스별로 실행됨
    bCreateNodeInstance = true;
    
    // === 블랙보드 키 기본값 설정 ===
    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UStartFireTask, TargetActorKey), AActor::StaticClass());
    TargetDistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UStartFireTask, TargetDistanceKey));
    
    // === 기본값 설정 ===
    MaxFiringRange = 1500.0f;
    MinFiringRange = 200.0f;
    FiringDuration = 5.0f;
    AimingTime = 0.5f;
    bStopFireOnTargetLoss = true;
    bAutoReloadOnEmpty = true;
    bUseBurstFire = false;
    bCheckAimAccuracy = true;
    AimAccuracyAngle = 10.0f;
}

EBTNodeResult::Type UStartFireTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // === 메모리 초기화 ===
    FStartFireTaskMemory* TaskMemory = reinterpret_cast<FStartFireTaskMemory*>(NodeMemory);
    TaskMemory->Initialize();
    
    // === 필수 컴포넌트 확인 ===
    AEnemy* ControlledEnemy = GetControlledEnemy(OwnerComp);
    if (!ControlledEnemy)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartFireTask: Enemy를 찾을 수 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartFireTask: Blackboard Component가 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    // === 타겟 정보 가져오기 ===
    AActor* targetActor = nullptr;
    float targetDistance = 0.0f;
    
    if (!GetTargetInfo(OwnerComp, targetActor, targetDistance))
    {
        UE_LOG(LogTemp, Warning, TEXT("StartFireTask: 유효한 타겟을 찾을 수 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    // === 발사 가능 여부 확인 ===
    if (!CanFire(OwnerComp, targetActor, targetDistance))
    {
        UE_LOG(LogTemp, Warning, TEXT("StartFireTask: 발사 조건을 만족하지 않습니다!"));
        return EBTNodeResult::Failed;
    }
    
    // === 메모리에 정보 저장 ===
    TaskMemory->StartTime = OwnerComp.GetWorld()->GetTimeSeconds();
    TaskMemory->AimingStartTime = TaskMemory->StartTime;
    TaskMemory->CurrentTarget = targetActor;
    
    // === 조준 시간이 있는 경우 조준 단계부터 시작 ===
    if (AimingTime > 0.0f)
    {
        TaskMemory->bAimingCompleted = false;
        UE_LOG(LogTemp, Log, TEXT("StartFireTask: 조준 시작 - Target: %s, Distance: %.1f"), 
            *targetActor->GetName(), targetDistance);
    }
    else
    {
        // 즉시 발사 시작
        TaskMemory->bAimingCompleted = true;
        StartFiring(OwnerComp, ControlledEnemy, TaskMemory);
    }
    
    return EBTNodeResult::InProgress;
}

void UStartFireTask::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // === 메모리 및 컴포넌트 확인 ===
    FStartFireTaskMemory* TaskMemory = reinterpret_cast<FStartFireTaskMemory*>(NodeMemory);
    
    AEnemy* ControlledEnemy = GetControlledEnemy(OwnerComp);
    if (!ControlledEnemy)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    float currentTime = OwnerComp.GetWorld()->GetTimeSeconds();
    
    // === 타겟 유효성 확인 ===
    AActor* currentTarget = TaskMemory->CurrentTarget.Get();
    if (!currentTarget && bStopFireOnTargetLoss)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartFireTask: 타겟을 잃었습니다!"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // === 타겟 정보 업데이트 ===
    AActor* targetActor = nullptr;
    float targetDistance = 0.0f;
    
    if (GetTargetInfo(OwnerComp, targetActor, targetDistance))
    {
        TaskMemory->CurrentTarget = targetActor;
        
        // === 발사 거리 체크 ===
        if (targetDistance > MaxFiringRange || targetDistance < MinFiringRange)
        {
            if (TaskMemory->bIsFiring)
            {
                StopFiring(OwnerComp, ControlledEnemy, TaskMemory);
            }
            UE_LOG(LogTemp, Warning, TEXT("StartFireTask: 타겟이 발사 범위를 벗어났습니다! Distance: %.1f"), targetDistance);
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            return;
        }
        
        // === 시야 확인 ===
        if (!HasLineOfSight(OwnerComp, targetActor))
        {
            if (TaskMemory->bIsFiring && bStopFireOnTargetLoss)
            {
                StopFiring(OwnerComp, ControlledEnemy, TaskMemory);
            }
            UE_LOG(LogTemp, Warning, TEXT("StartFireTask: 타겟이 보이지 않습니다!"));
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            return;
        }
    }
    
    // === 조준 단계 처리 ===
    if (!TaskMemory->bAimingCompleted)
    {
        float aimingElapsed = currentTime - TaskMemory->AimingStartTime;
        
        // 조준 정확도 체크
        if (bCheckAimAccuracy && targetActor)
        {
            if (IsAimingAccurate(OwnerComp, targetActor))
            {
                // 정확한 조준이면 조준 완료
                TaskMemory->bAimingCompleted = true;
                StartFiring(OwnerComp, ControlledEnemy, TaskMemory);
                UE_LOG(LogTemp, Log, TEXT("StartFireTask: 정확한 조준으로 발사 시작"));
            }
            else if (aimingElapsed >= AimingTime)
            {
                // 시간이 다 됐으면 강제로 발사 시작
                TaskMemory->bAimingCompleted = true;
                StartFiring(OwnerComp, ControlledEnemy, TaskMemory);
                UE_LOG(LogTemp, Log, TEXT("StartFireTask: 조준 시간 완료로 발사 시작"));
            }
        }
        else if (aimingElapsed >= AimingTime)
        {
            TaskMemory->bAimingCompleted = true;
            StartFiring(OwnerComp, ControlledEnemy, TaskMemory);
        }
    }
    
    // === 발사 단계 처리 ===
    if (TaskMemory->bIsFiring)
    {
        // === 탄약 확인 ===
        float timeSinceLastAmmoCheck = currentTime - TaskMemory->LastAmmoCheckTime;
        if (timeSinceLastAmmoCheck >= 1.0f) // 1초마다 탄약 확인
        {
            TaskMemory->LastAmmoCheckTime = currentTime;
            
            if (!HasAmmo(OwnerComp))
            {
                if (bAutoReloadOnEmpty)
                {
                    // 자동 재장전
                    if (ControlledEnemy->equippedGun)
                    {
                        ControlledEnemy->ReloadGun();
                        TaskMemory->bIsReloading = true;
                        UE_LOG(LogTemp, Log, TEXT("StartFireTask: 탄약 부족으로 재장전 시작"));
                    }
                }
                else
                {
                    // 재장전하지 않으면 발사 중단
                    StopFiring(OwnerComp, ControlledEnemy, TaskMemory);
                    FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
                    return;
                }
            }
        }
        
        // === 발사 지속 시간 체크 ===
        if (FiringDuration > 0.0f)
        {
            float firingElapsed = currentTime - TaskMemory->FiringStartTime;
            if (firingElapsed >= FiringDuration)
            {
                StopFiring(OwnerComp, ControlledEnemy, TaskMemory);
                FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
                return;
            }
        }
        
        // === 조준 정확도 지속 체크 ===
        if (bCheckAimAccuracy && targetActor)
        {
            if (!IsAimingAccurate(OwnerComp, targetActor))
            {
                // 조준이 부정확하면 일시 정지
                if (ControlledEnemy->isFiring)
                {
                    ControlledEnemy->StopGunFiring();
                    UE_LOG(LogTemp, Log, TEXT("StartFireTask: 조준 부정확으로 발사 일시 정지"));
                }
            }
            else
            {
                // 조준이 정확하면 발사 재개
                if (!ControlledEnemy->isFiring && !TaskMemory->bIsReloading)
                {
                    ControlledEnemy->StartGunFiring();
                    UE_LOG(LogTemp, Log, TEXT("StartFireTask: 조준 정확으로 발사 재개"));
                }
            }
        }
    }
    
    // === 디버그 정보 표시 ===
    #if WITH_EDITOR
    if (GEngine && GEngine->bEnableOnScreenDebugMessages)
    {
        FString debugText;
        if (!TaskMemory->bAimingCompleted)
        {
            float aimingProgress = (currentTime - TaskMemory->AimingStartTime) / FMath::Max(AimingTime, 0.1f);
            debugText = FString::Printf(TEXT("Aiming: %.1f%%"), aimingProgress * 100.0f);
        }
        else if (TaskMemory->bIsFiring)
        {
            if (FiringDuration > 0.0f)
            {
                float firingProgress = (currentTime - TaskMemory->FiringStartTime) / FiringDuration;
                debugText = FString::Printf(TEXT("Firing: %.1f%%"), firingProgress * 100.0f);
            }
            else
            {
                debugText = TEXT("Firing: Continuous");
            }
        }
        
        if (TaskMemory->bIsReloading)
        {
            debugText += TEXT(" [Reloading]");
        }
        
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, debugText);
    }
    #endif
}

uint16 UStartFireTask::GetInstanceMemorySize() const
{
    return sizeof(FStartFireTaskMemory);
}

void UStartFireTask::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // === 발사 중단 ===
    FStartFireTaskMemory* TaskMemory = reinterpret_cast<FStartFireTaskMemory*>(NodeMemory);
    
    if (AEnemy* ControlledEnemy = GetControlledEnemy(OwnerComp))
    {
        if (TaskMemory->bIsFiring)
        {
            StopFiring(OwnerComp, ControlledEnemy, TaskMemory);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("StartFireTask: 종료 - Result: %d"), (int32)TaskResult);
    
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

// === 내부 헬퍼 함수들 구현 ===

bool UStartFireTask::GetTargetInfo(UBehaviorTreeComponent& OwnerComp, AActor*& OutTargetActor, float& OutDistance) const
{
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    APawn* ControlledPawn = OwnerComp.GetAIOwner()->GetPawn();
    
    if (!BlackboardComp || !ControlledPawn)
    {
        return false;
    }
    
    // === 타겟 액터 가져오기 ===
    OutTargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
    if (!OutTargetActor || !IsValid(OutTargetActor))
    {
        return false;
    }
    
    // === 거리 계산 ===
    if (TargetDistanceKey.SelectedKeyName.IsValid())
    {
        // 블랙보드에서 거리 가져오기
        OutDistance = BlackboardComp->GetValueAsFloat(TargetDistanceKey.SelectedKeyName);
    }
    else
    {
        // 직접 거리 계산
        OutDistance = FVector::Dist(ControlledPawn->GetActorLocation(), OutTargetActor->GetActorLocation());
    }
    
    return true;
}

bool UStartFireTask::CanFire(UBehaviorTreeComponent& OwnerComp, AActor* TargetActor, float Distance) const
{
    if (!TargetActor)
    {
        return false;
    }
    
    // === 거리 체크 ===
    if (Distance > MaxFiringRange || Distance < MinFiringRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartFireTask: 타겟이 발사 범위를 벗어남 - Distance: %.1f"), Distance);
        return false;
    }
    
    // === 시야 체크 ===
    if (!HasLineOfSight(OwnerComp, TargetActor))
    {
        UE_LOG(LogTemp, Warning, TEXT("StartFireTask: 타겟이 보이지 않음"));
        return false;
    }
    
    // === 탄약 체크 ===
    if (!HasAmmo(OwnerComp) && !bAutoReloadOnEmpty)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartFireTask: 탄약 없음"));
        return false;
    }
    
    return true;
}

bool UStartFireTask::IsAimingAccurate(UBehaviorTreeComponent& OwnerComp, AActor* TargetActor) const
{
    APawn* ControlledPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (!ControlledPawn || !TargetActor)
    {
        return false;
    }
    
    // === 조준 각도 계산 ===
    FVector toTarget = (TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation()).GetSafeNormal();
    FVector forward = ControlledPawn->GetActorForwardVector();
    
    float dotProduct = FVector::DotProduct(forward, toTarget);
    float angleDegrees = FMath::RadiansToDegrees(FMath::Acos(dotProduct));
    
    return angleDegrees <= AimAccuracyAngle;
}

AEnemy* UStartFireTask::GetControlledEnemy(UBehaviorTreeComponent& OwnerComp) const
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return nullptr;
    }
    
    return Cast<AEnemy>(AIController->GetPawn());
}

bool UStartFireTask::HasAmmo(UBehaviorTreeComponent& OwnerComp) const
{
    AEnemy* ControlledEnemy = GetControlledEnemy(OwnerComp);
    if (!ControlledEnemy || !ControlledEnemy->equippedGun)
    {
        return false;
    }
    
    return ControlledEnemy->equippedGun->currentAmmoEquipped > 0;
}

bool UStartFireTask::HasLineOfSight(UBehaviorTreeComponent& OwnerComp, AActor* TargetActor) const
{
    APawn* ControlledPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (!ControlledPawn || !TargetActor)
    {
        return false;
    }
    
    UWorld* World = OwnerComp.GetWorld();
    if (!World)
    {
        return false;
    }
    
    // === 라인 트레이스로 시야 확인 ===
    FVector start = ControlledPawn->GetActorLocation() + FVector(0, 0, 50); // 눈 높이
    FVector end = TargetActor->GetActorLocation() + FVector(0, 0, 50);
    
    FHitResult hitResult;
    FCollisionQueryParams queryParams;
    queryParams.AddIgnoredActor(ControlledPawn);
    queryParams.AddIgnoredActor(TargetActor);
    
    bool bHit = World->LineTraceSingleByChannel(
        hitResult,
        start,
        end,
        ECC_WorldStatic,
        queryParams
    );
    
    return !bHit; // 충돌이 없으면 시야가 확보됨
}

void UStartFireTask::StartFiring(UBehaviorTreeComponent& OwnerComp, AEnemy* ControlledEnemy, FStartFireTaskMemory* TaskMemory)
{
    if (!ControlledEnemy || TaskMemory->bIsFiring)
    {
        return;
    }
    
    TaskMemory->bIsFiring = true;
    TaskMemory->FiringStartTime = OwnerComp.GetWorld()->GetTimeSeconds();
    TaskMemory->bIsReloading = false;
    
    // === 발사 방식 선택 ===
    if (bUseBurstFire && ControlledEnemy->canBurstFire)
    {
        ControlledEnemy->StartBurstFire();
        UE_LOG(LogTemp, Log, TEXT("StartFireTask: 버스트 파이어 시작"));
    }
    else
    {
        ControlledEnemy->StartGunFiring();
        UE_LOG(LogTemp, Log, TEXT("StartFireTask: 일반 발사 시작"));
    }
}

void UStartFireTask::StopFiring(UBehaviorTreeComponent& OwnerComp, AEnemy* ControlledEnemy, FStartFireTaskMemory* TaskMemory)
{
    if (!ControlledEnemy || !TaskMemory->bIsFiring)
    {
        return;
    }
    
    TaskMemory->bIsFiring = false;
    
    // === 발사 중단 ===
    if (bUseBurstFire && ControlledEnemy->isBurstFiring)
    {
        ControlledEnemy->StopBurstFire();
        UE_LOG(LogTemp, Log, TEXT("StartFireTask: 버스트 파이어 중단"));
    }
    else
    {
        ControlledEnemy->StopGunFiring();
        UE_LOG(LogTemp, Log, TEXT("StartFireTask: 일반 발사 중단"));
    }
}

