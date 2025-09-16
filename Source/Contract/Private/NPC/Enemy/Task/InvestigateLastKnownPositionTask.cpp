// Fill out your copyright notice in the Description page of Project Settings.

#include "InvestigateLastKnownPositionTask.h"
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

// 🔧 AI 관련 헤더 추가
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"

// Enemy 클래스 포함
#include "Enemy.h"

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
    
    // === 기본값 설정 ===
    AcceptanceRadius = 100.0f;
    InvestigationDuration = 8.0f;
    AdditionalSearchPoints = 3;
    SearchRadius = 300.0f;
    WaitTimeAtSearchPoint = 2.0f;
    MaxExecutionTime = 30.0f;
    MovementSpeed = 300.0f;
    bStopOnTargetFound = true;
    bClearAlertOnFailure = true;
}

EBTNodeResult::Type UInvestigateLastKnownPositionTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // === 메모리 초기화 ===
    FInvestigateLastKnownPositionTaskMemory* TaskMemory = reinterpret_cast<FInvestigateLastKnownPositionTaskMemory*>(NodeMemory);
    TaskMemory->Initialize();
    
    // === 필수 컴포넌트 확인 ===
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogTemp, Warning, TEXT("InvestigateLastKnownPositionTask: AI Controller가 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("InvestigateLastKnownPositionTask: 제어할 Pawn이 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("InvestigateLastKnownPositionTask: Blackboard Component가 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    // === 현재 타겟이 있는지 확인 ===
    if (HasCurrentTarget(OwnerComp))
    {
        UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 현재 타겟이 있어서 조사가 필요하지 않습니다."));
        return EBTNodeResult::Failed;
    }
    
    // === 마지막 알려진 위치 가져오기 ===
    FVector lastKnownLocation;
    if (!GetLastKnownLocation(OwnerComp, lastKnownLocation))
    {
        UE_LOG(LogTemp, Warning, TEXT("InvestigateLastKnownPositionTask: 마지막 알려진 위치를 찾을 수 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    // === 현재 위치와 너무 가까운지 확인 ===
    FVector currentLocation = ControlledPawn->GetActorLocation();
    float distanceToTarget = FVector::Dist(currentLocation, lastKnownLocation);
    
    if (distanceToTarget <= AcceptanceRadius)
    {
        // 이미 마지막 위치에 가까우면 바로 조사 단계로
        TaskMemory->CurrentPhase = EInvestigationPhase::InvestigatingAtLocation;
        UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 이미 목표 위치에 가까움, 즉시 조사 시작"));
    }
    else
    {
        // 마지막 위치로 이동 시작
        TaskMemory->CurrentPhase = EInvestigationPhase::MovingToLastKnownLocation;
        UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 마지막 위치로 이동 시작 - Distance: %.1f"), distanceToTarget);
    }
    
    // === 메모리에 정보 저장 ===
    TaskMemory->StartTime = OwnerComp.GetWorld()->GetTimeSeconds();
    TaskMemory->CurrentPhaseStartTime = TaskMemory->StartTime;
    TaskMemory->LastKnownLocation = lastKnownLocation;
    TaskMemory->CurrentTargetLocation = lastKnownLocation;
    
    // === 수색 포인트들 생성 ===
    TaskMemory->SearchPoints = GenerateSearchPoints(lastKnownLocation, OwnerComp);
    
    // === 캐릭터 이동 속도 설정 ===
    if (ACharacter* Character = Cast<ACharacter>(ControlledPawn))
    {
        if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
        {
            MovementComp->MaxWalkSpeed = MovementSpeed;
        }
    }
    
    // === 경계 상태 설정 ===
    if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
    {
        BB->SetValueAsBool(IsAlertKey.SelectedKeyName, true);
    }
    
    return EBTNodeResult::InProgress;
}

void UInvestigateLastKnownPositionTask::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // === 메모리 및 컴포넌트 확인 ===
    FInvestigateLastKnownPositionTaskMemory* TaskMemory = reinterpret_cast<FInvestigateLastKnownPositionTaskMemory*>(NodeMemory);
    
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
    
    float currentTime = OwnerComp.GetWorld()->GetTimeSeconds();
    float elapsedTime = currentTime - TaskMemory->StartTime;
    float phaseElapsedTime = currentTime - TaskMemory->CurrentPhaseStartTime;
    
    // === 최대 실행 시간 체크 ===
    if (elapsedTime >= MaxExecutionTime)
    {
        UE_LOG(LogTemp, Warning, TEXT("InvestigateLastKnownPositionTask: 최대 실행 시간 초과 (%.2fs)"), elapsedTime);
        if (bClearAlertOnFailure)
        {
            if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
            {
                BB->SetValueAsBool(IsAlertKey.SelectedKeyName, false);
            }
        }
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    // === 타겟 재발견 확인 ===
    if (bStopOnTargetFound && HasCurrentTarget(OwnerComp))
    {
        TaskMemory->bTargetRediscovered = true;
        UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 타겟 재발견으로 조사 중단"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // === 현재 단계에 따른 처리 ===
    switch (TaskMemory->CurrentPhase)
    {
        case EInvestigationPhase::MovingToLastKnownLocation:
            HandleMovingToLastKnownLocation(OwnerComp, TaskMemory, currentTime);
            break;
            
        case EInvestigationPhase::InvestigatingAtLocation:
            HandleInvestigatingAtLocation(OwnerComp, TaskMemory, currentTime, phaseElapsedTime);
            break;
            
        case EInvestigationPhase::MovingToSearchPoint:
            HandleMovingToSearchPoint(OwnerComp, TaskMemory, currentTime);
            break;
            
        case EInvestigationPhase::SearchingAtPoint:
            HandleSearchingAtPoint(OwnerComp, TaskMemory, currentTime, phaseElapsedTime);
            break;
            
        case EInvestigationPhase::Completed:
            UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 조사 완료"));
            if (bClearAlertOnFailure)
            {
                if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
                {
                    BB->SetValueAsBool(IsAlertKey.SelectedKeyName, false);
                }
            }
            FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
            return;
    }
    
    // === 디버그 정보 표시 ===
    #if WITH_EDITOR
    if (GEngine && GEngine->bEnableOnScreenDebugMessages)
    {
        FString phaseText;
        switch (TaskMemory->CurrentPhase)
        {
            case EInvestigationPhase::MovingToLastKnownLocation:
                phaseText = TEXT("Moving to Last Known Location");
                break;
            case EInvestigationPhase::InvestigatingAtLocation:
                phaseText = TEXT("Investigating at Location");
                break;
            case EInvestigationPhase::MovingToSearchPoint:
                phaseText = FString::Printf(TEXT("Moving to Search Point %d/%d"), 
                    TaskMemory->CurrentSearchPointIndex + 1, TaskMemory->SearchPoints.Num());
                break;
            case EInvestigationPhase::SearchingAtPoint:
                phaseText = FString::Printf(TEXT("Searching at Point %d/%d"), 
                    TaskMemory->CurrentSearchPointIndex + 1, TaskMemory->SearchPoints.Num());
                break;
            default:
                phaseText = TEXT("Unknown Phase");
                break;
        }
        
        FString debugText = FString::Printf(TEXT("Investigation: %s (%.1fs)"), *phaseText, elapsedTime);
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, debugText);
        
        // 수색 포인트들 시각화
        UWorld* World = OwnerComp.GetWorld();
        DrawDebugSphere(World, TaskMemory->LastKnownLocation, 100.0f, 8, FColor::Red, false, 0.1f);
        
        for (int32 i = 0; i < TaskMemory->SearchPoints.Num(); i++)
        {
            FColor pointColor = (i == TaskMemory->CurrentSearchPointIndex) ? FColor::Green : FColor::Blue;
            DrawDebugSphere(World, TaskMemory->SearchPoints[i], 50.0f, 8, pointColor, false, 0.1f);
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
    
    UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 종료 - Result: %d"), (int32)TaskResult);
    
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

// === 내부 헬퍼 함수들 구현 ===

bool UInvestigateLastKnownPositionTask::GetLastKnownLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const
{
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return false;
    }
    
    FVector lastKnownLocation = BlackboardComp->GetValueAsVector(LastKnownLocationKey.SelectedKeyName);
    if (lastKnownLocation.IsZero())
    {
        return false;
    }
    
    OutLocation = lastKnownLocation;
    return true;
}

TArray<FVector> UInvestigateLastKnownPositionTask::GenerateSearchPoints(const FVector& CenterLocation, UBehaviorTreeComponent& OwnerComp) const
{
    TArray<FVector> searchPoints;
    
    if (AdditionalSearchPoints <= 0)
    {
        return searchPoints;
    }
    
    // === 중심점 주변에 원형으로 수색 포인트 배치 ===
    float angleStep = 360.0f / AdditionalSearchPoints;
    
    for (int32 i = 0; i < AdditionalSearchPoints; i++)
    {
        float angle = angleStep * i;
        float radians = FMath::DegreesToRadians(angle);
        
        FVector offset = FVector(
            FMath::Cos(radians) * SearchRadius,
            FMath::Sin(radians) * SearchRadius,
            0.0f
        );
        
        FVector searchPoint = CenterLocation + offset;
        
        // 네비게이션 가능한 위치로 조정
        FVector navigablePoint = FindNavigablePosition(searchPoint, OwnerComp);
        if (!navigablePoint.IsZero())
        {
            searchPoints.Add(navigablePoint);
        }
    }
    
    return searchPoints;
}

FVector UInvestigateLastKnownPositionTask::FindNavigablePosition(const FVector& DesiredPosition, UBehaviorTreeComponent& OwnerComp) const
{
    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(OwnerComp.GetWorld());
    if (!NavSystem)
    {
        return DesiredPosition;
    }
    
    FNavLocation NavLocation;
    
    // === 원하는 위치에서 네비게이션 가능한 위치 찾기 ===
    if (NavSystem->ProjectPointToNavigation(DesiredPosition, NavLocation, FVector(200.0f, 200.0f, 200.0f)))
    {
        return NavLocation.Location;
    }
    
    // === 주변에서 네비게이션 가능한 위치 찾기 ===
    if (NavSystem->GetRandomReachablePointInRadius(DesiredPosition, 300.0f, NavLocation))
    {
        return NavLocation.Location;
    }
    
    return FVector::ZeroVector;
}

bool UInvestigateLastKnownPositionTask::HasCurrentTarget(UBehaviorTreeComponent& OwnerComp) const
{
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return false;
    }
    
    AActor* currentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
    return currentTarget != nullptr && IsValid(currentTarget);
}

bool UInvestigateLastKnownPositionTask::MoveToLocation(UBehaviorTreeComponent& OwnerComp, const FVector& TargetLocation) const
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return false;
    }
    
    EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
        TargetLocation,
        AcceptanceRadius,
        true,   // bStopOnOverlap
        true,   // bUsePathfinding
        true,   // bProjectDestinationToNavigation
        true,   // bCanStrafe
        nullptr, // FilterClass
        true    // bAllowPartialPath
    );
    
    return MoveResult == EPathFollowingRequestResult::RequestSuccessful;
}

bool UInvestigateLastKnownPositionTask::HasReachedDestination(UBehaviorTreeComponent& OwnerComp, const FVector& TargetLocation) const
{
    APawn* ControlledPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (!ControlledPawn)
    {
        return false;
    }
    
    float distance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetLocation);
    return distance <= AcceptanceRadius;
}

AEnemy* UInvestigateLastKnownPositionTask::GetControlledEnemy(UBehaviorTreeComponent& OwnerComp) const
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return nullptr;
    }
    
    return Cast<AEnemy>(AIController->GetPawn());
}

// === 단계별 처리 함수들 ===

void UInvestigateLastKnownPositionTask::HandleSearchingAtPoint(UBehaviorTreeComponent& OwnerComp, FInvestigateLastKnownPositionTaskMemory* TaskMemory, float CurrentTime, float PhaseElapsedTime)
{
    // === 수색 시간 체크 ===
    if (PhaseElapsedTime >= WaitTimeAtSearchPoint)
    {
        // 다음 수색 포인트로 이동
        TaskMemory->CurrentSearchPointIndex++;
        
        if (TaskMemory->CurrentSearchPointIndex >= TaskMemory->SearchPoints.Num())
        {
            // 모든 수색 포인트 완료
            TaskMemory->CurrentPhase = EInvestigationPhase::Completed;
            UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 모든 수색 포인트 완료"));
        }
        else
        {
            // 다음 수색 포인트로 이동
            TaskMemory->CurrentPhase = EInvestigationPhase::MovingToSearchPoint;
            TaskMemory->CurrentPhaseStartTime = CurrentTime;
            TaskMemory->CurrentTargetLocation = TaskMemory->SearchPoints[TaskMemory->CurrentSearchPointIndex];
            TaskMemory->bIsMoving = false;
            
            UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 다음 수색 포인트로 이동"));
        }
    }
}

void UInvestigateLastKnownPositionTask::HandleMovingToLastKnownLocation(UBehaviorTreeComponent& OwnerComp, FInvestigateLastKnownPositionTaskMemory* TaskMemory, float CurrentTime)
{
    // === 목적지 도달 확인 ===
    if (HasReachedDestination(OwnerComp, TaskMemory->LastKnownLocation))
    {
        // 조사 단계로 전환
        TaskMemory->CurrentPhase = EInvestigationPhase::InvestigatingAtLocation;
        TaskMemory->CurrentPhaseStartTime = CurrentTime;
        TaskMemory->bIsMoving = false;
        
        UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 마지막 위치 도달, 조사 시작"));
    }
    else if (!TaskMemory->bIsMoving)
    {
        // 이동 시작
        if (MoveToLocation(OwnerComp, TaskMemory->LastKnownLocation))
        {
            TaskMemory->bIsMoving = true;
            UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 마지막 위치로 이동 시작"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("InvestigateLastKnownPositionTask: 마지막 위치로 이동 실패"));
            TaskMemory->CurrentPhase = EInvestigationPhase::Completed;
        }
    }
}

void UInvestigateLastKnownPositionTask::HandleInvestigatingAtLocation(UBehaviorTreeComponent& OwnerComp, FInvestigateLastKnownPositionTaskMemory* TaskMemory, float CurrentTime, float PhaseElapsedTime)
{
    // === 조사 시간 체크 ===
    if (PhaseElapsedTime >= WaitTimeAtSearchPoint)
    {
        // 추가 수색 포인트가 있는지 확인
        if (TaskMemory->SearchPoints.Num() > 0)
        {
            // 첫 번째 수색 포인트로 이동 시작
            TaskMemory->CurrentPhase = EInvestigationPhase::MovingToSearchPoint;
            TaskMemory->CurrentPhaseStartTime = CurrentTime;
            TaskMemory->CurrentSearchPointIndex = 0;
            TaskMemory->CurrentTargetLocation = TaskMemory->SearchPoints[0];
            TaskMemory->bIsMoving = false;
            
            UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 수색 포인트로 이동 시작"));
        }
        else
        {
            // 수색 포인트가 없으면 조사 완료
            TaskMemory->CurrentPhase = EInvestigationPhase::Completed;
            UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 추가 수색 포인트 없음, 조사 완료"));
        }
    }
}

void UInvestigateLastKnownPositionTask::HandleMovingToSearchPoint(UBehaviorTreeComponent& OwnerComp, FInvestigateLastKnownPositionTaskMemory* TaskMemory, float CurrentTime)
{
    // === 수색 포인트 도달 확인 ===
    if (HasReachedDestination(OwnerComp, TaskMemory->CurrentTargetLocation))
    {
        // 해당 포인트에서 수색 시작
        TaskMemory->CurrentPhase = EInvestigationPhase::SearchingAtPoint;
        TaskMemory->CurrentPhaseStartTime = CurrentTime;
        TaskMemory->bIsMoving = false;
        
        UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 수색 포인트 %d 도달, 수색 시작"), 
            TaskMemory->CurrentSearchPointIndex + 1);
    }
    else if (!TaskMemory->bIsMoving)
    {
        // 수색 포인트로 이동 시작
        if (MoveToLocation(OwnerComp, TaskMemory->CurrentTargetLocation))
        {
            TaskMemory->bIsMoving = true;
            UE_LOG(LogTemp, Log, TEXT("InvestigateLastKnownPositionTask: 수색 포인트 %d로 이동 시작"), 
                TaskMemory->CurrentSearchPointIndex + 1);
        }
        else
        {
            // 이동 실패 시 다음 포인트로
            TaskMemory->CurrentSearchPointIndex++;
            if (TaskMemory->CurrentSearchPointIndex >= TaskMemory->SearchPoints.Num())
            {
                TaskMemory->CurrentPhase = EInvestigationPhase::Completed;
            }
            else
            {
                TaskMemory->CurrentTargetLocation = TaskMemory->SearchPoints[TaskMemory->CurrentSearchPointIndex];
            }
        }
    }
}