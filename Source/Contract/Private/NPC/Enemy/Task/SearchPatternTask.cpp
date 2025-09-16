// Fill out your copyright notice in the Description page of Project Settings.

#include "SearchPatternTask.h"
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

// Enemy 클래스 포함
#include "Enemy.h"

USearchPatternTask::USearchPatternTask()
{
    // 노드 이름 설정
    NodeName = TEXT("Search Pattern");
    
    // 이 태스크는 Tick을 사용함
    bNotifyTick = true;
    
    // 인스턴스별로 실행됨
    bCreateNodeInstance = true;
    
    // === 블랙보드 키 기본값 설정 ===
    SearchCenterKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(USearchPatternTask, SearchCenterKey));
    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USearchPatternTask, TargetActorKey), AActor::StaticClass());
    IsAlertKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(USearchPatternTask, IsAlertKey));
    
    // === 기본값 설정 ===
    SearchPattern = ESearchPattern::Circular;
    SearchRadius = 800.0f;
    SearchPointCount = 8;
    WaitTimeAtPoint = 3.0f;
    AcceptanceRadius = 100.0f;
    MaxSearchTime = 60.0f;
    MovementSpeed = 350.0f;
    GridSize = 3;
    SpiralTurns = 2;
    LinearLines = 3;
    bStopOnTargetFound = true;
    bClearAlertOnFailure = true;
    bRandomizeOrder = false;
    bReturnToStart = false;
}

EBTNodeResult::Type USearchPatternTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // === 메모리 초기화 ===
    FSearchPatternTaskMemory* TaskMemory = reinterpret_cast<FSearchPatternTaskMemory*>(NodeMemory);
    TaskMemory->Initialize();
    
    // === 필수 컴포넌트 확인 ===
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogTemp, Warning, TEXT("SearchPatternTask: AI Controller가 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("SearchPatternTask: 제어할 Pawn이 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("SearchPatternTask: Blackboard Component가 없습니다!"));
        return EBTNodeResult::Failed;
    }
    
    // === 현재 타겟이 있는지 확인 ===
    if (HasCurrentTarget(OwnerComp))
    {
        UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 현재 타겟이 있어서 검색이 필요하지 않습니다."));
        return EBTNodeResult::Failed;
    }
    
    // === 검색 중심점 가져오기 ===
    FVector searchCenter;
    if (!GetSearchCenter(OwnerComp, searchCenter))
    {
        // 중심점이 없으면 현재 위치를 사용
        searchCenter = ControlledPawn->GetActorLocation();
        UE_LOG(LogTemp, Warning, TEXT("SearchPatternTask: 검색 중심점이 없어서 현재 위치 사용"));
    }
    
    // === 메모리에 정보 저장 ===
    TaskMemory->StartTime = OwnerComp.GetWorld()->GetTimeSeconds();
    TaskMemory->CurrentPhaseStartTime = TaskMemory->StartTime;
    TaskMemory->SearchCenter = searchCenter;
    TaskMemory->StartLocation = ControlledPawn->GetActorLocation();
    TaskMemory->CurrentPhase = ESearchPhase::MovingToSearchPoint;
    
    // === 검색 포인트들 생성 ===
    TaskMemory->SearchPoints = GenerateSearchPoints(searchCenter, OwnerComp);
    
    if (TaskMemory->SearchPoints.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SearchPatternTask: 검색 포인트를 생성하지 못했습니다!"));
        return EBTNodeResult::Failed;
    }
    
    // === 포인트 순서 무작위화 ===
    if (bRandomizeOrder)
    {
        for (int32 i = TaskMemory->SearchPoints.Num() - 1; i > 0; i--)
        {
            int32 j = FMath::RandRange(0, i);
            TaskMemory->SearchPoints.Swap(i, j);
        }
        UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 검색 포인트 순서 무작위화"));
    }
    
    // === 첫 번째 검색 포인트 설정 ===
    TaskMemory->CurrentTargetLocation = TaskMemory->SearchPoints[0];
    TaskMemory->CurrentSearchIndex = 0;
    
    // === 캐릭터 이동 속도 설정 ===
    if (ACharacter* Character = Cast<ACharacter>(ControlledPawn))
    {
        if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
        {
            MovementComp->MaxWalkSpeed = MovementSpeed;
        }
    }
    
    // === 경계 상태 설정 ===
    BlackboardComp->SetValueAsBool(IsAlertKey.SelectedKeyName, true);
    
    UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 검색 시작 - Pattern: %d, Points: %d"), 
        (int32)SearchPattern, TaskMemory->SearchPoints.Num());
    
    return EBTNodeResult::InProgress;
}

void USearchPatternTask::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // === 메모리 및 컴포넌트 확인 ===
    FSearchPatternTaskMemory* TaskMemory = reinterpret_cast<FSearchPatternTaskMemory*>(NodeMemory);
    
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
    
    // === 최대 검색 시간 체크 ===
    if (elapsedTime >= MaxSearchTime)
    {
        UE_LOG(LogTemp, Warning, TEXT("SearchPatternTask: 최대 검색 시간 초과 (%.2fs)"), elapsedTime);
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
        TaskMemory->bTargetFound = true;
        UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 타겟 재발견으로 검색 중단"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }
    
    // === 현재 단계에 따른 처리 ===
    switch (TaskMemory->CurrentPhase)
    {
        case ESearchPhase::MovingToSearchPoint:
            HandleMovingToSearchPoint(OwnerComp, TaskMemory, currentTime);
            break;
            
        case ESearchPhase::SearchingAtPoint:
            HandleSearchingAtPoint(OwnerComp, TaskMemory, currentTime, phaseElapsedTime);
            break;
            
        case ESearchPhase::ReturningToStart:
            HandleReturningToStart(OwnerComp, TaskMemory, currentTime);
            break;
            
        case ESearchPhase::Completed:
            UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 검색 완료 - Points Searched: %d"), TaskMemory->PointsSearched);
            if (bClearAlertOnFailure && !TaskMemory->bTargetFound)
            {
                if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
                {
                    BB->SetValueAsBool(IsAlertKey.SelectedKeyName, false);
                }
            }
            FinishLatentTask(OwnerComp, TaskMemory->bTargetFound ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
            return;
    }
    
    // === 디버그 정보 표시 ===
    #if WITH_EDITOR
    if (GEngine && GEngine->bEnableOnScreenDebugMessages)
    {
        FString phaseText;
        switch (TaskMemory->CurrentPhase)
        {
            case ESearchPhase::MovingToSearchPoint:
                phaseText = FString::Printf(TEXT("Moving to Point %d/%d"), 
                    TaskMemory->CurrentSearchIndex + 1, TaskMemory->SearchPoints.Num());
                break;
            case ESearchPhase::SearchingAtPoint:
                phaseText = FString::Printf(TEXT("Searching at Point %d/%d"), 
                    TaskMemory->CurrentSearchIndex + 1, TaskMemory->SearchPoints.Num());
                break;
            case ESearchPhase::ReturningToStart:
                phaseText = TEXT("Returning to Start");
                break;
            default:
                phaseText = TEXT("Unknown Phase");
                break;
        }
        
        FString debugText = FString::Printf(TEXT("Search Pattern: %s (%.1fs)"), *phaseText, elapsedTime);
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan, debugText);
        
        // 검색 포인트들 시각화
        UWorld* World = OwnerComp.GetWorld();
        DrawDebugSphere(World, TaskMemory->SearchCenter, 100.0f, 8, FColor::Red, false, 0.1f);
        
        for (int32 i = 0; i < TaskMemory->SearchPoints.Num(); i++)
        {
            FColor pointColor;
            if (i < TaskMemory->CurrentSearchIndex)
            {
                pointColor = FColor::FColor(128, 128, 128); // 이미 검색한 포인트
            }
            else if (i == TaskMemory->CurrentSearchIndex)
            {
                pointColor = FColor::Green; // 현재 검색 중인 포인트
            }
            else
            {
                pointColor = FColor::Blue; // 아직 검색하지 않은 포인트
            }
            
            DrawDebugSphere(World, TaskMemory->SearchPoints[i], 50.0f, 8, pointColor, false, 0.1f);
            DrawDebugString(World, TaskMemory->SearchPoints[i] + FVector(0, 0, 100), 
                FString::Printf(TEXT("%d"), i + 1), nullptr, pointColor, 0.1f);
        }
    }
    #endif
}

uint16 USearchPatternTask::GetInstanceMemorySize() const
{
    return sizeof(FSearchPatternTaskMemory);
}

void USearchPatternTask::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // === 이동 중단 ===
    if (AAIController* AIController = OwnerComp.GetAIOwner())
    {
        AIController->StopMovement();
    }
    
    // === 메모리 정리 ===
    FSearchPatternTaskMemory* TaskMemory = reinterpret_cast<FSearchPatternTaskMemory*>(NodeMemory);
    TaskMemory->bIsMoving = false;
    
    UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 종료 - Result: %d, Points Searched: %d"), 
        (int32)TaskResult, TaskMemory->PointsSearched);
    
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

// === 내부 헬퍼 함수들 구현 ===

bool USearchPatternTask::GetSearchCenter(UBehaviorTreeComponent& OwnerComp, FVector& OutCenter) const
{
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return false;
    }
    
    FVector searchCenter = BlackboardComp->GetValueAsVector(SearchCenterKey.SelectedKeyName);
    if (searchCenter.IsZero())
    {
        return false;
    }
    
    OutCenter = searchCenter;
    return true;
}

TArray<FVector> USearchPatternTask::GenerateSearchPoints(const FVector& CenterLocation, UBehaviorTreeComponent& OwnerComp) const
{
    TArray<FVector> searchPoints;
    
    switch (SearchPattern)
    {
        case ESearchPattern::Circular:
            searchPoints = GenerateCircularPoints(CenterLocation);
            break;
            
        case ESearchPattern::Grid:
            searchPoints = GenerateGridPoints(CenterLocation);
            break;
            
        case ESearchPattern::Spiral:
            searchPoints = GenerateSpiralPoints(CenterLocation);
            break;
            
        case ESearchPattern::Linear:
            searchPoints = GenerateLinearPoints(CenterLocation);
            break;
            
        case ESearchPattern::Random:
            searchPoints = GenerateRandomPoints(CenterLocation, OwnerComp);
            break;
            
        case ESearchPattern::Custom:
            searchPoints = GenerateCustomPoints(CenterLocation);
            break;
            
        default:
            searchPoints = GenerateCircularPoints(CenterLocation);
            break;
    }
    
    // === 모든 포인트를 네비게이션 가능한 위치로 조정 ===
    for (int32 i = 0; i < searchPoints.Num(); i++)
    {
        FVector navigablePoint = FindNavigablePosition(searchPoints[i], OwnerComp);
        if (!navigablePoint.IsZero())
        {
            searchPoints[i] = navigablePoint;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: %d개의 검색 포인트 생성"), searchPoints.Num());
    
    return searchPoints;
}

TArray<FVector> USearchPatternTask::GenerateCircularPoints(const FVector& Center) const
{
    TArray<FVector> points;
    
    float angleStep = 360.0f / SearchPointCount;
    
    for (int32 i = 0; i < SearchPointCount; i++)
    {
        float angle = angleStep * i;
        float radians = FMath::DegreesToRadians(angle);
        
        FVector offset = FVector(
            FMath::Cos(radians) * SearchRadius,
            FMath::Sin(radians) * SearchRadius,
            0.0f
        );
        
        points.Add(Center + offset);
    }
    
    return points;
}

TArray<FVector> USearchPatternTask::GenerateGridPoints(const FVector& Center) const
{
    TArray<FVector> points;
    
    float spacing = (SearchRadius * 2.0f) / (GridSize - 1);
    float startOffset = -SearchRadius;
    
    for (int32 x = 0; x < GridSize; x++)
    {
        for (int32 y = 0; y < GridSize; y++)
        {
            FVector offset = FVector(
                startOffset + (spacing * x),
                startOffset + (spacing * y),
                0.0f
            );
            
            points.Add(Center + offset);
        }
    }
    
    return points;
}

TArray<FVector> USearchPatternTask::GenerateSpiralPoints(const FVector& Center) const
{
    TArray<FVector> points;
    
    float totalAngle = 360.0f * SpiralTurns;
    float angleStep = totalAngle / SearchPointCount;
    
    for (int32 i = 0; i < SearchPointCount; i++)
    {
        float angle = angleStep * i;
        float radians = FMath::DegreesToRadians(angle);
        float radius = SearchRadius * (float(i) / float(SearchPointCount - 1));
        
        FVector offset = FVector(
            FMath::Cos(radians) * radius,
            FMath::Sin(radians) * radius,
            0.0f
        );
        
        points.Add(Center + offset);
    }
    
    return points;
}

TArray<FVector> USearchPatternTask::GenerateLinearPoints(const FVector& Center) const
{
    TArray<FVector> points;
    
    float lineSpacing = (SearchRadius * 2.0f) / (LinearLines - 1);
    float startY = -SearchRadius;
    
    for (int32 lineIndex = 0; lineIndex < LinearLines; lineIndex++)
    {
        float yOffset = startY + (lineSpacing * lineIndex);
        bool isEvenLine = (lineIndex % 2 == 0);
        
        // 각 라인에서 포인트들 생성
        int32 pointsPerLine = FMath::Max(2, SearchPointCount / LinearLines);
        float xSpacing = (SearchRadius * 2.0f) / (pointsPerLine - 1);
        
        for (int32 pointIndex = 0; pointIndex < pointsPerLine; pointIndex++)
        {
            float xOffset;
            if (isEvenLine)
            {
                // 짝수 라인: 왼쪽에서 오른쪽
                xOffset = -SearchRadius + (xSpacing * pointIndex);
            }
            else
            {
                // 홀수 라인: 오른쪽에서 왼쪽
                xOffset = SearchRadius - (xSpacing * pointIndex);
            }
            
            FVector offset = FVector(xOffset, yOffset, 0.0f);
            points.Add(Center + offset);
        }
    }
    
    return points;
}

TArray<FVector> USearchPatternTask::GenerateRandomPoints(const FVector& Center, UBehaviorTreeComponent& OwnerComp) const
{
    TArray<FVector> points;
    
    for (int32 i = 0; i < SearchPointCount; i++)
    {
        // 원형 영역 내에서 랜덤 포인트 생성
        float angle = FMath::RandRange(0.0f, 360.0f);
        float radius = FMath::RandRange(SearchRadius * 0.3f, SearchRadius);
        float radians = FMath::DegreesToRadians(angle);
        
        FVector offset = FVector(
            FMath::Cos(radians) * radius,
            FMath::Sin(radians) * radius,
            0.0f
        );
        
        points.Add(Center + offset);
    }
    
    return points;
}

TArray<FVector> USearchPatternTask::GenerateCustomPoints(const FVector& Center) const
{
    TArray<FVector> points;
    
    for (const FVector& relativePoint : CustomSearchPoints)
    {
        // 상대적 위치를 절대 위치로 변환
        points.Add(Center + relativePoint);
    }
    
    return points;
}

FVector USearchPatternTask::FindNavigablePosition(const FVector& DesiredPosition, UBehaviorTreeComponent& OwnerComp) const
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

bool USearchPatternTask::HasCurrentTarget(UBehaviorTreeComponent& OwnerComp) const
{
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return false;
    }
    
    AActor* currentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
    return currentTarget != nullptr && IsValid(currentTarget);
}

bool USearchPatternTask::MoveToLocation(UBehaviorTreeComponent& OwnerComp, const FVector& TargetLocation) const
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return false;
    }
    
    AIController->MoveToLocation(TargetLocation, AcceptanceRadius);
    return true;
}

bool USearchPatternTask::HasReachedDestination(UBehaviorTreeComponent& OwnerComp, const FVector& TargetLocation) const
{
    APawn* ControlledPawn = OwnerComp.GetAIOwner()->GetPawn();
    if (!ControlledPawn)
    {
        return false;
    }
    
    float distance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetLocation);
    return distance <= AcceptanceRadius;
}

AEnemy* USearchPatternTask::GetControlledEnemy(UBehaviorTreeComponent& OwnerComp) const
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return nullptr;
    }
    
    return Cast<AEnemy>(AIController->GetPawn());
}

bool USearchPatternTask::ScanForTargetInArea(UBehaviorTreeComponent& OwnerComp, const FVector& SearchLocation) const
{
    // 실제 게임에서는 여기에 시야각 체크, 라인 트레이스 등을 구현
    // 현재는 블랙보드의 타겟 정보만 확인
    return HasCurrentTarget(OwnerComp);
}

// === 단계별 처리 함수들 ===

void USearchPatternTask::HandleMovingToSearchPoint(UBehaviorTreeComponent& OwnerComp, FSearchPatternTaskMemory* TaskMemory, float CurrentTime)
{
    // === 목적지 도달 확인 ===
    if (HasReachedDestination(OwnerComp, TaskMemory->CurrentTargetLocation))
    {
        // 검색 단계로 전환
        TaskMemory->CurrentPhase = ESearchPhase::SearchingAtPoint;
        TaskMemory->CurrentPhaseStartTime = CurrentTime;
        TaskMemory->bIsMoving = false;
        
        UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 검색 포인트 %d 도달, 검색 시작"), 
            TaskMemory->CurrentSearchIndex + 1);
    }
    else if (!TaskMemory->bIsMoving)
    {
        // 이동 시작
        if (MoveToLocation(OwnerComp, TaskMemory->CurrentTargetLocation))
        {
            TaskMemory->bIsMoving = true;
            UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 검색 포인트 %d로 이동 시작"), 
                TaskMemory->CurrentSearchIndex + 1);
        }
        else
        {
            // 이동 실패 시 다음 포인트로
            TaskMemory->CurrentSearchIndex++;
            if (TaskMemory->CurrentSearchIndex >= TaskMemory->SearchPoints.Num())
            {
                // 모든 포인트 완료
                if (bReturnToStart)
                {
                    TaskMemory->CurrentPhase = ESearchPhase::ReturningToStart;
                    TaskMemory->CurrentTargetLocation = TaskMemory->StartLocation;
                }
                else
                {
                    TaskMemory->CurrentPhase = ESearchPhase::Completed;
                }
            }
            else
            {
                TaskMemory->CurrentTargetLocation = TaskMemory->SearchPoints[TaskMemory->CurrentSearchIndex];
            }
        }
    }
}

void USearchPatternTask::HandleSearchingAtPoint(UBehaviorTreeComponent& OwnerComp, FSearchPatternTaskMemory* TaskMemory, float CurrentTime, float PhaseElapsedTime)
{
    // === 타겟 스캔 ===
    if (ScanForTargetInArea(OwnerComp, TaskMemory->CurrentTargetLocation))
    {
        TaskMemory->bTargetFound = true;
        TaskMemory->CurrentPhase = ESearchPhase::Completed;
        UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 타겟 발견!"));
        return;
    }
    
    // === 검색 시간 체크 ===
    if (PhaseElapsedTime >= WaitTimeAtPoint)
    {
        TaskMemory->PointsSearched++;
        TaskMemory->CurrentSearchIndex++;
        
        if (TaskMemory->CurrentSearchIndex >= TaskMemory->SearchPoints.Num())
        {
            // 모든 포인트 검색 완료
            if (bReturnToStart)
            {
                TaskMemory->CurrentPhase = ESearchPhase::ReturningToStart;
                TaskMemory->CurrentPhaseStartTime = CurrentTime;
                TaskMemory->CurrentTargetLocation = TaskMemory->StartLocation;
                TaskMemory->bIsMoving = false;
                UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 모든 포인트 검색 완료, 시작점으로 복귀"));
            }
            else
            {
                TaskMemory->CurrentPhase = ESearchPhase::Completed;
                UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 모든 포인트 검색 완료"));
            }
        }
        else
        {
            // 다음 검색 포인트로 이동
            TaskMemory->CurrentPhase = ESearchPhase::MovingToSearchPoint;
            TaskMemory->CurrentPhaseStartTime = CurrentTime;
            TaskMemory->CurrentTargetLocation = TaskMemory->SearchPoints[TaskMemory->CurrentSearchIndex];
            TaskMemory->bIsMoving = false;
            
            UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 다음 검색 포인트로 이동"));
        }
    }
}

void USearchPatternTask::HandleReturningToStart(UBehaviorTreeComponent& OwnerComp, FSearchPatternTaskMemory* TaskMemory, float CurrentTime)
{
    // === 시작점 도달 확인 ===
    if (HasReachedDestination(OwnerComp, TaskMemory->StartLocation))
    {
        TaskMemory->CurrentPhase = ESearchPhase::Completed;
        TaskMemory->bIsMoving = false;
        
        UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 시작점 복귀 완료"));
    }
    else if (!TaskMemory->bIsMoving)
    {
        // 시작점으로 이동 시작
        if (MoveToLocation(OwnerComp, TaskMemory->StartLocation))
        {
            TaskMemory->bIsMoving = true;
            UE_LOG(LogTemp, Log, TEXT("SearchPatternTask: 시작점으로 복귀 시작"));
        }
        else
        {
            // 이동 실패 시 그냥 완료
            TaskMemory->CurrentPhase = ESearchPhase::Completed;
        }
    }
}

