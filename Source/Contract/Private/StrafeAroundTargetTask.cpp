// Fill out your copyright notice in the Description page of Project Settings.

#include "StrafeAroundTargetTask.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

// 🔧 필요한 AI 관련 헤더 추가
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "AISystem.h"

UStrafeAroundTargetTask::UStrafeAroundTargetTask()
{
    // 노드 이름 설정
    NodeName = TEXT("Strafe Around Target");
    
    // 이 태스크는 Tick을 사용함
    bNotifyTick = true;
    
    // 인스턴스별로 실행됨
    bCreateNodeInstance = true;
    
    // === 블랙보드 키 기본값 설정 ===
    targetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UStrafeAroundTargetTask, targetActorKey), AActor::StaticClass());
    lastKnownLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UStrafeAroundTargetTask, lastKnownLocationKey));
    
    // === 기본값 설정 - 더 반응성 있게 조정 ===
    optimalDistance = 800.0f;
    distanceTolerance = 80.0f;     // 기존 100.0f에서 80.0f로 감소 (더 민감하게)
    strafeSpeed = 60.0f;           // 기존 45.0f에서 60.0f로 증가 (더 빠른 스트레이프)
    directionChangeInterval = 2.5f; // 기존 3.0f에서 2.5f로 감소 (더 자주 방향 변경)
    maxExecutionTime = 10.0f;
    movementSpeed = 450.0f;        // 기존 400.0f에서 450.0f로 증가 (더 빠른 이동)
    strafePattern = EStrafePattern::Adaptive;
    isAvoidTargetLOS = true;
    obstacleAvoidanceRadius = 150.0f;
}

EBTNodeResult::Type UStrafeAroundTargetTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // === 메모리 초기화 ===
    FStrafeAroundTargetTaskMemory* taskMemory = reinterpret_cast<FStrafeAroundTargetTaskMemory*>(NodeMemory);
    taskMemory->Initialize();
    
    // === 필수 컴포넌트 확인 ===
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

    if (!AIController)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullAIController, StrafeAroundTargetTask.cpp) : AI Controller가 없습니다!"));
        return EBTNodeResult::Failed;
    }

    if (!ControlledPawn)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullPawn, StrafeAroundTargetTask.cpp) : 제어할 Pawn이 없습니다!"));
        return EBTNodeResult::Failed;
    }

    if (!BlackboardComp)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullBlackboard, StrafeAroundTargetTask.cpp) : Blackboard Component가 없습니다!"));
        return EBTNodeResult::Failed;
    }

    if (targetActorKey.SelectedKeyName.IsNone())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NoTargetActorKey, StrafeAroundTargetTask.cpp) : TargetActorKey가 설정되지 않았습니다!"));
        return EBTNodeResult::Failed;
    }

    if (lastKnownLocationKey.SelectedKeyName.IsNone())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            TEXT("Error (NoLastKnownLocationKey, StrafeAroundTargetTask.cpp) : LastKnownLocationKey가 설정되지 않았습니다!"));
    }
    
    // 블랙보드 값 확인
    AActor* targetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(targetActorKey.SelectedKeyName));
    FVector lastKnownLoc = BlackboardComp->GetValueAsVector(lastKnownLocationKey.SelectedKeyName);
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("StrafeAroundTargetTask: TargetActor: %s, LastKnownLoc: %s"), 
        targetActor ? *targetActor->GetName() : TEXT("NULL"),
        *lastKnownLoc.ToString()));
    
    // === 타겟 정보 계산 ===
    FVector targetLocation;
    float currentDistance;
    
    if (!CalculateTargetInfo(OwnerComp, targetLocation, currentDistance))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NoValidTarget, StrafeAroundTargetTask.cpp) : 유효한 타겟을 찾을 수 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    // === 메모리에 정보 저장 ===
    taskMemory->startTime = OwnerComp.GetWorld()->GetTimeSeconds();
    taskMemory->lastDirectionChangeTime = taskMemory->startTime;
    taskMemory->targetLocation = targetLocation;
    taskMemory->lastTargetDistance = currentDistance;
    
    // === 스트레이프 패턴에 따른 초기 방향 설정 ===
    switch (strafePattern)
    {
        case EStrafePattern::Clockwise:
            taskMemory->isCurrentlyClockwise = true;
            break;
            
        case EStrafePattern::CounterClockwise:
            taskMemory->isCurrentlyClockwise = false;
            break;

        case EStrafePattern::Random:
        case EStrafePattern::Adaptive:
        default:
            taskMemory->isCurrentlyClockwise = FMath::RandBool();
            break;
    }
    
    // === 캐릭터 이동 속도 설정 ===
    if (ACharacter* Character = Cast<ACharacter>(ControlledPawn))
    {
        if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
            MovementComp->MaxWalkSpeed = movementSpeed;
    }

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("StrafeAroundTargetTask: 스트레이프 시작 - Distance: %.1f, Optimal: %.1f"),
        currentDistance, optimalDistance));

    return EBTNodeResult::InProgress;
}

void UStrafeAroundTargetTask::TickTask(UBehaviorTreeComponent& ownerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // === 메모리 및 컴포넌트 확인 ===
    FStrafeAroundTargetTaskMemory* taskMemory = reinterpret_cast<FStrafeAroundTargetTaskMemory*>(NodeMemory);
    
    AAIController* aiController = ownerComp.GetAIOwner();

    APawn* controlledPawn = aiController ? aiController->GetPawn() : nullptr;

    if (!aiController)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullAIController, StrafeAroundTargetTask.cpp) : AIController가 없습니다!"));
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);
        return;
    }

    if (controlledPawn == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullPawn, StrafeAroundTargetTask.cpp) : ControlledPawn이 없습니다!"));
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);
        return;
    }
    
    // === 최대 실행 시간 체크 ===
    float currentTime = ownerComp.GetWorld()->GetTimeSeconds();
    float elapsedTime = currentTime - taskMemory->startTime;
    
    if (elapsedTime >= maxExecutionTime)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("StrafeAroundTargetTask: 최대 실행 시간 도달 (%.2fs)"), elapsedTime));
        FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // === 타겟 정보 업데이트 ===
    FVector targetLocation;
    float currentDistance;
    
    if (!CalculateTargetInfo(ownerComp, targetLocation, currentDistance))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (LostTarget, StrafeAroundTargetTask.cpp) : 타겟을 잃었습니다!"));
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);
        return;
    }
    
    taskMemory->targetLocation = targetLocation;
    taskMemory->lastTargetDistance = currentDistance;
    
    // === 방향 변경 타이밍 체크 ===
    float timeSinceDirectionChange = currentTime - taskMemory->lastDirectionChangeTime;

    if (timeSinceDirectionChange >= directionChangeInterval)
    {
        // === 스트레이프 패턴에 따른 방향 변경 ===
        if (strafePattern == EStrafePattern::Random)
            taskMemory->isCurrentlyClockwise = FMath::RandBool();

        else if (strafePattern == EStrafePattern::Adaptive)
        {
            // Adaptive 모드: 타겟의 시야를 피하는 방향으로 이동
            FVector currentPos = controlledPawn->GetActorLocation();
            FVector targetForward = FVector::ZeroVector;
            
            // 타겟이 액터인 경우 Forward 벡터 가져오기
            if (AActor* targetActor = Cast<AActor>(ownerComp.GetBlackboardComponent()->GetValueAsObject(targetActorKey.SelectedKeyName)))
                targetForward = targetActor->GetActorForwardVector();
            
            // 현재 위치가 타겟의 시야각 내에 있다면 반대 방향으로 변경
            if (IsInTargetLOS(currentPos, targetLocation, targetForward))
                taskMemory->isCurrentlyClockwise = !taskMemory->isCurrentlyClockwise;
        }
        
        taskMemory->lastDirectionChangeTime = currentTime;

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("StrafeAroundTargetTask: 방향 변경 - %s"), 
			taskMemory->isCurrentlyClockwise ? TEXT("시계방향") : TEXT("반시계방향")));
    }
    
    // === 거리 조정이 필요한지 확인 ===
    float distanceDiff = FMath::Abs(currentDistance - optimalDistance);
    taskMemory->isDistanceAdjustmentMode = (distanceDiff > distanceTolerance);
    
    // === 목표 위치 계산 ===
    FVector currentLocation = controlledPawn->GetActorLocation();
    FVector desiredPosition;
    
    if (taskMemory->isDistanceAdjustmentMode)
    {
        // 거리 조정 모드: 최적 거리로 이동
        desiredPosition = CalculateDistanceAdjustmentPosition(currentLocation, targetLocation, currentDistance);

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("StrafeAroundTargetTask: 거리 조정 모드 - Current: %.1f, Target: %.1f"));
    }

        // 스트레이프 모드: 타겟 주변을 원형으로 이동
    else
        desiredPosition = CalculateStrafePosition(currentLocation, targetLocation, currentDistance, taskMemory->isCurrentlyClockwise);
    
    // === 네비게이션 가능한 위치로 조정 ===
    FVector navigablePosition = FindNavigablePosition(desiredPosition, ownerComp);
    
    if (navigablePosition.IsZero())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NoNavigablePosition, StrafeAroundTargetTask.cpp) : 이동 가능한 위치를 찾을 수 없습니다!"));
        taskMemory->isCurrentlyClockwise = !taskMemory->isCurrentlyClockwise;
        return;
    }
    
    // === 현재 목표 위치와 다르면 새로운 이동 명령 ===
    float distanceToCurrentTarget = FVector::Dist(navigablePosition, taskMemory->currentTargetPosition);
    
    if (!taskMemory->isMoving || distanceToCurrentTarget > 100.0f)
    {
        taskMemory->currentTargetPosition = navigablePosition;
        
        // === AI 이동 명령 ===
        FAIMoveRequest MoveRequest;
        MoveRequest.SetGoalLocation(navigablePosition);
        MoveRequest.SetAcceptanceRadius(50.0f);
        MoveRequest.SetUsePathfinding(true);
        MoveRequest.SetAllowPartialPath(true);
        
        FPathFollowingRequestResult MoveResult = aiController->MoveTo(MoveRequest);
        
        if (MoveResult.Code == EPathFollowingRequestResult::RequestSuccessful)
        {
            taskMemory->moveRequestID = MoveResult.MoveId;
            taskMemory->isMoving = true;

            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("StrafeAroundTargetTask: 새로운 위치로 이동 시작 - %s"), *navigablePosition.ToString()));
        }

        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                TEXT("Error (MoveRequestFailed, StrafeAroundTargetTask.cpp) : 이동 요청 실패"));
            taskMemory->isMoving = false;
        }
    }
    
    // === 디버그 그리기 ===
    #if WITH_EDITOR
    if (GEngine && GEngine->bEnableOnScreenDebugMessages)
    {
        // 더 자세한 디버그 정보
        FString modeText = taskMemory->isDistanceAdjustmentMode ? TEXT("[Adjusting]") : TEXT("[Strafing]");
        FString directionText = taskMemory->isCurrentlyClockwise ? TEXT("CW") : TEXT("CCW");
        
        FString debugText = FString::Printf(TEXT("Strafe: %.1fm (Target: %.1fm) %s %s"), 
            currentDistance, optimalDistance, *modeText, *directionText);
        
        // 추가 디버그 정보
        FString detailText = FString::Printf(TEXT("Pos: %s | Target: %s"), 
            *controlledPawn->GetActorLocation().ToString(), 
            *targetLocation.ToString());
        
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan, debugText);
        GEngine->AddOnScreenDebugMessage(-2, 0.0f, FColor::Yellow, detailText);
        
        // 타겟과 목표 위치 시각화
        DrawDebugSphere(ownerComp.GetWorld(), targetLocation, 50.0f, 8, FColor::Red, false, 0.1f);
        DrawDebugSphere(ownerComp.GetWorld(), navigablePosition, 30.0f, 8, FColor::Green, false, 0.1f);
        DrawDebugLine(ownerComp.GetWorld(), controlledPawn->GetActorLocation(), navigablePosition, FColor::Yellow, false, 0.1f, 0, 2.0f);
        
        // 최적 거리 원 그리기
        DrawDebugCircle(ownerComp.GetWorld(), targetLocation, optimalDistance, 32, FColor::Blue, false, 0.1f, 0, 2.0f, FVector(0,1,0), FVector(1,0,0));
    }
    #endif
}

uint16 UStrafeAroundTargetTask::GetInstanceMemorySize() const
{
    return sizeof(FStrafeAroundTargetTaskMemory);
}

void UStrafeAroundTargetTask::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // === 이동 중단 ===
    if (AAIController* AIController = OwnerComp.GetAIOwner())
    {
        AIController->StopMovement();
    }
    
    // === 메모리 정리 ===
    FStrafeAroundTargetTaskMemory* TaskMemory = reinterpret_cast<FStrafeAroundTargetTaskMemory*>(NodeMemory);
    TaskMemory->isMoving = false;
    TaskMemory->moveRequestID = FAIRequestID::InvalidRequest;
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("StrafeAroundTargetTask: 종료 - Result: %d"), (int32)TaskResult));
    
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

// === 내부 헬퍼 함수들 구현 ===

bool UStrafeAroundTargetTask::CalculateTargetInfo(UBehaviorTreeComponent& ownerComp, FVector& outTargetLocation, float& outCurrentDistance) const
{
    UBlackboardComponent* BlackboardComp = ownerComp.GetBlackboardComponent();
    APawn* ControlledPawn = ownerComp.GetAIOwner()->GetPawn();

    if (!BlackboardComp || !ControlledPawn)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("StrafeAroundTargetTask: 필수 컴포넌트가 없습니다!"));
        outCurrentDistance = 0.0f;
        return false;
    }

    FVector currentLocation = ControlledPawn->GetActorLocation();
    bool isTargetFound = false;

    if (!targetActorKey.SelectedKeyName.IsNone())
    {
        AActor* targetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(targetActorKey.SelectedKeyName));
        if (targetActor && IsValid(targetActor))
        {
            outTargetLocation = targetActor->GetActorLocation();
            outCurrentDistance = FVector::Dist(currentLocation, outTargetLocation);
            isTargetFound = true;
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("StrafeAroundTargetTask: 타겟 액터 사용 - %s, 거리: %.2f"), *targetActor->GetName(), outCurrentDistance));
        }
    }

    if (!isTargetFound && !lastKnownLocationKey.SelectedKeyName.IsNone())
    {
        FVector lastKnownLocation = BlackboardComp->GetValueAsVector(lastKnownLocationKey.SelectedKeyName);
        if (!lastKnownLocation.IsZero())
        {
            outTargetLocation = lastKnownLocation;
            outCurrentDistance = FVector::Dist(currentLocation, outTargetLocation);
            isTargetFound = true;
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("StrafeAroundTargetTask: 마지막 위치 사용 - %s, 거리: %.2f"), *lastKnownLocation.ToString(), outCurrentDistance));
        }
    }

    if (!isTargetFound)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("StrafeAroundTargetTask: 타겟을 찾을 수 없습니다!"));
        outCurrentDistance = 0.0f;
        outTargetLocation = FVector::ZeroVector;
    }

    return isTargetFound;
}

FVector UStrafeAroundTargetTask::CalculateStrafePosition(const FVector& currentLocation, const FVector& targetLocation, float currentDistance, bool isClockwise) const
{
    // === 타겟을 중심으로 한 원형 이동 ===
    FVector toTarget = (targetLocation - currentLocation).GetSafeNormal();
    FVector rightVector = FVector::CrossProduct(toTarget, FVector::UpVector).GetSafeNormal();
    
    // 스트레이프 방향 결정
    FVector strafeDirection = isClockwise ? rightVector : -rightVector;
    
    // 현재 거리를 유지하면서 옆으로 이동
    float strafeDistance = (strafeSpeed / 180.0f) * PI * currentDistance / 4.0f; // 90도 호의 길이
    FVector strafeOffset = strafeDirection * strafeDistance;
    
    // 목표 위치 계산
    FVector desiredPosition = currentLocation + strafeOffset;
    
    return desiredPosition;
}

FVector UStrafeAroundTargetTask::CalculateDistanceAdjustmentPosition(const FVector& currentLocation, const FVector& targetLocation, float currentDistance) const
{
    // === 최적 거리로 조정 ===
    FVector directionToTarget = (targetLocation - currentLocation).GetSafeNormal();
    
    if (currentDistance > optimalDistance)
    {
        // 너무 멀면 가까이 이동 - 계수를 0.8f로 증가 (기존 0.5f)
        float moveDistance = currentDistance - optimalDistance;

        return currentLocation + (directionToTarget * moveDistance * 0.8f);
    }

    else
    {
        // 너무 가까우면 멀리 이동 - 계수를 0.8f로 증가 (기존 0.5f)
        float moveDistance = optimalDistance - currentDistance;

        return currentLocation - (directionToTarget * moveDistance * 0.8f);
    }
}

FVector UStrafeAroundTargetTask::FindNavigablePosition(const FVector& desiredPosition, UBehaviorTreeComponent& ownerComp) const
{
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(ownerComp.GetWorld());

    if (navSystem == nullptr)
        return desiredPosition;
    
    FNavLocation navLocation;
    
    // === 1. 원하는 위치에서 네비게이션 가능한 위치 찾기 ===
    if (navSystem->ProjectPointToNavigation(desiredPosition, navLocation, FVector(100.0f, 100.0f, 100.0f)))
        return navLocation.Location;
    
    // === 2. 주변에서 네비게이션 가능한 위치 찾기 ===
    TArray<float> searchRadii = {100.0f, 200.0f, 300.0f};
    
    for (float radius : searchRadii)
    {
        if (navSystem->GetRandomReachablePointInRadius(desiredPosition, radius, navLocation))
            return navLocation.Location;
    }
    
    return FVector::ZeroVector;
}

bool UStrafeAroundTargetTask::IsPathClear(const FVector& start, const FVector& end, UBehaviorTreeComponent& ownerComp) const
{
    UWorld* world = ownerComp.GetWorld();

    if (world == nullptr)
        return true;
    
    // === 라인 트레이스로 경로 확인 ===
    FHitResult hitResult;
    FCollisionQueryParams queryParams;
    queryParams.AddIgnoredActor(ownerComp.GetAIOwner()->GetPawn());
    
    bool isHit = world->LineTraceSingleByChannel(
        hitResult,
        start,
        end,
        ECC_WorldStatic,
        queryParams
    );
    
    return !isHit;
}

bool UStrafeAroundTargetTask::IsInTargetLOS(const FVector& position, const FVector& targetLocation, const FVector& targetForward) const
{
    if (targetForward.IsZero())
        return false;
    
    // === 타겟의 시야각 내에 있는지 확인 (90도 시야각) ===
    FVector toPosition = (position - targetLocation).GetSafeNormal();
    float dotProduct = FVector::DotProduct(targetForward, toPosition);
    
    // cos(45도) = 0.707 (90도 시야각의 절반)
    return dotProduct > 0.707f;
}