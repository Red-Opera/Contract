#include "InvestigateLastKnownPositionTask.h"
#include "AIController.h"
#include "AITypes.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "Enemy.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

UInvestigateLastKnownPositionTask::UInvestigateLastKnownPositionTask()
{
    // 노드 이름 설정
    NodeName = TEXT("Investigate Last Known Position");
    
    // 이 태스크는 Tick을 사용함
    bNotifyTick = true;
    
    // 인스턴스별로 실행됨
    bCreateNodeInstance = true;
    
    // === 블랙보드 키 기본값 설정 ===
    LastKnownLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UInvestigateLastKnownPositionTask, LastKnownLocationKey));
    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UInvestigateLastKnownPositionTask, TargetActorKey), AActor::StaticClass());
    IsAlertKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UInvestigateLastKnownPositionTask, IsAlertKey));
    
    // 🔧 전투 상태 블랙보드 키 추가
    isInCombatKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UInvestigateLastKnownPositionTask, isInCombatKey));
    
    // === 기본값 설정 ===
    AcceptanceRadius = 100.0f;
    investigationDuration = 8.0f;
    additionalSearchPoints = 3;
    searchRadius = 300.0f;
    waitTimeAtSearchPoint = 2.0f;
    maxExecutionTime = 30.0f;
    movementSpeed = 300.0f;
    isStopOnTargetFound = true;
    isClearAlertOnFailure = true;
    combatClearTime = 10.0f;
}

EBTNodeResult::Type UInvestigateLastKnownPositionTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // === 메모리 초기화 ===
    FInvestigateLastKnownPositionTaskMemory* taskMemory = reinterpret_cast<FInvestigateLastKnownPositionTaskMemory*>(NodeMemory);
    taskMemory->Initialize();

    // === 필수 컴포넌트 확인 ===
    AAIController* aiController = OwnerComp.GetAIOwner();

    if (aiController == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (NullAIController, InvestigateLastKnownPositionTask.cpp) : AI Controller가 없습니다!"));

        return EBTNodeResult::Failed;
    }

    APawn* controlledPawn = aiController->GetPawn();
    if (controlledPawn == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullPawn, InvestigateLastKnownPositionTask.cpp) : 제어할 Pawn이 없습니다!"));

        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* blackboardComp = OwnerComp.GetBlackboardComponent();
    if (blackboardComp == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (NullBlackboard, InvestigateLastKnownPositionTask.cpp) : Blackboard Component가 없습니다!"));

        return EBTNodeResult::Failed;
    }

    // === 현재 타겟이 있는지 확인 ===
    if (HasCurrentTarget(OwnerComp))
        return EBTNodeResult::Failed;

    // === 마지막 알려진 위치 가져오기 ===
    FVector lastKnownLocation;

    if (!GetLastKnownLocation(OwnerComp, lastKnownLocation))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NoLastKnownLocation, InvestigateLastKnownPositionTask.cpp) : 마지막 알려진 위치를 찾을 수 없습니다!"));

        return EBTNodeResult::Failed;
    }

    // === 현재 위치와 너무 가까운지 확인 ===
    FVector currentLocation = controlledPawn->GetActorLocation();
    float distanceToTarget = FVector::Dist(currentLocation, lastKnownLocation);

    // 이미 마지막 위치에 가까우면 바로 조사 단계로
    if (distanceToTarget <= AcceptanceRadius)
        taskMemory->CurrentPhase = EInvestigationPhase::InvestigatingAtLocation;

    // 마지막 위치로 이동 시작
    else
        taskMemory->CurrentPhase = EInvestigationPhase::MovingToLastKnownLocation;

    // === 메모리에 정보 저장 ===
    taskMemory->StartTime = OwnerComp.GetWorld()->GetTimeSeconds();
    taskMemory->CurrentPhaseStartTime = taskMemory->StartTime;
    taskMemory->LastKnownLocation = lastKnownLocation;
    taskMemory->CurrentTargetLocation = lastKnownLocation;

    // === 수색 포인트들 생성 ===
    taskMemory->SearchPoints = GenerateSearchPoints(lastKnownLocation, OwnerComp);

    // === 캐릭터 이동 속도 설정 ===
    if (ACharacter* character = Cast<ACharacter>(controlledPawn))
    {
        if (UCharacterMovementComponent* movementComp = character->GetCharacterMovement())
            movementComp->MaxWalkSpeed = movementSpeed;
    }

    // === 경계 상태 설정 ===
    if (UBlackboardComponent* blackboard = OwnerComp.GetBlackboardComponent())
        blackboard->SetValueAsBool(IsAlertKey.SelectedKeyName, true);

    return EBTNodeResult::InProgress;
}

void UInvestigateLastKnownPositionTask::TickTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
    // === 메모리 및 컴포넌트 확인 ===
    FInvestigateLastKnownPositionTaskMemory* taskMemory = reinterpret_cast<FInvestigateLastKnownPositionTaskMemory*>(nodeMemory);
    
    AAIController* aiController = ownerComp.GetAIOwner();

    if (aiController == nullptr)
    {
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);

        return;
    }
    
    APawn* controlledPawn = aiController->GetPawn();

    if (controlledPawn == nullptr)
    {
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);

        return;
    }
    
    float currentTime = ownerComp.GetWorld()->GetTimeSeconds();
    float elapsedTime = currentTime - taskMemory->StartTime;
    float phaseElapsedTime = currentTime - taskMemory->CurrentPhaseStartTime;
    
    // 🔧 전투 상태 해제 시간 체크 추가
    if (elapsedTime >= combatClearTime && !taskMemory->bCombatStateCleared)
    {
        if (UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent())
        {
            blackboard->SetValueAsBool(isInCombatKey.SelectedKeyName, false);
            taskMemory->bCombatStateCleared = true;
        }
    }
    
    // === 최대 실행 시간 체크 ===
    if (elapsedTime >= maxExecutionTime)
    {
        if (isClearAlertOnFailure)
        {
            if (UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent())
            {
                blackboard->SetValueAsBool(IsAlertKey.SelectedKeyName, false);
                blackboard->SetValueAsBool(isInCombatKey.SelectedKeyName, false);
            }
        }

        FinishLatentTask(ownerComp, EBTNodeResult::Failed);

        return;
    }
    
    // === 타겟 재발견 확인 ===
    if (isStopOnTargetFound && HasCurrentTarget(ownerComp))
    {
        taskMemory->bTargetRediscovered = true;
        UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 타겟 재발견으로 조사 중단"));

        FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);

        return;
    }
    
    // === 현재 단계에 따른 처리 ===
    switch (taskMemory->CurrentPhase)
    {
        case EInvestigationPhase::MovingToLastKnownLocation:
            HandleMovingToLastKnownLocation(ownerComp, taskMemory, currentTime);
            break;
            
        case EInvestigationPhase::InvestigatingAtLocation:
            HandleInvestigatingAtLocation(ownerComp, taskMemory, currentTime, phaseElapsedTime);
            break;
            
        case EInvestigationPhase::MovingToSearchPoint:
            HandleMovingToSearchPoint(ownerComp, taskMemory, currentTime);
            break;
            
        case EInvestigationPhase::SearchingAtPoint:
            HandleSearchingAtPoint(ownerComp, taskMemory, currentTime, phaseElapsedTime);
            break;
            
        case EInvestigationPhase::Completed:
        {
            if (isClearAlertOnFailure)
            {
                if (UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent())
                {
                    blackboard->SetValueAsBool(IsAlertKey.SelectedKeyName, false);
                    // 🔧 전투 상태도 해제
                    blackboard->SetValueAsBool(isInCombatKey.SelectedKeyName, false);
                }
            }

            FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);

            return;
        }
    }
    
    // === 디버그 정보 표시 ===
    #if WITH_EDITOR
    if (GEngine && GEngine->bEnableOnScreenDebugMessages)
    {
        FString phaseText;
        switch (taskMemory->CurrentPhase)
        {
            case EInvestigationPhase::MovingToLastKnownLocation:
                phaseText = TEXT("Moving to Last Known Location");
                break;

            case EInvestigationPhase::InvestigatingAtLocation:
                phaseText = TEXT("Investigating at Location");
                break;

            case EInvestigationPhase::MovingToSearchPoint:
                phaseText = FString::Printf(TEXT("Moving to Search Point %d/%d"), 
                    taskMemory->CurrentSearchPointIndex + 1, taskMemory->SearchPoints.Num());
                break;

            case EInvestigationPhase::SearchingAtPoint:
                phaseText = FString::Printf(TEXT("Searching at Point %d/%d"), 
                    taskMemory->CurrentSearchPointIndex + 1, taskMemory->SearchPoints.Num());
                break;

            default:
                phaseText = TEXT("Unknown Phase");
                break;
        }
        
        FString debugText = FString::Printf(TEXT("Investigation: %s (%.1fs)"), *phaseText, elapsedTime);
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, debugText);
        
        // 수색 포인트들 시각화
        UWorld* World = ownerComp.GetWorld();
        DrawDebugSphere(World, taskMemory->LastKnownLocation, 100.0f, 8, FColor::Red, false, 0.1f);
        
        for (int32 i = 0; i < taskMemory->SearchPoints.Num(); i++)
        {
            FColor pointColor = (i == taskMemory->CurrentSearchPointIndex) ? FColor::Green : FColor::Blue;
            DrawDebugSphere(World, taskMemory->SearchPoints[i], 50.0f, 8, pointColor, false, 0.1f);
        }
    }
    #endif
}

uint16 UInvestigateLastKnownPositionTask::GetInstanceMemorySize() const
{
    return sizeof(FInvestigateLastKnownPositionTaskMemory);
}

void UInvestigateLastKnownPositionTask::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // === 이동 중단 ===
    if (AAIController* AIController = OwnerComp.GetAIOwner())
    {
        AIController->StopMovement();
    }
    
    // === 메모리 정리 ===
    FInvestigateLastKnownPositionTaskMemory* TaskMemory = reinterpret_cast<FInvestigateLastKnownPositionTaskMemory*>(NodeMemory);
    TaskMemory->bIsMoving = false;
    
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

// === 내부 헬퍼 함수들 구현 ===

bool UInvestigateLastKnownPositionTask::GetLastKnownLocation(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const
{
    UBlackboardComponent* blackboardComp = ownerComp.GetBlackboardComponent();

    if (blackboardComp == nullptr)
        return false;
    
    FVector lastKnownLocation = blackboardComp->GetValueAsVector(LastKnownLocationKey.SelectedKeyName);

    if (lastKnownLocation.IsZero())
        return false;
    
    outLocation = lastKnownLocation;
    return true;
}

TArray<FVector> UInvestigateLastKnownPositionTask::GenerateSearchPoints(const FVector& centerLocation, UBehaviorTreeComponent& ownerComp) const
{
    TArray<FVector> searchPoints;
    
    if (additionalSearchPoints <= 0)
        return searchPoints;
    
    // === 중심점 주변에 원형으로 수색 포인트 배치 ===
    float angleStep = 360.0f / additionalSearchPoints;
    
    for (int32 i = 0; i < additionalSearchPoints; i++)
    {
        float angle = angleStep * i;
        float radians = FMath::DegreesToRadians(angle);
        
        FVector offset = FVector(
            FMath::Cos(radians) * searchRadius,
            FMath::Sin(radians) * searchRadius,
            0.0f
        );
        
        FVector searchPoint = centerLocation + offset;
        
        // 네비게이션 가능한 위치로 조정
        FVector navigablePoint = FindNavigablePosition(searchPoint, ownerComp);

        if (!navigablePoint.IsZero())
            searchPoints.Add(navigablePoint);
    }
    
    return searchPoints;
}

FVector UInvestigateLastKnownPositionTask::FindNavigablePosition(const FVector& desiredPosition, UBehaviorTreeComponent& ownerComp) const
{
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(ownerComp.GetWorld());

    if (navSystem == nullptr)
        return desiredPosition;
    
    FNavLocation navLocation;
    
    // === 원하는 위치에서 네비게이션 가능한 위치 찾기 ===
    if (navSystem->ProjectPointToNavigation(desiredPosition, navLocation, FVector(200.0f, 200.0f, 200.0f)))
        return navLocation.Location;
    
    // === 주변에서 네비게이션 가능한 위치 찾기 ===
    if (navSystem->GetRandomReachablePointInRadius(desiredPosition, 300.0f, navLocation))
        return navLocation.Location;
    
    return FVector::ZeroVector;
}

bool UInvestigateLastKnownPositionTask::HasCurrentTarget(UBehaviorTreeComponent& ownerComp) const
{
    UBlackboardComponent* blackboardComp = ownerComp.GetBlackboardComponent();

    if (blackboardComp == nullptr)
        return false;
    
    AActor* currentTarget = Cast<AActor>(blackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));

    return currentTarget != nullptr && IsValid(currentTarget);
}

bool UInvestigateLastKnownPositionTask::MoveToLocation(UBehaviorTreeComponent& ownerComp, const FVector& targetLocation) const
{
    AAIController* aiController = ownerComp.GetAIOwner();

    if (aiController == nullptr)
        return false;
    
    EPathFollowingRequestResult::Type moveResult = aiController->MoveToLocation(
        targetLocation,
        AcceptanceRadius,
        true,   // bStopOnOverlap
        true,   // bUsePathfinding
        true,   // bProjectDestinationToNavigation
        true,   // bCanStrafe
        nullptr, // FilterClass
        true    // bAllowPartialPath
    );
    
    return moveResult == EPathFollowingRequestResult::RequestSuccessful;
}

bool UInvestigateLastKnownPositionTask::HasReachedDestination(UBehaviorTreeComponent& ownerComp, const FVector& targetLocation) const
{
    APawn* controlledPawn = ownerComp.GetAIOwner()->GetPawn();

    if (controlledPawn == nullptr)
        return false;
    
    float distance = FVector::Dist(controlledPawn->GetActorLocation(), targetLocation);
    return distance <= AcceptanceRadius;
}

AEnemy* UInvestigateLastKnownPositionTask::GetControlledEnemy(UBehaviorTreeComponent& ownerComp) const
{
    AAIController* aiController = ownerComp.GetAIOwner();

    if (aiController == nullptr)
        return nullptr;
    
    return Cast<AEnemy>(aiController->GetPawn());
}

// === 단계별 처리 함수들 ===

void UInvestigateLastKnownPositionTask::HandleSearchingAtPoint(UBehaviorTreeComponent& ownerComp, FInvestigateLastKnownPositionTaskMemory* taskMemory, float currentTime, float phaseElapsedTime)
{
    // === 수색 시간 체크 ===
    if (phaseElapsedTime >= waitTimeAtSearchPoint)
    {
        // 다음 수색 포인트로 이동
        taskMemory->CurrentSearchPointIndex++;
        
            // 모든 수색 포인트 완료
        if (taskMemory->CurrentSearchPointIndex >= taskMemory->SearchPoints.Num())
            taskMemory->CurrentPhase = EInvestigationPhase::Completed;

        else
        {
            // 다음 수색 포인트로 이동
            taskMemory->CurrentPhase = EInvestigationPhase::MovingToSearchPoint;
            taskMemory->CurrentPhaseStartTime = currentTime;
            taskMemory->CurrentTargetLocation = taskMemory->SearchPoints[taskMemory->CurrentSearchPointIndex];
            taskMemory->bIsMoving = false;
        }
    }
}

void UInvestigateLastKnownPositionTask::HandleMovingToLastKnownLocation(UBehaviorTreeComponent& ownerComp, FInvestigateLastKnownPositionTaskMemory* taskMemory, float currentTime)
{
    // === 목적지 도달 확인 ===
    if (HasReachedDestination(ownerComp, taskMemory->LastKnownLocation))
    {
        // 조사 단계로 전환
        taskMemory->CurrentPhase = EInvestigationPhase::InvestigatingAtLocation;
        taskMemory->CurrentPhaseStartTime = currentTime;
        taskMemory->bIsMoving = false;
    }

    else if (!taskMemory->bIsMoving)
    {
        // 이동 시작
        if (MoveToLocation(ownerComp, taskMemory->LastKnownLocation))
            taskMemory->bIsMoving = true;

        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                TEXT("Error (MoveToLastKnownLocationFailed, InvestigateLastKnownPositionTask.cpp) : 마지막 위치로 이동에 실패했습니다!"));

            taskMemory->CurrentPhase = EInvestigationPhase::Completed;
        }
    }
}

void UInvestigateLastKnownPositionTask::HandleInvestigatingAtLocation(UBehaviorTreeComponent& ownerComp, FInvestigateLastKnownPositionTaskMemory* taskMemory, float currentTime, float phaseElapsedTime)
{
    // === 조사 시간 체크 ===
    if (phaseElapsedTime >= waitTimeAtSearchPoint)
    {
        // 추가 수색 포인트가 있는지 확인
        if (taskMemory->SearchPoints.Num() > 0)
        {
            // 첫 번째 수색 포인트로 이동 시작
            taskMemory->CurrentPhase = EInvestigationPhase::MovingToSearchPoint;
            taskMemory->CurrentPhaseStartTime = currentTime;
            taskMemory->CurrentSearchPointIndex = 0;
            taskMemory->CurrentTargetLocation = taskMemory->SearchPoints[0];
            taskMemory->bIsMoving = false;
        }

        // 수색 포인트가 없으면 조사 완료
        else
            taskMemory->CurrentPhase = EInvestigationPhase::Completed;
    }
}

void UInvestigateLastKnownPositionTask::HandleMovingToSearchPoint(UBehaviorTreeComponent& ownerComp, FInvestigateLastKnownPositionTaskMemory* taskMemory, float currentTime)
{
    // === 수색 포인트 도달 확인 ===
    if (HasReachedDestination(ownerComp, taskMemory->CurrentTargetLocation))
    {
        // 해당 포인트에서 수색 시작
        taskMemory->CurrentPhase = EInvestigationPhase::SearchingAtPoint;
        taskMemory->CurrentPhaseStartTime = currentTime;
        taskMemory->bIsMoving = false;
        
        UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 수색 포인트 %d 도달, 수색 시작"), 
            taskMemory->CurrentSearchPointIndex + 1);
    }

    else if (!taskMemory->bIsMoving)
    {
        // 수색 포인트로 이동 시작
        if (MoveToLocation(ownerComp, taskMemory->CurrentTargetLocation))
            taskMemory->bIsMoving = true;

        else
        {
            // 이동 실패 시 다음 포인트로
            taskMemory->CurrentSearchPointIndex++;

            if (taskMemory->CurrentSearchPointIndex >= taskMemory->SearchPoints.Num())
                taskMemory->CurrentPhase = EInvestigationPhase::Completed;

            else
                taskMemory->CurrentTargetLocation = taskMemory->SearchPoints[taskMemory->CurrentSearchPointIndex];
        }
    }
}