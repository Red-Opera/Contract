#include "EnemyAI.h"
#include "Enemy.h"
#include "OccupiedTerritory.h"

// AI 및 비헤이비어 트리 관련
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

// 인식 시스템
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

// 네비게이션 및 이동
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITypes.h"

// 엔진 코어
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// 디버그 및 유틸리티
#include "DrawDebugHelpers.h"

// 블랙보드 키 이름 상수 정의
const FName AEnemyAI::BB_IsInCombat(TEXT("IsInCombat"));
const FName AEnemyAI::BB_IsAlert(TEXT("IsAlert"));
const FName AEnemyAI::BB_IsBurstFiring(TEXT("IsBurstFiring"));
const FName AEnemyAI::BB_TargetActor(TEXT("TargetActor"));
const FName AEnemyAI::BB_LastKnownPlayerLocation(TEXT("LastKnownPlayerLocation"));
const FName AEnemyAI::BB_FireDistance(TEXT("FireDistance"));
const FName AEnemyAI::BB_SelfActor(TEXT("SelfActor"));

// 디버그용 콘솔 변수 선언
static TAutoConsoleVariable<int32> CVarShowEnemyAIDebug(
    TEXT("ai.ShowEnemyAIDebug"),
    0,
    TEXT("Show Enemy AI debug information\n")
    TEXT("0: Disabled\n")
    TEXT("1: Enabled"),
    ECVF_Default
);

AEnemyAI::AEnemyAI()
{
    PrimaryActorTick.bCanEverTick = true;

    // === AI 인식 컴포넌트 초기화 ===
    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

    // 시각 감지 설정
    UAISenseConfig_Sight* sightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    if (sightConfig)
    {
        sightConfig->SightRadius = sightRadius;
        sightConfig->LoseSightRadius = loseSightRadius;
        sightConfig->PeripheralVisionAngleDegrees = peripheralVisionAngle;
        sightConfig->DetectionByAffiliation.bDetectEnemies = true;
        sightConfig->DetectionByAffiliation.bDetectFriendlies = true;
        sightConfig->DetectionByAffiliation.bDetectNeutrals = true;
        sightConfig->SetMaxAge(3.0f);

        AIPerceptionComp->ConfigureSense(*sightConfig);
        AIPerceptionComp->SetDominantSense(sightConfig->GetSenseImplementation());
    }

    // === 변수 초기화 ===
    currentTargetTerritory = nullptr;
    controlledPawn = nullptr;
    controlledEnemy = nullptr;
    
    // TPS Kit GASP 시스템 상태 초기화
    isInCombat = false;
    isAlert = false;
    isPatrolling = true;
    isBurstFiring = false;
    
    // 타겟 추적 초기화
    currentTarget = nullptr;
    lastKnownTargetLocation = FVector::ZeroVector;
    lastTargetSeenTime = 0.0f;
    
    // 타이머 초기화
    alertTimer = 0.0f;
    combatTimer = 0.0f;
}

void AEnemyAI::BeginPlay()
{
    Super::BeginPlay();

    // === 필수 컴포넌트 검증 ===
    if (!AIPerceptionComp)
    {
        UE_LOG(LogTemp, Error, TEXT("EnemyAI: AI Perception Component가 존재하지 않습니다."));
        return;
    }

    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("EnemyAI: World가 존재하지 않습니다."));
        return;
    }

    // === 내비게이션 시스템 검증 ===
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!navSystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnemyAI: Navigation System이 존재하지 않습니다."));
    }

    // === 경로 추적 컴포넌트 검증 ===
    if (!GetPathFollowingComponent())
    {
        UE_LOG(LogTemp, Warning, TEXT("EnemyAI: Path Following Component가 존재하지 않습니다."));
    }

    // === 인식 이벤트 콜백 등록 ===
    if (AIPerceptionComp)
    {
        AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAI::OnPerceptionUpdated);
    }

    UE_LOG(LogTemp, Log, TEXT("EnemyAI: 초기화 완료"));
}

void AEnemyAI::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    // === 🔧 이동 관련 코드 제거, 블랙보드 동기화만 유지 ===
    // 블랙보드 동기화는 비헤이비어 트리가 있을 때만
    if (BehaviorTree && GetBlackboardComponent())
    {
        SyncEnemyStateWithBlackboard();
        UpdateTargetDistance();
    }

    // === 디버그 정보 표시 ===
    DisplayDebugInfo();
}

void AEnemyAI::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // === 제어할 폰 참조 설정 ===
    controlledPawn = InPawn;
    controlledEnemy = Cast<AEnemy>(InPawn);
    
    if (!controlledPawn)
    {
        UE_LOG(LogTemp, Error, TEXT("EnemyAI: 제어할 폰이 null입니다!"));
        return;
    }

    if (!controlledEnemy)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnemyAI: 폰이 Enemy 클래스가 아닙니다!"));
    }

    // === 🔧 이동 관련 초기화 제거 ===
    
    // === 경로 이동 설정 ===
    if (GetPathFollowingComponent())
    {
        GetPathFollowingComponent()->SetAcceptanceRadius(acceptanceRadius);
    }

    // === 비헤이비어 트리 시스템 초기화 ===
    if (BehaviorTree && blackboardData)
    {
        // 🔧 수정된 블랙보드 사용 방법
        if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent())
        {
            BlackboardComp->InitializeBlackboard(*blackboardData);
            UE_LOG(LogTemp, Log, TEXT("EnemyAI: 블랙보드 초기화 완료"));
        }
        
        // 블랙보드 초기값 설정
        UBlackboardComponent* blackboardComp = GetBlackboardComponent();
        if (blackboardComp)
        {
            // 블랙보드 컴포넌트가 없는 경우 생성
            UseBlackboard(blackboardData, blackboardComp);

            // 기본 블랙보드 값 설정
            blackboardComp->SetValueAsObject(BB_SelfActor, controlledPawn);
            blackboardComp->SetValueAsBool(BB_IsInCombat, false);
            blackboardComp->SetValueAsBool(BB_IsAlert, false);
            blackboardComp->SetValueAsBool(BB_IsBurstFiring, false);
            blackboardComp->SetValueAsFloat(BB_FireDistance, 0.0f);
            
            UE_LOG(LogTemp, Log, TEXT("EnemyAI: 블랙보드 기본값 설정 완료"));
        }
        
        // 비헤이비어 트리 실행
        RunBehaviorTree(BehaviorTree);
        UE_LOG(LogTemp, Log, TEXT("EnemyAI: 비헤이비어 트리 시작"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("EnemyAI: BehaviorTree 또는 BlackboardData가 설정되지 않았습니다!"));
    }

    UE_LOG(LogTemp, Log, TEXT("EnemyAI: Pawn 제어 시작"));
}

void AEnemyAI::OnUnPossess()
{
    Super::OnUnPossess();
    
    controlledPawn = nullptr;
    controlledEnemy = nullptr;
    currentTargetTerritory = nullptr;
    currentTarget = nullptr;
    
    UE_LOG(LogTemp, Log, TEXT("EnemyAI: Pawn 제어 해제"));
}

// === TPS Kit GASP 시스템 - 상태 관리 ===

void AEnemyAI::EnterCombatState(AActor* Target)
{
    if (!Target || !controlledEnemy) return;

    // === Combat 상태로 전환 ===
    isInCombat = true;
    isAlert = false;
    isPatrolling = false;
    
    currentTarget = Target;
    lastKnownTargetLocation = Target->GetActorLocation();
    lastTargetSeenTime = GetWorld()->GetTimeSeconds();
    
    // === Enemy 객체에 전투 모드 진입 알림 ===
    controlledEnemy->EnterCombatMode(Target);
    
    // === 블랙보드 IsInCombat을 true로 즉시 설정 ===
    if (UBlackboardComponent* blackboardComp = GetBlackboardComponent())
    {
        blackboardComp->SetValueAsBool(BB_IsInCombat, true);
        blackboardComp->SetValueAsObject(BB_TargetActor, Target);
        blackboardComp->SetValueAsVector(BB_LastKnownPlayerLocation, Target->GetActorLocation());
        blackboardComp->SetValueAsBool(BB_IsAlert, false); // Combat 중에는 Alert 해제
        
        UE_LOG(LogTemp, Warning, TEXT("EnemyAI: 블랙보드 IsInCombat = true 설정"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("EnemyAI: Combat 상태 진입 - Target: %s"), *Target->GetName());
}

void AEnemyAI::EnterAlertState(const FVector& LastKnownLocation)
{
    if (!controlledEnemy) return;

    // === Alert 상태로 전환 ===
    isInCombat = false;
    isAlert = true;
    isPatrolling = false;
    
    lastKnownTargetLocation = LastKnownLocation;
    alertTimer = 0.0f;
    
    // === 블랙보드 업데이트 ===
    UpdateBlackboardKeys();
    
    UE_LOG(LogTemp, Warning, TEXT("EnemyAI: Alert 상태 진입 - Location: %s"), *LastKnownLocation.ToString());
}

void AEnemyAI::EnterPatrolState()
{
    if (!controlledEnemy) return;

    // === Patrol 상태로 전환 ===
    isInCombat = false;
    isAlert = false;
    isPatrolling = true;
    
    currentTarget = nullptr;
    
    // === Enemy 객체에 전투 모드 해제 알림 ===
    if (controlledEnemy->isInCombat)
    {
        controlledEnemy->ExitCombatMode();
    }
    
    // === 블랙보드 업데이트 ===
    UpdateBlackboardKeys();
    
    UE_LOG(LogTemp, Log, TEXT("EnemyAI: Patrol 상태 진입"));
}

void AEnemyAI::ClearAllStates()
{
    isInCombat = false;
    isAlert = false;
    isPatrolling = true;
    isBurstFiring = false;
    
    currentTarget = nullptr;
    alertTimer = 0.0f;
    combatTimer = 0.0f;
    
    if (controlledEnemy && controlledEnemy->isInCombat)
    {
        controlledEnemy->ExitCombatMode();
    }
    
    UpdateBlackboardKeys();
    
    UE_LOG(LogTemp, Log, TEXT("EnemyAI: 모든 상태 초기화"));
}

// === 상태 확인 함수들 ===

bool AEnemyAI::GetInCombat() const
{
    return isInCombat;
}

bool AEnemyAI::GetAlert() const
{
    return isAlert;
}

bool AEnemyAI::GetPatrolling() const
{
    return isPatrolling;
}

// === TPS Kit GASP 시스템 - 업데이트 함수들 ===

void AEnemyAI::UpdateGASPSystem(float DeltaTime)
{
    // === 🔧 완전히 비활성화 - 비헤이비어 트리에서만 제어 ===
    return;
}

void AEnemyAI::UpdateCombatBehavior(float DeltaTime)
{
    // === 🔧 자동 전투 행동 제거 - 비헤이비어 트리에서만 제어 ===
    return;
}

void AEnemyAI::UpdateAlertBehavior(float DeltaTime)
{
    // === 🔧 자동 경계 행동 제거 - 비헤이비어 트리에서만 제어 ===
    return;
}

void AEnemyAI::UpdatePatrolBehavior(float DeltaTime)
{
    // === 🔧 자동 패트롤 완전 제거 - 비헤이비어 트리에서만 제어 ===
    return;
}

// === 🔧 완전히 제거된 함수들 ===
// void AEnemyAI::UpdateMovementDirection(float DeltaTime) - 제거됨

// === 🔧 영역 탐색 및 이동 함수들 제거 ===

AOccupiedTerritory* AEnemyAI::FindNearestFriendlyTerritory()
{
    // === 🔧 비헤이비어 트리에서만 호출되도록 제한 ===
    if (!BehaviorTree || !GetBlackboardComponent()) return nullptr;
    
    if (!GetWorld() || !controlledPawn) return nullptr;

    TArray<AActor*> foundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOccupiedTerritory::StaticClass(), foundActors);

    if (foundActors.Num() == 0) return nullptr;

    AOccupiedTerritory* nearestFriendlyTerritory = nullptr;
    float shortestDistance = FLT_MAX;
    FVector currentLocation = controlledPawn->GetActorLocation();

    for (AActor* actor : foundActors)
    {
        AOccupiedTerritory* territory = Cast<AOccupiedTerritory>(actor);

        if (territory && territory->IsFriendlyTerritory())
        {
            float distance = FVector::Dist(currentLocation, territory->GetActorLocation());

            if (distance < shortestDistance)
            {
                shortestDistance = distance;
                nearestFriendlyTerritory = territory;
            }
        }
    }

    return nearestFriendlyTerritory;
}

// === 🔧 MoveToFriendlyTerritory, MoveToTargetLocation, OnMoveCompleted 제거 ===

// === 블랙보드 동기화 함수들 ===

void AEnemyAI::SyncEnemyStateWithBlackboard()
{
    // === 🔧 비헤이비어 트리가 없으면 블랙보드 동기화 안함 ===
    UBlackboardComponent* blackboardComp = GetBlackboardComponent();
    if (!blackboardComp || !controlledEnemy || !BehaviorTree) return;

    // === 🔧 전투 해제 조건 확인 ===
    CheckCombatDisengagementConditions(blackboardComp);

    // === Enemy → Blackboard 동기화 ===
    blackboardComp->SetValueAsBool(BB_IsInCombat, controlledEnemy->isInCombat);
    blackboardComp->SetValueAsBool(BB_IsAlert, isAlert);
    blackboardComp->SetValueAsBool(BB_IsBurstFiring, controlledEnemy->isBurstFiring);
    
    // === 타겟 정보 업데이트 ===
    if (controlledEnemy->currentTarget)
    {
        blackboardComp->SetValueAsObject(BB_TargetActor, controlledEnemy->currentTarget);
        blackboardComp->SetValueAsVector(BB_LastKnownPlayerLocation, controlledEnemy->currentTarget->GetActorLocation());
    }
    else if (!lastKnownTargetLocation.IsZero())
    {
        blackboardComp->SetValueAsVector(BB_LastKnownPlayerLocation, lastKnownTargetLocation);
    }
}

// === 🔧 새로 추가된 전투 해제 조건 확인 함수 ===
void AEnemyAI::CheckCombatDisengagementConditions(UBlackboardComponent* blackboardComp)
{
    if (!blackboardComp || !controlledPawn) return;

    // 현재 전투 중인지 확인
    bool currentIsInCombat = blackboardComp->GetValueAsBool(BB_IsInCombat);
    if (!currentIsInCombat) return;

    // FireDistance 확인
    float fireDistance = blackboardComp->GetValueAsFloat(BB_FireDistance);
    if (fireDistance < 800.0f) return;

    // 목표 이동 위치 확인 (LastKnownPlayerLocation을 이동 목표로 가정)
    FVector moveTargetLocation = blackboardComp->GetValueAsVector(BB_LastKnownPlayerLocation);
    if (moveTargetLocation.IsZero()) return;

    // 현재 위치에서 목표 이동 위치까지의 거리 확인
    float distanceToMoveTarget = FVector::Dist(controlledPawn->GetActorLocation(), moveTargetLocation);

    // 조건 확인: FireDistance >= 800 && 이동 목표까지 거리 <= 500
    if (fireDistance >= 800.0f && distanceToMoveTarget <= 500.0f)
    {
        // 전투 상태 해제
        isInCombat = false;
        
        // Enemy 객체도 전투 모드 해제
        if (controlledEnemy && controlledEnemy->isInCombat)
        {
            controlledEnemy->ExitCombatMode();
        }

        // 블랙보드 즉시 업데이트
        blackboardComp->SetValueAsBool(BB_IsInCombat, false);
        blackboardComp->SetValueAsBool(BB_IsAlert, true); // Alert 상태로 전환

        UE_LOG(LogTemp, Warning, TEXT("EnemyAI: 전투 해제 조건 충족 - FireDistance: %.1f, MoveDistance: %.1f"), 
            fireDistance, distanceToMoveTarget);
    }
}

// === 전투 관련 함수들 ===

void AEnemyAI::StartAttack()
{
    if (!controlledEnemy) return;
    
    isAttacking = true;
    controlledEnemy->StartGunFiring();
    
    UE_LOG(LogTemp, Warning, TEXT("EnemyAI: 공격 시작"));
}

void AEnemyAI::StopAttack()
{
    if (!controlledEnemy) return;
    
    isAttacking = false;
    controlledEnemy->StopGunFiring();
    
    UE_LOG(LogTemp, Log, TEXT("EnemyAI: 공격 중지"));
}

void AEnemyAI::StartBurstFire()
{
    if (!controlledEnemy || isBurstFiring) return;
    
    isBurstFiring = true;
    controlledEnemy->StartBurstFire();
    
    UE_LOG(LogTemp, Warning, TEXT("EnemyAI: 버스트 파이어 시작"));
}

void AEnemyAI::StopBurstFire()
{
    if (!controlledEnemy || !isBurstFiring) return;
    
    isBurstFiring = false;
    controlledEnemy->StopBurstFire();
    
    UE_LOG(LogTemp, Log, TEXT("EnemyAI: 버스트 파이어 중지"));
}

// === 인식 시스템 ===

void AEnemyAI::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor || !controlledEnemy) return;

    // === 플레이어 감지 처리만 유지 - 상태 변경은 블랙보드를 통해서만 ===
    if (APawn* detectedPawn = Cast<APawn>(Actor))
    {
        if (detectedPawn->IsPlayerControlled())
        {
            if (Stimulus.WasSuccessfullySensed())
            {
                // === 🔧 비헤이비어 트리가 자동으로 전투 모드 진입하도록 블랙보드 설정 ===
                if (UBlackboardComponent* blackboardComp = GetBlackboardComponent())
                {
                    blackboardComp->SetValueAsObject(BB_TargetActor, Actor);
                    blackboardComp->SetValueAsVector(BB_LastKnownPlayerLocation, Actor->GetActorLocation());
                    
                    float distance = FVector::Dist(controlledPawn->GetActorLocation(), Actor->GetActorLocation());
                    blackboardComp->SetValueAsFloat(BB_FireDistance, distance);
                    
                    // 🔧 IsInCombat을 true로 설정하여 Combat Sequence 활성화
                    blackboardComp->SetValueAsBool(BB_IsInCombat, true);
                    blackboardComp->SetValueAsBool(BB_IsAlert, false); // Alert 해제
                    
                    UE_LOG(LogTemp, Warning, TEXT("EnemyAI: 전투 모드 활성화 - Distance: %.1f"), distance);
                }
                
                // Enemy 객체도 전투 모드로 설정
                if (controlledEnemy && !controlledEnemy->IsInCombat())
                {
                    controlledEnemy->EnterCombatMode(Actor);
                }
            }
            else
            {
                // === 플레이어 시야에서 사라짐 ===
                if (UBlackboardComponent* blackboardComp = GetBlackboardComponent())
                {
                    blackboardComp->SetValueAsObject(BB_TargetActor, nullptr);
                    blackboardComp->SetValueAsBool(BB_IsInCombat, false);
                    blackboardComp->SetValueAsBool(BB_IsAlert, true); // Alert 모드로 전환
                    blackboardComp->SetValueAsFloat(BB_FireDistance, 9999.0f);
                }
            }
        }
    }
}

// === 유틸리티 함수들 ===

bool AEnemyAI::CanSeeTarget(AActor* Target) const
{
    if (!Target || !controlledPawn || !AIPerceptionComp) return false;
    
    FActorPerceptionBlueprintInfo perceptionInfo;
    AIPerceptionComp->GetActorsPerception(Target, perceptionInfo);
    
    for (const FAIStimulus& stimulus : perceptionInfo.LastSensedStimuli)
    {
        if (stimulus.WasSuccessfullySensed())
        {
            return true;
        }
    }
    
    return false;
}

void AEnemyAI::DisplayDebugInfo()
{
    if (CVarShowEnemyAIDebug.GetValueOnGameThread() == 0 || !controlledEnemy) return;
        
    FString stateInfo;
    if (isInCombat)
        stateInfo = TEXT("COMBAT");
    else if (isAlert)
        stateInfo = FString::Printf(TEXT("ALERT (%.1fs)"), alertTimer);
    else
        stateInfo = TEXT("PATROL");
        
    if (isBurstFiring)
        stateInfo += TEXT(" | FIRING");
        
    FString targetInfo = TEXT("No Target");
    if (currentTarget)
    {
        float distance = FVector::Dist(controlledEnemy->GetActorLocation(), currentTarget->GetActorLocation());
        targetInfo = FString::Printf(TEXT("%s (%.0f)"), *currentTarget->GetName(), distance);
    }
    
    FVector location = controlledEnemy->GetActorLocation() + FVector(0, 0, 100);
    DrawDebugString(GetWorld(), location, stateInfo, nullptr, FColor::White, 0.0f, true);
    DrawDebugString(GetWorld(), location + FVector(0, 0, 15), targetInfo, nullptr, FColor::Yellow, 0.0f, true);
}

void AEnemyAI::UpdateTargetDistance()
{
    UBlackboardComponent* blackboardComp = GetBlackboardComponent();
    if (!blackboardComp || !controlledPawn) return;

    // 현재 타겟 가져오기
    AActor* target = Cast<AActor>(blackboardComp->GetValueAsObject(BB_TargetActor));
    
    if (target)
    {
        // 타겟과의 거리 계산
        float distance = FVector::Dist(controlledPawn->GetActorLocation(), target->GetActorLocation());
        blackboardComp->SetValueAsFloat(BB_FireDistance, distance);
    }
    else
    {
        // 타겟이 없으면 매우 큰 값으로 설정
        blackboardComp->SetValueAsFloat(BB_FireDistance, 9999.0f);
    }
}

void AEnemyAI::UpdateBlackboardKeys()
{
    UBlackboardComponent* blackboardComp = GetBlackboardComponent();
    if (!blackboardComp) return;

    // 현재 상태를 블랙보드에 동기화
    blackboardComp->SetValueAsBool(BB_IsInCombat, isInCombat);
    blackboardComp->SetValueAsBool(BB_IsAlert, isAlert);
    blackboardComp->SetValueAsBool(BB_IsBurstFiring, isBurstFiring);
    
    // 타겟 정보 업데이트
    if (currentTarget)
    {
        blackboardComp->SetValueAsObject(BB_TargetActor, currentTarget);
        blackboardComp->SetValueAsVector(BB_LastKnownPlayerLocation, currentTarget->GetActorLocation());
        
        // 타겟과의 거리도 업데이트
        if (controlledPawn)
        {
            float distance = FVector::Dist(controlledPawn->GetActorLocation(), currentTarget->GetActorLocation());
            blackboardComp->SetValueAsFloat(BB_FireDistance, distance);
        }
    }
    else if (!lastKnownTargetLocation.IsZero())
    {
        blackboardComp->SetValueAsVector(BB_LastKnownPlayerLocation, lastKnownTargetLocation);
        blackboardComp->SetValueAsFloat(BB_FireDistance, 9999.0f);
    }
    
    // Self Actor 설정 (필요한 경우)
    if (controlledPawn)
    {
        blackboardComp->SetValueAsObject(BB_SelfActor, controlledPawn);
    }
}