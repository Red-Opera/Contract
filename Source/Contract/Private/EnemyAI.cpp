#include "EnemyAI.h"
#include "Enemy.h"
#include "OccupiedTerritory.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyAI::AEnemyAI()
{
    PrimaryActorTick.bCanEverTick = true;

    // AI 인식 컴포넌트 초기화
    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

    // 시각 감지 설정
    UAISenseConfig_Sight* sightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight Config"));
    sightConfig->SightRadius = 1500.0f;
    sightConfig->LoseSightRadius = 2000.0f;
    sightConfig->PeripheralVisionAngleDegrees = 90.0f;
    sightConfig->DetectionByAffiliation.bDetectEnemies = true;
    sightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    sightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    AIPerceptionComp->ConfigureSense(*sightConfig);
    AIPerceptionComp->SetDominantSense(sightConfig->GetSenseImplementation());

    // 변수 초기화
    currentTargetTerritory = nullptr;
    controlledPawn = nullptr;
}

void AEnemyAI::BeginPlay()
{
    Super::BeginPlay();

    // 필수 컴포넌트 검증
    if (AIPerceptionComp == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("AI Perception Component가 존재하지 않습니다."));
        
        return;
    }

    // 월드 존재 검증
    if (GetWorld() == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("World가 존재하지 않습니다."));
        
        return;
    }

    // 내비게이션 시스템 검증
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (navSystem == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Navigation System이 존재하지 않습니다."));
        
        return;
    }

    // 경로 추적 컴포넌트 검증
    if (GetPathFollowingComponent() == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Path Following Component가 존재하지 않습니다."));
        
        return;
    }

    // 인식 이벤트 콜백 등록
    AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAI::OnPerceptionUpdated);

    
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Enemy AI 초기화 완료"));
}

void AEnemyAI::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    // 매 프레임 상태 업데이트
    UpdateMovementDirection(deltaTime);                     // 이동 방향 업데이트 - 가장 중요!
    UpdateTerritorySearchState(deltaTime);                  // 영역 탐색 상태 업데이트
    UpdateCombatState(deltaTime);                           // 전투 상태 업데이트
}

void AEnemyAI::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // 제어할 폰 참조 가져오기
    controlledPawn = InPawn;
    
    if (controlledPawn == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("EnemyAI: 제어할 폰이 null입니다!"));
        return;
    }

    lastPosition = controlledPawn->GetActorLocation();
    
    // 경로 이동 설정
    if (GetPathFollowingComponent() != nullptr)
        GetPathFollowingComponent()->SetAcceptanceRadius(acceptanceRadius);
   
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Enemy AI Possessed Pawn"));

    // 월드 존재 검증 후 타이머 설정
    if (GetWorld() != nullptr)
    {
        // 지연된 초기 이동 (BeginPlay 후에 실행되도록)
        FTimerHandle InitialMoveTimer;

        GetWorld()->GetTimerManager().SetTimer(
            InitialMoveTimer,
            FTimerDelegate::CreateUObject(this, &AEnemyAI::MoveToFriendlyTerritory),
            1.0f,
            false
        );
    }

    // 비헤이비어 트리 초기화
    if (BehaviorTree != nullptr && blackboardData != nullptr)
    {
        UBlackboardComponent* blackboardComp = GetBlackboardComponent();

        if (blackboardComp != nullptr)
        {
            UseBlackboard(blackboardData, blackboardComp);
            RunBehaviorTree(BehaviorTree);
        }
    }
}

void AEnemyAI::OnUnPossess()
{
    Super::OnUnPossess();
    controlledPawn = nullptr;
    currentTargetTerritory = nullptr;
}

AOccupiedTerritory* AEnemyAI::FindNearestFriendlyTerritory()
{
    if (GetWorld() == nullptr || controlledPawn == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("World 또는 Controlled Pawn이 존재하지 않습니다."));

        return nullptr;
    }

    // 모든 AOccupiedTerritory 액터 찾기
    TArray<AActor*> foundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOccupiedTerritory::StaticClass(), foundActors);

    if (foundActors.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("OccupiedTerritory 액터가 발견되지 않았습니다."));

        return nullptr;
    }

    AOccupiedTerritory* nearestFriendlyTerritory = nullptr;
    float shortestDistance = FLT_MAX;
    FVector currentLocation = controlledPawn->GetActorLocation();

    for (AActor* actor : foundActors)
    {
        AOccupiedTerritory* territory = Cast<AOccupiedTerritory>(actor);

        if (territory != nullptr && territory->IsFriendlyTerritory())
        {
            float distance = FVector::Dist(currentLocation, territory->GetActorLocation());

            if (distance < shortestDistance)
            {
                shortestDistance = distance;
                nearestFriendlyTerritory = territory;
            }
        }
    }

    if (nearestFriendlyTerritory != nullptr)
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
            FString::Printf(TEXT("아군 영역 발견: %s (거리: %.2f)"),
                *nearestFriendlyTerritory->GetActorLocation().ToString(), shortestDistance));

    else
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("아군 영역을 찾을 수 없습니다."));

    return nearestFriendlyTerritory;
}

void AEnemyAI::MoveToFriendlyTerritory()
{
    // 이미 이동 중이면 무시
    if (isMovingToTerritory)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("이미 영역으로 이동 중입니다."));

        return;
    }

    AOccupiedTerritory* targetTerritory = FindNearestFriendlyTerritory();
    if (targetTerritory != nullptr)
    {
        currentTargetTerritory = targetTerritory;
        MoveToTargetLocation(targetTerritory->GetActorLocation());
        
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue, TEXT("아군 영역으로 이동 시작"));
    }
}

void AEnemyAI::MoveToTargetLocation(FVector targetLocation)
{
    if (controlledPawn == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("제어할 Pawn이 존재하지 않습니다."));

        return;
    }

    FVector currentLocation = controlledPawn->GetActorLocation();

    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
        FString::Printf(TEXT("현재 위치: %s -> 목표 위치: %s"),
            *currentLocation.ToString(), *targetLocation.ToString()));

    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (navSystem == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Navigation System을 찾을 수 없습니다."));

        return;
    }

    // 목표 위치가 내비게이션 가능한지 확인
    FVector navigableLocation = FindNearestNavigableLocation(targetLocation);

    if (navigableLocation != FVector::ZeroVector)
    {
        // 이동 요청 설정
        FAIMoveRequest moveRequest;
        moveRequest.SetGoalLocation(navigableLocation);
        moveRequest.SetAcceptanceRadius(acceptanceRadius);
        moveRequest.SetUsePathfinding(true);
        moveRequest.SetAllowPartialPath(true);
        moveRequest.SetProjectGoalLocation(true);

        // 이동 요청 실행
        FPathFollowingRequestResult result = MoveTo(moveRequest);

        if (result.Code == EPathFollowingRequestResult::RequestSuccessful)
        {
            isMovingToTerritory = true;

            float distance = FVector::Dist(currentLocation, navigableLocation);

            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
                FString::Printf(TEXT("이동 요청 성공! 거리: %.2f"), distance));
        }

        else
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("이동 요청 실패!"));
    }

    else
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("이동 가능한 위치를 찾을 수 없습니다."));
}

void AEnemyAI::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    isMovingToTerritory = false;

    if (Result.IsSuccess())
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("목표 위치에 성공적으로 도달했습니다!"));

        // 목표 영역에 도달했으면 다음 행동 결정
        if (currentTargetTerritory != nullptr)
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("아군 영역 도달 - 전투 준비!"));
    }

    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("목표 위치 도달 실패!"));

        // 이동 실패 시 타이머를 사용하여 지연된 재시도 (무한 재귀 방지)
        if (GetWorld() != nullptr)
        {
            FTimerHandle RetryMoveTimer;
            GetWorld()->GetTimerManager().SetTimer(
                RetryMoveTimer,
                FTimerDelegate::CreateUObject(this, &AEnemyAI::MoveToFriendlyTerritory),
                2.0f,  // 2초 후 재시도
                false
            );
        }
    }
}

void AEnemyAI::UpdateTerritorySearchState(float DeltaTime)
{
    timeSinceLastTerritorySearch += DeltaTime;
    
    // 주기적으로 새로운 영역 탐색 (현재 이동 중이 아닐 때만)
    if (!isMovingToTerritory && timeSinceLastTerritorySearch >= territorySearchInterval)
    {
        timeSinceLastTerritorySearch = 0.0f;
        
        // 현재 목표가 없거나 더 가까운 영역이 있는지 확인
        AOccupiedTerritory* nearestTerritory = FindNearestFriendlyTerritory();

        if (nearestTerritory != nullptr && nearestTerritory != currentTargetTerritory)
            MoveToFriendlyTerritory();
    }
}

void AEnemyAI::UpdateCombatState(float DeltaTime)
{
    timeSinceLastAttackDecision += DeltaTime;
    
    if (timeSinceLastAttackDecision < attackDecisionUpdateInterval)
        return;
        
    timeSinceLastAttackDecision = 0.0f;

    // 공격 범위 내에 있으면 공격, 아니면 공격 중지
    if (IsInAttackRange())
    {
        if (!isAttacking)
            StartAttack();
    }

    else
    {
        if (isAttacking)
            StopAttack();
    }
}

void AEnemyAI::UpdateMovementDirection(float DeltaTime)
{
    if (!controlledPawn)
        return;

    // 방향 업데이트 타이머 증가
    timeSinceLastDirectionUpdate += DeltaTime;

    // 지정된 간격으로 방향 업데이트 (더 자주 업데이트)
    if (timeSinceLastDirectionUpdate >= movementDirectionUpdateInterval)
    {
        timeSinceLastDirectionUpdate = 0.0f;

        // 현재 위치
        FVector currentLocation = controlledPawn->GetActorLocation();

        // 이동 방향 계산 (이전 위치와 현재 위치의 차이)
        FVector movementDelta = currentLocation - lastPosition;
        movementDelta.Z = 0.0f; // 수직 이동 무시

        // 이동 거리가 충분한 경우에만 방향 업데이트
        if (movementDelta.SizeSquared() > 1.0f) // 1cm 이상 이동한 경우
        {
            currentMovementDirection = movementDelta.GetSafeNormal();
        }
        else
        {
            // 정지 상태
            currentMovementDirection = FVector::ZeroVector;
        }

        // 이전 위치 업데이트
        lastPosition = currentLocation;

        // Enemy 캐릭터에 이동 상태 전달
        AEnemy* enemy = Cast<AEnemy>(controlledPawn);
        if (enemy)
        {
            // AI가 계산한 이동 방향을 Enemy에 전달
            bool isRunning = currentMovementDirection.SizeSquared() > 0.5f; // 빠르게 이동 중인지 판단
            enemy->UpdateMovementState(isRunning, currentMovementDirection);
            
            // 디버그 정보 출력
            GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Orange,
                FString::Printf(TEXT("Enemy AI MovementDirection: %s, Running: %s"),
                    *currentMovementDirection.ToString(), isRunning ? TEXT("Yes") : TEXT("No")));
        }
    }
}

void AEnemyAI::StartAttack()
{
    isAttacking = true;
    
    GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("적 공격 시작!"));
}

void AEnemyAI::StopAttack()
{
    isAttacking = false;
    
    
    GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("적 공격 중지!"));
}

bool AEnemyAI::IsInAttackRange() const
{
    // 현재 목표 영역과의 거리가 공격 범위 내인지 확인
    if (controlledPawn == nullptr || currentTargetTerritory == nullptr)
        return false;
        
    const float distanceToTarget = FVector::Dist(
        controlledPawn->GetActorLocation(), 
        currentTargetTerritory->GetActorLocation()
    );
    
    return distanceToTarget < attackRange;
}

FVector AEnemyAI::FindNearestNavigableLocation(FVector targetLocation)
{
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(GetWorld());

    if (navSystem == nullptr) 
        return FVector::ZeroVector;

    FNavLocation navLocation;
    
    // 1. 먼저 목표 위치가 NavMesh에 있는지 확인
    if (navSystem->ProjectPointToNavigation(targetLocation, navLocation, FVector(100.0f, 100.0f, 100.0f)))
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
            FString::Printf(TEXT("목표 위치가 이동 가능합니다: %s"), *navLocation.Location.ToString()));

        return navLocation.Location;
    }

    // 2. 목표 위치 주변에서 가장 가까운 NavMesh 위치 찾기
    TArray<float> searchRadii = {200.0f, 500.0f, 1000.0f, 2000.0f};
    
    for (float radius : searchRadii)
    {
        if (navSystem->GetRandomReachablePointInRadius(targetLocation, radius, navLocation))
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue,
                FString::Printf(TEXT("반경 %.0f에서 이동 가능한 위치 발견: %s"),
                    radius, *navLocation.Location.ToString()));

            return navLocation.Location;
        }
    }

    // 3. 현재 위치에서 목표 방향으로 가장 가까운 NavMesh 위치 찾기
    if (controlledPawn != nullptr)
    {
        FVector currentLocation = controlledPawn->GetActorLocation();

        if (navSystem->GetRandomReachablePointInRadius(currentLocation, 1000.0f, navLocation))
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange,
                FString::Printf(TEXT("현재 위치에서 대체 위치로 이동: %s"), *navLocation.Location.ToString()));

            return navLocation.Location;
        }
    }

    return FVector::ZeroVector;
}

bool AEnemyAI::IsLocationNavigable(FVector location)
{
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(GetWorld());

    if (navSystem == nullptr) 
        return false;

    FNavLocation navLocation;
    return navSystem->ProjectPointToNavigation(location, navLocation, FVector(50.0f, 50.0f, 50.0f));
}

void AEnemyAI::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    // AI 인식 처리 (필요에 따라 구현)
    if (Stimulus.WasSuccessfullySensed())
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Purple, 
            FString::Printf(TEXT("적 AI가 감지함: %s"), *Actor->GetName()));
    }
}