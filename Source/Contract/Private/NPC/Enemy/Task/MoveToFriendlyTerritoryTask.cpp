#include "MoveToFriendlyTerritoryTask.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
// AI 이동 관련 헤더 추가
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"

// 🔧 TActorIterator를 위한 헤더 추가
#include "EngineUtils.h"

// Enemy 클래스 포함
#include "Enemy.h"
// OccupiedTerritory 헤더 추가
#include "OccupiedTerritory.h"

UMoveToFriendlyTerritoryTask::UMoveToFriendlyTerritoryTask()
{
    // 노드 이름 설정
    NodeName = TEXT("Move to Friendly Territory");
    
    // 이 태스크는 Tick을 사용함
    bNotifyTick = true;
    
    // 인스턴스별로 실행됨
    bCreateNodeInstance = true;
    
    // === 블랙보드 키 기본값 설정 ===
    customLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UMoveToFriendlyTerritoryTask, customLocationKey));
    targetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UMoveToFriendlyTerritoryTask, targetActorKey), AActor::StaticClass());
    isAlertKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UMoveToFriendlyTerritoryTask, isAlertKey));
    
    // === 기본값 설정 ===
    territoryType = EFriendlyTerritoryType::SpawnPoint;
    shouldAbortOnCombat = true;
    acceptanceRadius = 150.0f;
    maxSearchRadius = 2000.0f;
    minSafeDistance = 800.0f;
    maxMoveTime = 45.0f;
    movementSpeed = 450.0f;
    waitTimeAtTerritory = 5.0f;
    isAllowCombatWhileMoving = false;
    friendlyTerritoryTag = TEXT("FriendlyTerritory");
    safeZoneTag = TEXT("SafeZone");
    coverTag = TEXT("Cover");
    spawnPointTag = TEXT("SpawnPoint");
    patrolPointTag = TEXT("PatrolPoint");
    isClearAlertOnArrival = false;
    isChooseClosest = true;
    isChooseFarthestFromTarget = false;
}

EBTNodeResult::Type UMoveToFriendlyTerritoryTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // === 메모리 초기화 ===
    FMoveToFriendlyTerritoryTaskMemory* taskMemory = reinterpret_cast<FMoveToFriendlyTerritoryTaskMemory*>(NodeMemory);
    taskMemory->Initialize();
    
    // === 필수 컴포넌트 확인 ===
    AAIController* aiController = OwnerComp.GetAIOwner();

    if (!aiController)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullAIController, MoveToFriendlyTerritoryTask.cpp) : AI Controller가 없습니다!"));

        return EBTNodeResult::Failed;
    }
    
    APawn* controlledPawn = aiController->GetPawn();

    if (controlledPawn == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullPawn, MoveToFriendlyTerritoryTask.cpp) : 제어할 Pawn이 없습니다!"));

        return EBTNodeResult::Failed;
    }
    
    UBlackboardComponent* blackboardComp = OwnerComp.GetBlackboardComponent();

    if (blackboardComp == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullBlackboard, MoveToFriendlyTerritoryTask.cpp) : Blackboard Component가 없습니다!"));

        return EBTNodeResult::Failed;
    }
    
    // === 메모리에 정보 저장 ===
    taskMemory->startTime = OwnerComp.GetWorld()->GetTimeSeconds();
    taskMemory->currentPhaseStartTime = taskMemory->startTime;
    taskMemory->startLocation = controlledPawn->GetActorLocation();
    taskMemory->currentPhase = EMoveToTerritoryPhase::FindingTerritory;
    
    // === 캐릭터 이동 속도 설정 ===
    if (ACharacter* Character = Cast<ACharacter>(controlledPawn))
    {
        if (UCharacterMovementComponent* movementComp = Character->GetCharacterMovement())
            movementComp->MaxWalkSpeed = movementSpeed;
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        FString::Printf(TEXT("MoveToFriendlyTerritoryTask: 우호 지역 이동 시작 - Type: %d"), (int32)territoryType));

    return EBTNodeResult::InProgress;
}

void UMoveToFriendlyTerritoryTask::TickTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
    // === 메모리 및 컴포넌트 확인 ===
    FMoveToFriendlyTerritoryTaskMemory* taskMemory = reinterpret_cast<FMoveToFriendlyTerritoryTaskMemory*>(nodeMemory);
    
    AAIController* aiController = ownerComp.GetAIOwner();

    if (aiController == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullAIController, MoveToFriendlyTerritoryTask.cpp) : AIController가 없습니다!"));
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);

        return;
    }
    
    APawn* controlledPawn = aiController->GetPawn();

    if (controlledPawn == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullPawn, MoveToFriendlyTerritoryTask.cpp) : ControlledPawn이 없습니다!"));
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);

        return;
    }
    
    // === 🔧 전투 상태 감지 시 태스크 중단 ===
    if (shouldAbortOnCombat)
    {
        UBlackboardComponent* blackboardComp = ownerComp.GetBlackboardComponent();
        if (blackboardComp)
        {
            bool isInCombat = blackboardComp->GetValueAsBool(TEXT("IsInCombat"));

            if (isInCombat)
            {
                GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
                    TEXT("MoveToFriendlyTerritoryTask: 전투 상태 감지로 인한 태스크 중단!"));
                FinishLatentTask(ownerComp, EBTNodeResult::Aborted);

                return;
            }
        }
    }
    
    float currentTime = ownerComp.GetWorld()->GetTimeSeconds();
    float elapsedTime = currentTime - taskMemory->startTime;
    float phaseElapsedTime = currentTime - taskMemory->currentPhaseStartTime;
    
    // === 최대 이동 시간 체크 ===
    if (elapsedTime >= maxMoveTime)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            FString::Printf(TEXT("Error (MaxMoveTimeExceeded, MoveToFriendlyTerritoryTask.cpp) : 최대 이동 시간 초과 (%.2fs)"), elapsedTime));
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);

        return;
    }
    
    // === 이동 중 전투 처리 ===
    if (isAllowCombatWhileMoving && taskMemory->currentPhase == EMoveToTerritoryPhase::MovingToTerritory)
    {
        HandleCombatWhileMoving(ownerComp, taskMemory);
    }
    
    // === 현재 단계에 따른 처리 ===
    switch (taskMemory->currentPhase)
    {
        case EMoveToTerritoryPhase::FindingTerritory:
            HandleFindingTerritory(ownerComp, taskMemory, currentTime);
            break;
            
        case EMoveToTerritoryPhase::MovingToTerritory:
            HandleMovingToTerritory(ownerComp, taskMemory, currentTime);
            break;
            
        case EMoveToTerritoryPhase::WaitingAtTerritory:
            HandleWaitingAtTerritory(ownerComp, taskMemory, currentTime, phaseElapsedTime);
            break;
            
        case EMoveToTerritoryPhase::Completed:
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                TEXT("MoveToFriendlyTerritoryTask: 우호 지역 이동 완료"));

            if (isClearAlertOnArrival)
            {
                if (UBlackboardComponent* blockboard = ownerComp.GetBlackboardComponent())
                    blockboard->SetValueAsBool(isAlertKey.SelectedKeyName, false);
            }

            FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);
            return;
    }
    
    // === 🔧 추가 안전 체크: Enemy의 전투 상태 직접 확인 ===
    if (shouldAbortOnCombat)
    {
        if (AEnemy* enemy = GetControlledEnemy(ownerComp))
        {
            if (enemy->IsInCombat())
            {
                GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
                    TEXT("MoveToFriendlyTerritoryTask: Enemy 전투 상태 감지로 인한 태스크 중단!"));
                FinishLatentTask(ownerComp, EBTNodeResult::Aborted);
                return;
            }
        }
    }
    
    // === 디버그 정보 표시 ===
    #if WITH_EDITOR
    if (GEngine && GEngine->bEnableOnScreenDebugMessages)
    {
        FString phaseText;

        switch (taskMemory->currentPhase)
        {
            case EMoveToTerritoryPhase::FindingTerritory:
                phaseText = TEXT("Finding Territory");
                break;
                
            case EMoveToTerritoryPhase::MovingToTerritory:
                phaseText = TEXT("Moving to Territory");
                break;

            case EMoveToTerritoryPhase::WaitingAtTerritory:
                phaseText = TEXT("Waiting at Territory");
                break;

            default:
                phaseText = TEXT("Unknown Phase");
                break;
        }
        
        FString debugText = FString::Printf(TEXT("Move to Territory: %s (%.1fs)"), *phaseText, elapsedTime);
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, debugText);
        
        // 목표 위치 시각화
        if (!taskMemory->targetTerritoryLocation.IsZero())
        {
            UWorld* world = ownerComp.GetWorld();

            DrawDebugSphere(world, taskMemory->targetTerritoryLocation, acceptanceRadius, 12, FColor::Green, false, 0.1f);
            DrawDebugString(world, taskMemory->targetTerritoryLocation + FVector(0, 0, 200), 
                TEXT("Friendly Territory"), nullptr, FColor::Green, 0.1f);
        }
    }
    #endif
}

uint16 UMoveToFriendlyTerritoryTask::GetInstanceMemorySize() const
{
    return sizeof(FMoveToFriendlyTerritoryTaskMemory);
}

void UMoveToFriendlyTerritoryTask::OnTaskFinished(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, EBTNodeResult::Type taskResult)
{
    // === 이동 중단 ===
    if (AAIController* aiController = ownerComp.GetAIOwner())
        aiController->StopMovement();
    
    // === 메모리 정리 ===
    FMoveToFriendlyTerritoryTaskMemory* TaskMemory = reinterpret_cast<FMoveToFriendlyTerritoryTaskMemory*>(nodeMemory);
    TaskMemory->isMoving = false;
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        FString::Printf(TEXT("MoveToFriendlyTerritoryTask: 종료 - Result: %d"), (int32)taskResult));

    Super::OnTaskFinished(ownerComp, nodeMemory, taskResult);
}

// === 내부 헬퍼 함수들 구현 ===

bool UMoveToFriendlyTerritoryTask::FindTargetTerritory(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const
{
    // 1. 먼저 모든 타입에서 OccupiedTerritory 중 Friendly 영역을 우선 검색
    if (FindFriendlyOccupiedTerritory(ownerComp, outLocation))
    {
        return true;
    }
    
    // 2. OccupiedTerritory에서 찾지 못했다면 기존 방식으로 검색
    switch (territoryType)
    {
        case EFriendlyTerritoryType::SpawnPoint:
            return FindSpawnPoint(ownerComp, outLocation);
            
        case EFriendlyTerritoryType::PatrolPoint:
            return FindPatrolPoint(ownerComp, outLocation);
            
        case EFriendlyTerritoryType::CoverPoint:
            return FindCoverPoint(ownerComp, outLocation);
            
        case EFriendlyTerritoryType::SafeZone:
            return FindSafeZone(ownerComp, outLocation);
            
        case EFriendlyTerritoryType::NearestAlly:
            return FindNearestAlly(ownerComp, outLocation);
            
        case EFriendlyTerritoryType::CustomLocation:
            return GetCustomLocation(ownerComp, outLocation);
            
        default:
            return FindSpawnPoint(ownerComp, outLocation);
    }
}

bool UMoveToFriendlyTerritoryTask::FindSpawnPoint(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const
{
    TArray<AActor*> spawnPoints = FindActorsWithTag(ownerComp, spawnPointTag);

    if (spawnPoints.Num() == 0)
    {
        // 스폰 포인트가 없으면 시작 위치를 사용
        if (AEnemy* Enemy = GetControlledEnemy(ownerComp))
        {
            outLocation = Enemy->GetActorLocation(); // 현재 위치 근처
            return true;
        }

        // 실패 시 오류 메시지 출력
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NoSpawnPointAndNoPawn, MoveToFriendlyTerritoryTask.cpp) : 스폰 포인트와 Pawn 모두 없습니다!"));
        return false;
    }
    
    TArray<FVector> candidates;

    for (AActor* spawnPoint : spawnPoints)
    {
        FVector location = spawnPoint->GetActorLocation();

        if (IsLocationSafe(location, ownerComp))
            candidates.Add(location);
    }
    
    if (candidates.Num() > 0)
    {
        outLocation = SelectBestTerritory(candidates, ownerComp);

        return true;
    }
    
    return false;
}

bool UMoveToFriendlyTerritoryTask::FindPatrolPoint(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const
{
    TArray<AActor*> patrolPoints = FindActorsWithTag(ownerComp, patrolPointTag);

    if (patrolPoints.Num() == 0)
        return false;
    
    TArray<FVector> candidates;

    for (AActor* patrolPoint : patrolPoints)
    {
        FVector location = patrolPoint->GetActorLocation();

        if (IsLocationSafe(location, ownerComp))
            candidates.Add(location);
    }
    
    if (candidates.Num() > 0)
    {
        outLocation = SelectBestTerritory(candidates, ownerComp);

        return true;
    }
    
    return false;
}

bool UMoveToFriendlyTerritoryTask::FindCoverPoint(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const
{
    TArray<AActor*> coverPoints = FindActorsWithTag(ownerComp, coverTag);

    if (coverPoints.Num() == 0)
        return false;
    
    TArray<FVector> candidates;

    for (AActor* coverPoint : coverPoints)
    {
        FVector location = coverPoint->GetActorLocation();

        if (IsLocationSafe(location, ownerComp))
            candidates.Add(location);
    }
    
    if (candidates.Num() > 0)
    {
        outLocation = SelectBestTerritory(candidates, ownerComp);

        return true;
    }
    
    return false;
}

// 새로운 함수 추가 - 우호적인 OccupiedTerritory 찾기
bool UMoveToFriendlyTerritoryTask::FindFriendlyOccupiedTerritory(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const
{
    UWorld* world = ownerComp.GetWorld();
    if (world == nullptr)
        return false;
    
    APawn* controlledPawn = ownerComp.GetAIOwner()->GetPawn();
    if (controlledPawn == nullptr)
        return false;
    
    FVector pawnLocation = controlledPawn->GetActorLocation();
    
    // 모든 OccupiedTerritory 액터 검색
    TArray<AActor*> allOccupiedTerritories;
    UGameplayStatics::GetAllActorsOfClass(world, AOccupiedTerritory::StaticClass(), allOccupiedTerritories);
    
    TArray<FVector> friendlyTerritories;
    
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
        FString::Printf(TEXT("OccupiedTerritory 검색: 총 %d개 발견"), allOccupiedTerritories.Num()));
    
    for (AActor* actor : allOccupiedTerritories)
    {
        AOccupiedTerritory* territory = Cast<AOccupiedTerritory>(actor);
        if (territory && territory->IsFriendlyTerritory())
        {
            float distance = FVector::Dist(pawnLocation, territory->GetActorLocation());
            
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
                FString::Printf(TEXT("Friendly Territory 발견: 거리 %.1f"), distance));
            
            // 검색 반경 제한 없이 모든 Friendly 영역을 후보로 추가
            friendlyTerritories.Add(territory->GetActorLocation());
        }
    }
    
    if (friendlyTerritories.Num() > 0)
    {
        outLocation = SelectBestTerritory(friendlyTerritories, ownerComp);
        
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
            FString::Printf(TEXT("우호적 OccupiedTerritory 발견: %d개 후보 중 선택"), friendlyTerritories.Num()));
        
        return true;
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
        TEXT("우호적 OccupiedTerritory를 찾을 수 없음"));
    
    return false;
}

bool UMoveToFriendlyTerritoryTask::FindSafeZone(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const
{
    // 기존 태그 기반 SafeZone 검색만 수행
    TArray<AActor*> safeZones = FindActorsWithTag(ownerComp, safeZoneTag);
    
    if (safeZones.Num() == 0)
        return false;
    
    TArray<FVector> candidates;
    
    for (AActor* safeZone : safeZones)
    {
        FVector location = safeZone->GetActorLocation();
        candidates.Add(location); // 안전 지역은 기본적으로 안전함
    }
    
    if (candidates.Num() > 0)
    {
        outLocation = SelectBestTerritory(candidates, ownerComp);
        return true;
    }
    
    return false;
}

bool UMoveToFriendlyTerritoryTask::FindNearestAlly(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const
{
    // 아군 찾기 - AEnemy 클래스의 다른 인스턴스들
    UWorld* world = ownerComp.GetWorld();

    if (world == nullptr)
        return false;
    
    APawn* controlledPawn = ownerComp.GetAIOwner()->GetPawn();

    if (controlledPawn == nullptr)
        return false;
    
    TArray<AActor*> allEnemies;
    UGameplayStatics::GetAllActorsOfClass(world, AEnemy::StaticClass(), allEnemies);
    
    AActor* nearestAlly = nullptr;
    float nearestDistance = MAX_FLT;
    
    for (AActor* actor : allEnemies)
    {
        // 자기 자신 제외
        if (actor == controlledPawn) 
            continue; 
        
        float distance = FVector::Dist(controlledPawn->GetActorLocation(), actor->GetActorLocation());

        if (distance < nearestDistance && distance <= maxSearchRadius)
        {
            nearestDistance = distance;
            nearestAlly = actor;
        }
    }
    
    if (nearestAlly && IsLocationSafe(nearestAlly->GetActorLocation(), ownerComp))
    {
        outLocation = nearestAlly->GetActorLocation();

        return true;
    }
    
    return false;
}

bool UMoveToFriendlyTerritoryTask::GetCustomLocation(UBehaviorTreeComponent& ownerComp, FVector& outLocation) const
{
    UBlackboardComponent* blackboardComp = ownerComp.GetBlackboardComponent();

    if (blackboardComp == nullptr)
        return false;
    
    FVector customLocation = blackboardComp->GetValueAsVector(customLocationKey.SelectedKeyName);

    if (customLocation.IsZero())
        return false;
    
    outLocation = customLocation;

    return true;
}

TArray<AActor*> UMoveToFriendlyTerritoryTask::FindActorsWithTag(UBehaviorTreeComponent& ownerComp, const FName& tag) const
{
    TArray<AActor*> result;

    UWorld* world = ownerComp.GetWorld();

    if (world == nullptr)
        return result;
    
    APawn* controlledPawn = ownerComp.GetAIOwner()->GetPawn();

    if (controlledPawn == nullptr)
        return result;
    
    FVector pawnLocation = controlledPawn->GetActorLocation();
    
	// 모든 액터를 검색
    TArray<AActor*> allActors;
    UGameplayStatics::GetAllActorsOfClass(world, AActor::StaticClass(), allActors);
    
    for (AActor* actor : allActors)
    {
        if (actor && actor->Tags.Contains(tag))
        {
            float distance = FVector::Dist(pawnLocation, actor->GetActorLocation());

            if (distance <= maxSearchRadius)
                result.Add(actor);
        }
    }
    
    return result;
}

FVector UMoveToFriendlyTerritoryTask::SelectBestTerritory(const TArray<FVector>& candidates, UBehaviorTreeComponent& ownerComp) const
{
    if (candidates.Num() == 0)
        return FVector::ZeroVector;

    if (candidates.Num() == 1)
        return candidates[0];

    APawn* controlledPawn = ownerComp.GetAIOwner()->GetPawn();
    AActor* currentTarget = GetCurrentTarget(ownerComp);
    FVector pawnLocation = controlledPawn->GetActorLocation();

    FVector bestLocation = candidates[0];
    float bestScore = -MAX_FLT;

    for (const FVector& candidate : candidates)
    {
        float score = 0.0f;
        float distanceToSelf = FVector::Dist(pawnLocation, candidate);

        // 가까운 것을 선호 (거리가 가까울수록 높은 점수)
        if (isChooseClosest)
            score += (maxSearchRadius - distanceToSelf) / maxSearchRadius * 100.0f;

        // 타겟으로부터 먼 것을 선호
        if (isChooseFarthestFromTarget && currentTarget)
        {
            float distanceToTarget = FVector::Dist(currentTarget->GetActorLocation(), candidate);

            score += distanceToTarget / maxSearchRadius * 200.0f; // 안전성에 더 높은 가중치
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestLocation = candidate;
        }
    }

    return bestLocation;
}

bool UMoveToFriendlyTerritoryTask::IsLocationSafe(const FVector& location, UBehaviorTreeComponent& ownerComp) const
{
    AActor* currentTarget = GetCurrentTarget(ownerComp);

    if (currentTarget == nullptr)
        return true; // 타겟이 없으면 모든 위치가 안전
    
    float distanceToTarget = FVector::Dist(location, currentTarget->GetActorLocation());

    return distanceToTarget >= minSafeDistance;
}

bool UMoveToFriendlyTerritoryTask::MoveToLocation(UBehaviorTreeComponent& ownerComp, const FVector& targetLocation) const
{
    AAIController* aiController = ownerComp.GetAIOwner();

    if (aiController == nullptr)
        return false;
    
    // AI 이동 요청 생성 - 더 고급 AI 기능 활용
    FAIMoveRequest moveRequest;
    moveRequest.SetGoalLocation(targetLocation);
    moveRequest.SetAcceptanceRadius(acceptanceRadius);
    moveRequest.SetCanStrafe(true); // 스트레이프 이동 허용
    moveRequest.SetReachTestIncludesAgentRadius(true);
    moveRequest.SetReachTestIncludesGoalRadius(true);
    moveRequest.SetProjectGoalLocation(true); // 목표 위치를 네비메시에 투영
    moveRequest.SetUsePathfinding(true); // 패스파인딩 사용
    moveRequest.SetAllowPartialPath(false); // 부분 경로 허용 안함
    
    // 이동 속도 조정
    if (APawn* pawn = aiController->GetPawn())
    {
        if (ACharacter* character = Cast<ACharacter>(pawn))
        {
            if (UCharacterMovementComponent* movementComp = character->GetCharacterMovement())
            {
                // 우호 지역으로 이동할 때는 조금 더 빠르게
                movementComp->MaxWalkSpeed = movementSpeed * 1.2f;
            }
        }
    }
    
    // AI 이동 실행
    EPathFollowingRequestResult::Type result = aiController->MoveTo(moveRequest);
    
    // 결과 확인 및 디버깅
    switch (result)
    {
        case EPathFollowingRequestResult::RequestSuccessful:
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
                TEXT("AI 이동 요청 성공"));
            return true;
            
        case EPathFollowingRequestResult::AlreadyAtGoal:
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
                TEXT("AI 이미 목표 위치에 도달"));
            return true;
            
        case EPathFollowingRequestResult::Failed:
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
                TEXT("AI 이동 요청 실패"));
            return false;
            
        default:
            return false;
    }
}

FVector UMoveToFriendlyTerritoryTask::FindNavigablePosition(const FVector& desiredPosition, UBehaviorTreeComponent& ownerComp) const
{
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(ownerComp.GetWorld());

    if (navSystem == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            TEXT("네비게이션 시스템을 찾을 수 없음"));
        return desiredPosition;
    }
    
    APawn* controlledPawn = ownerComp.GetAIOwner()->GetPawn();
    if (controlledPawn == nullptr)
        return desiredPosition;
    
    // AI 에이전트 정보 가져오기
    const ANavigationData* navData = navSystem->GetNavDataForProps(controlledPawn->GetNavAgentPropertiesRef());
    if (navData == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            TEXT("네비게이션 데이터를 찾을 수 없음"));
        return desiredPosition;
    }
    
    FNavLocation navLocation;
    
    // 1. 먼저 정확한 위치로 투영 시도
    FVector extents(500.0f, 500.0f, 1000.0f); // 더 큰 범위로 검색
    if (navSystem->ProjectPointToNavigation(desiredPosition, navLocation, extents, navData))
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("네비게이션 투영 성공: %.1f 단위 이동"), 
                FVector::Dist(desiredPosition, navLocation.Location)));
        return navLocation.Location;
    }
    
    // 2. 투영 실패시 근처 네비게이션 가능한 위치 찾기
    float searchRadius = 1000.0f; // 더 큰 반경으로 검색
    if (navSystem->GetRandomReachablePointInRadius(desiredPosition, searchRadius, navLocation))
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
            FString::Printf(TEXT("근처 네비게이션 위치 발견: %.1f 단위 거리"), 
                FVector::Dist(desiredPosition, navLocation.Location)));
        return navLocation.Location;
    }
    
    // 3. 현재 위치에서 목표 방향으로 갈 수 있는 최대한 가까운 위치 찾기
    FVector currentLocation = controlledPawn->GetActorLocation();
    FVector direction = (desiredPosition - currentLocation).GetSafeNormal();
    
    for (float distance = 500.0f; distance <= 3000.0f; distance += 500.0f)
    {
        FVector testLocation = currentLocation + (direction * distance);
        if (navSystem->ProjectPointToNavigation(testLocation, navLocation, FVector(200.0f), navData))
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
                FString::Printf(TEXT("방향 검색 성공: %.1f 거리에서 발견"), distance));
            return navLocation.Location;
        }
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
        TEXT("네비게이션 가능한 위치를 찾을 수 없음 - 원래 위치 반환"));
    
    return FVector::ZeroVector;
}

AActor* UMoveToFriendlyTerritoryTask::GetCurrentTarget(UBehaviorTreeComponent& ownerComp) const
{
    UBlackboardComponent* blackboardComp = ownerComp.GetBlackboardComponent();

    if (blackboardComp == nullptr)
        return nullptr;
    
    return Cast<AActor>(blackboardComp->GetValueAsObject(targetActorKey.SelectedKeyName));
}

bool UMoveToFriendlyTerritoryTask::HasReachedDestination(UBehaviorTreeComponent& ownerComp, const FVector& targetLocation) const
{
    APawn* controlledPawn = ownerComp.GetAIOwner()->GetPawn();

    if (controlledPawn == nullptr)
        return false;
    
    float distance = FVector::Dist(controlledPawn->GetActorLocation(), targetLocation);

    return distance <= acceptanceRadius;
}

AEnemy* UMoveToFriendlyTerritoryTask::GetControlledEnemy(UBehaviorTreeComponent& ownerComp) const
{
    AAIController* aiController = ownerComp.GetAIOwner();

    if (aiController == nullptr)
        return nullptr;
    
    return Cast<AEnemy>(aiController->GetPawn());
}

void UMoveToFriendlyTerritoryTask::HandleCombatWhileMoving(UBehaviorTreeComponent& ownerComp, FMoveToFriendlyTerritoryTaskMemory* taskMemory) const
{
    // 이동 중 전투 처리 로직
    AActor* currentTarget = GetCurrentTarget(ownerComp);

    if (currentTarget)
    {
        taskMemory->isCombatOccurred = true;
        // 여기에 이동 중 전투 로직 추가 가능
        // 예: 발사하면서 이동, 엄폐물 활용 등
    }
}

// === 단계별 처리 함수들 ===

void UMoveToFriendlyTerritoryTask::HandleFindingTerritory(UBehaviorTreeComponent& ownerComp, FMoveToFriendlyTerritoryTaskMemory* taskMemory, float currentTime)
{
    FVector territoryLocation;

    if (FindTargetTerritory(ownerComp, territoryLocation))
    {
        FVector navigableLocation = FindNavigablePosition(territoryLocation, ownerComp);
        if (!navigableLocation.IsZero())
        {
            taskMemory->targetTerritoryLocation = navigableLocation;
            taskMemory->isTerritoryFound = true;
            taskMemory->currentPhase = EMoveToTerritoryPhase::MovingToTerritory;
            taskMemory->currentPhaseStartTime = currentTime;

            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                FString::Printf(TEXT("MoveToFriendlyTerritoryTask: 우호 지역 발견 - %s"), *territoryLocation.ToString()));
        }

        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                TEXT("Error (UnnavigableTerritory, MoveToFriendlyTerritoryTask.cpp) : 네비게이션 불가능한 우호 지역"));

            taskMemory->currentPhase = EMoveToTerritoryPhase::Completed;
        }
    }

    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NoTerritoryFound, MoveToFriendlyTerritoryTask.cpp) : 우호 지역을 찾을 수 없습니다"));
        taskMemory->currentPhase = EMoveToTerritoryPhase::Completed;
    }
}

void UMoveToFriendlyTerritoryTask::HandleMovingToTerritory(UBehaviorTreeComponent& ownerComp, FMoveToFriendlyTerritoryTaskMemory* taskMemory, float currentTime)
{
    if (HasReachedDestination(ownerComp, taskMemory->targetTerritoryLocation))
    {
        taskMemory->currentPhase = EMoveToTerritoryPhase::WaitingAtTerritory;
        taskMemory->currentPhaseStartTime = currentTime;
        taskMemory->isMoving = false;
        taskMemory->isArrivedAtTerritory = true;

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
            TEXT("MoveToFriendlyTerritoryTask: 우호 지역 도착"));
    }

    else if (!taskMemory->isMoving)
    {
        if (MoveToLocation(ownerComp, taskMemory->targetTerritoryLocation))
        {
            taskMemory->isMoving = true;

            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                TEXT("MoveToFriendlyTerritoryTask: 우호 지역으로 이동 시작"));
        }

        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                TEXT("Error (MoveToTerritoryFailed, MoveToFriendlyTerritoryTask.cpp) : 우호 지역으로 이동 실패"));

            taskMemory->currentPhase = EMoveToTerritoryPhase::Completed;
        }
    }
}

void UMoveToFriendlyTerritoryTask::HandleWaitingAtTerritory(UBehaviorTreeComponent& OwnerComp, FMoveToFriendlyTerritoryTaskMemory* taskMemory, float currentTime, float phaseElapsedTime)
{
    if (phaseElapsedTime >= waitTimeAtTerritory)
    {
        taskMemory->currentPhase = EMoveToTerritoryPhase::Completed;
        taskMemory->isMovementCompleted = true;

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("MoveToFriendlyTerritoryTask: 우호 지역에서 대기 완료"));
    }
}