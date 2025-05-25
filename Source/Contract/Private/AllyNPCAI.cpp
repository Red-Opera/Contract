#include "AllyNPCAI.h"
#include "AllyNPC.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

AAllyNPCAI::AAllyNPCAI()
{
    PrimaryActorTick.bCanEverTick = true;

    // AI 인식 컴포넌트 초기화
    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

    // 시각 감지 설정
    UAISenseConfig_Sight* sightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight Config"));
    sightConfig->SightRadius = 1500.0f;                   // 시야 범위
    sightConfig->LoseSightRadius = 2000.0f;               // 시야 손실 범위
    sightConfig->PeripheralVisionAngleDegrees = 60.0f;    // 주변 시야각 (도)
    sightConfig->DetectionByAffiliation.bDetectEnemies = true;
    sightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    sightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    AIPerceptionComp->ConfigureSense(*sightConfig);
    AIPerceptionComp->SetDominantSense(sightConfig->GetSenseImplementation());

    // 전투 변수 초기화
    isFiring = false;
    timeSinceLastShotDecision = 0.0f;
    
    // 이동 방향 추적 변수 초기화
    lastPosition = FVector::ZeroVector;
    currentMovementDirection = FVector::ZeroVector;
    timeSinceLastDirectionUpdate = 0.0f;

    // 속도 보간 변수 초기화 (부드러운 속도 변화를 위함)
    currentSpeed = 0.0f;
    targetSpeed = 0.0f; 
    speedInterpRate = 2.0f;  // 낮은 값으로 설정하여 부드러운 가속/감속
}

void AAllyNPCAI::BeginPlay()
{
    Super::BeginPlay();

    // 플레이어 캐릭터 참조 가져오기
    playerPawn = GetPlayerPawn();

    // 디버그 검사: 플레이어 캐릭터 존재 여부
    if (playerPawn == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 캐릭터를 찾을 수 없습니다!"));
        return;
    }

    // 디버그 검사: NPC 존재 여부
    if (controlledAllyNPC == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("제어할 Ally NPC가 없습니다!"));
        return;
    }

    // 초기 위치 저장
    lastPosition = controlledAllyNPC->GetActorLocation();

    // 인식 이벤트 콜백 등록
    AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AAllyNPCAI::OnPerceptionUpdated);
}

void AAllyNPCAI::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    // 매 프레임 상태 업데이트
    UpdateMovementDirection(deltaTime);  // 이동 방향 업데이트
    UpdateMovementState(deltaTime);      // 이동 상태 업데이트
    UpdateCombatState(deltaTime);        // 전투 상태 업데이트
}

void AAllyNPCAI::OnPossess(APawn* inPawn)
{
    Super::OnPossess(inPawn);

    // 제어할 NPC 참조 가져오기
    controlledAllyNPC = Cast<AAllyNPC>(inPawn);

    // 플레이어 참조 가져오기
    playerPawn = GetPlayerPawn();

    // NPC 초기화
    if (controlledAllyNPC)
    {
        lastPosition = controlledAllyNPC->GetActorLocation();
        
        // 경로 이동 설정
        if (GetPathFollowingComponent())
        {
            // 목적지 도달 범위 설정
            GetPathFollowingComponent()->SetAcceptanceRadius(100.0f);
        }
        
        // 부드러운 이동을 위한 캐릭터 이동 컴포넌트 설정
        if (controlledAllyNPC->GetCharacterMovement())
        {
            UCharacterMovementComponent* movementComp = controlledAllyNPC->GetCharacterMovement();
            movementComp->bRequestedMoveUseAcceleration = true;
        }
    }

    // 비헤이비어 트리 초기화
    if (BehaviorTree && blackboardData)
    {
        UBlackboardComponent* blackboardComp = GetBlackboardComponent();

        if (blackboardComp)
        {
            UseBlackboard(blackboardData, blackboardComp);
            RunBehaviorTree(BehaviorTree);
        }
    }

    // 플레이어 추적 시작
    MoveToPlayer();
}

void AAllyNPCAI::OnUnPossess()
{
    Super::OnUnPossess();
    controlledAllyNPC = nullptr;
}

void AAllyNPCAI::MoveToPlayer()
{
    if (!playerPawn || !controlledAllyNPC)
    {
        return;
    }

    // UpdateMovementState에서 실제 이동 처리
    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Blue, TEXT("MoveToPlayer 호출됨"));
}

void AAllyNPCAI::StartFiring()
{
    isFiring = true;
    controlledAllyNPC->StartFiring();
}

void AAllyNPCAI::StopFiring()
{
    isFiring = false;
    controlledAllyNPC->StopFiring();
}

void AAllyNPCAI::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    // AI 인식 처리 (현재는 플레이어 지원 역할로 미구현)
}

APawn* AAllyNPCAI::GetPlayerPawn()
{
    return UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
}

void AAllyNPCAI::UpdateCombatState(float DeltaTime)
{
    // 발사 결정 타이머 업데이트
    timeSinceLastShotDecision += DeltaTime;

    // 결정 주기 대기
    if (timeSinceLastShotDecision < decisionUpdateInterval)
        return;

    timeSinceLastShotDecision = 0.0f;

    // 발사 범위 내에 있으면 발사, 아니면 발사 중지
    if (IsInFireRange())
    {
        if (!isFiring)
            StartFiring();
    }
    else
    {
        if (isFiring)
            StopFiring();
    }
}

void AAllyNPCAI::UpdateMovementState(float DeltaTime)
{
    if (!controlledAllyNPC || !playerPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("controlledAllyNPC or playerPawn is null"));
        return;
    }

    // 플레이어와의 거리 계산
    const float distanceToPlayer = FVector::Dist(controlledAllyNPC->GetActorLocation(), playerPawn->GetActorLocation());
    
    // 디버그 정보 표시
    GEngine->AddOnScreenDebugMessage(
        -1, 0.1f, FColor::Yellow, 
        FString::Printf(TEXT("플레이어와의 거리: %.2f / 팔로우 거리: %.2f"), distanceToPlayer, followDistance)
    );

    // 히스테리시스를 적용한 이동 상태 결정
    if (distanceToPlayer > followDistance * 1.1f)  // 이동 시작 임계값
    {
        // 거리에 따른 이동 속도 결정 (달리기 또는 걷기)
        bool isRunning = distanceToPlayer > followDistance * 1.5f;
        targetSpeed = isRunning ? controlledAllyNPC->runSpeed : controlledAllyNPC->walkSpeed;
        
        // 플레이어 위치로 이동 요청 설정
        FVector playerLocation = playerPawn->GetActorLocation();
        FAIMoveRequest MoveRequest;
        MoveRequest.SetGoalLocation(playerLocation);
        MoveRequest.SetAcceptanceRadius(followDistance * 0.8f);
        MoveRequest.SetUsePathfinding(true);
        MoveRequest.SetAllowPartialPath(true);
        MoveRequest.SetProjectGoalLocation(false);
        MoveRequest.SetNavigationFilter(GetDefaultNavigationFilterClass());
        
        // 이동 요청 실행
        FPathFollowingRequestResult result = MoveTo(MoveRequest);
        
        // 애니메이션 상태 업데이트
        controlledAllyNPC->UpdateMovementState(isRunning, currentMovementDirection);
    }
    else if (distanceToPlayer < followDistance * 0.9f)  // 정지 임계값
    {
        // 충분히 가까우면 정지
        targetSpeed = 0.0f;
        StopMovement();
        controlledAllyNPC->UpdateMovementState(false, FVector::ZeroVector);
    }
    
    // 부드러운 속도 전환을 위한 보간 적용
    currentSpeed = FMath::FInterpTo(currentSpeed, targetSpeed, DeltaTime, speedInterpRate);
    
    // 계산된 속도 적용
    if (controlledAllyNPC->GetCharacterMovement())
    {
        controlledAllyNPC->GetCharacterMovement()->MaxWalkSpeed = currentSpeed;
    }
}

void AAllyNPCAI::UpdateMovementDirection(float DeltaTime)
{
    if (!controlledAllyNPC)
        return;

    // 방향 업데이트 타이머 증가
    timeSinceLastDirectionUpdate += DeltaTime;

    // 지정된 간격으로 방향 업데이트
    if (timeSinceLastDirectionUpdate >= movementDirectionUpdateInterval)
    {
        FVector currentPosition = controlledAllyNPC->GetActorLocation();
        FVector movementDelta = currentPosition - lastPosition;
        float movementDistance = movementDelta.Size();
        
        // 의미 있는 이동이 있을 경우만 방향 업데이트
        if (movementDistance > 0.01f)
        {
            // 이동 방향 정규화
            currentMovementDirection = movementDelta.GetSafeNormal();
            
            // 이동 방향으로 NPC 회전
            if (!currentMovementDirection.IsNearlyZero())
            {
                // 이동 방향을 회전으로 변환
                FRotator targetRotation = currentMovementDirection.Rotation();
                targetRotation.Pitch = 0.0f;  // 수평 유지
                targetRotation.Roll = 0.0f;   // 수평 유지
                
                // 현재 회전 가져오기
                FRotator currentRotation = controlledAllyNPC->GetActorRotation();
                
                // 부드러운 회전을 위한 보간 적용
                FRotator interpolatedRotation = FMath::RInterpTo(
                    currentRotation,
                    targetRotation,
                    DeltaTime,
                    rotationInterpSpeed
                );
                
                // 보간된 회전 적용
                controlledAllyNPC->SetActorRotation(interpolatedRotation);
                
                // 디버그 정보 표시
                GEngine->AddOnScreenDebugMessage(
                    -1, 0.1f, FColor::Green,
                    FString::Printf(TEXT("보간 회전: Yaw=%.2f, 목표=%.2f, 이동 방향: X=%.2f, Y=%.2f"),
                    interpolatedRotation.Yaw, targetRotation.Yaw, 
                    currentMovementDirection.X, currentMovementDirection.Y)
                );
            }
        }
        
        // 현재 위치 저장 및 타이머 초기화
        lastPosition = currentPosition;
        timeSinceLastDirectionUpdate = 0.0f;
    }
}

bool AAllyNPCAI::IsInFireRange() const
{
    // 플레이어와의 거리가 발사 범위(1000유닛) 내인지 확인
    const float DistanceToPlayer = FVector::Dist(controlledAllyNPC->GetActorLocation(), playerPawn->GetActorLocation());
    return DistanceToPlayer < 1000.0f;
}