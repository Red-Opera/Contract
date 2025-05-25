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

    // AI 인식 컴포넌트 설정
    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

    // Sight 감지 설정
    UAISenseConfig_Sight* sightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight Config"));
    sightConfig->SightRadius = 1500.0f;
    sightConfig->LoseSightRadius = 2000.0f;
    sightConfig->PeripheralVisionAngleDegrees = 60.0f;
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
}

void AAllyNPCAI::BeginPlay()
{
    Super::BeginPlay();

    // 플레이어 캐릭터 찾기
    playerPawn = GetPlayerPawn();

    if (playerPawn == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("플레이어 캐릭터를 찾을 수 없습니다!"));

        return;
    }

    if (controlledAllyNPC == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("제어할 Ally NPC가 없습니다!"));

        return;
    }

    // 초기 위치 설정
    lastPosition = controlledAllyNPC->GetActorLocation();

    // Perception 업데이트 콜백 등록
    AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AAllyNPCAI::OnPerceptionUpdated);
}

void AAllyNPCAI::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    // 이동 방향 업데이트
    UpdateMovementDirection(deltaTime);

    // 이동 상태 업데이트
    UpdateMovementState(deltaTime);

    // 전투 상태 업데이트
    UpdateCombatState(deltaTime);
}

void AAllyNPCAI::OnPossess(APawn* inPawn)
{
    Super::OnPossess(inPawn);

    controlledAllyNPC = Cast<AAllyNPC>(inPawn);

    // 플레이어 즉시 찾기
    playerPawn = GetPlayerPawn();

    // 초기 위치 설정
    if (controlledAllyNPC)
        lastPosition = controlledAllyNPC->GetActorLocation();

    // AI 컨트롤러 초기화  
    if (BehaviorTree && blackboardData)
    {
        UBlackboardComponent* blackboardComp = GetBlackboardComponent();

        if (blackboardComp)
        {
            UseBlackboard(blackboardData, blackboardComp);
            RunBehaviorTree(BehaviorTree);
        }
    }

    // 즉시 플레이어 따라가기 시작
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

    // 이 함수는 이제 UpdateMovementState에서 처리하므로 간단히 유지
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
    // AI 인식에 따른 처리 (예: 적 발견)
    // 현재 구현에서는 플레이어 지원 역할이므로 적 인식 로직은 제외
}

APawn* AAllyNPCAI::GetPlayerPawn()
{
    return UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
}

void AAllyNPCAI::UpdateCombatState(float DeltaTime)
{
    timeSinceLastShotDecision += DeltaTime;

    // 결정 주기가 지나지 않았으면 리턴
    if (timeSinceLastShotDecision < decisionUpdateInterval)
        return;

    timeSinceLastShotDecision = 0.0f;

    // 발사 조건 확인 및 결정
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

    const float distanceToPlayer = FVector::Dist(controlledAllyNPC->GetActorLocation(), playerPawn->GetActorLocation());
    
    // 디버그 로그
    GEngine->AddOnScreenDebugMessage
    (
        -1, 0.1f, FColor::Yellow, 
        FString::Printf(TEXT("플레이어와의 거리: %.2f / 팔로우 거리: %.2f"), distanceToPlayer, followDistance)
    );

    // 수정된 조건: 거리가 followDistance보다 클 때 플레이어를 따라가야 함
    if (distanceToPlayer > followDistance)
    {
        // AI를 통한 이동 - MoveToLocation 사용
        bool isRunning = distanceToPlayer > followDistance * 1.5f;
        
        // 이동 속도 설정
        if (controlledAllyNPC->GetCharacterMovement())
        {
            controlledAllyNPC->GetCharacterMovement()->MaxWalkSpeed = isRunning ? controlledAllyNPC->runSpeed : controlledAllyNPC->walkSpeed;
        }
        
        // 플레이어 위치로 이동 (회전 제어는 비활성화)
        FVector playerLocation = playerPawn->GetActorLocation();
        auto result = MoveToLocation(playerLocation, followDistance * 0.8f, true, true, false, false);
        
        // 애니메이션을 위한 이동 벡터 업데이트 (현재 이동 방향 사용)
        controlledAllyNPC->UpdateMovementState(isRunning, currentMovementDirection);
    }  

    else
    {
        // 가까이 있을 때는 이동 중지
        StopMovement();
        controlledAllyNPC->UpdateMovementState(false, FVector::ZeroVector);
    }
}

void AAllyNPCAI::UpdateMovementDirection(float DeltaTime)
{
    if (!controlledAllyNPC)
        return;

    timeSinceLastDirectionUpdate += DeltaTime;

    // 짧은 간격으로 이동 방향 업데이트
    if (timeSinceLastDirectionUpdate >= movementDirectionUpdateInterval)
    {
        FVector currentPosition = controlledAllyNPC->GetActorLocation();
        
        // 이동 거리 계산
        FVector movementDelta = currentPosition - lastPosition;
        float movementDistance = movementDelta.Size();
        
        // 최소 이동 거리 이상일 때만 방향 업데이트 (노이즈 방지)
        if (movementDistance > 0.01f)
        {
            currentMovementDirection = movementDelta.GetSafeNormal();
            
            // NPC를 이동 방향으로 회전시키기 (보간 적용)
            if (!currentMovementDirection.IsNearlyZero())
            {
                // 이동 방향을 회전값으로 변환
                FRotator targetRotation = currentMovementDirection.Rotation();
                
                // 피치와 롤은 0으로 설정하여 수평 유지
                targetRotation.Pitch = 0.0f;
                targetRotation.Roll = 0.0f;
                
                // 현재 회전값
                FRotator currentRotation = controlledAllyNPC->GetActorRotation();
                
                // 보간된 회전값 계산 (부드러운 회전)
                FRotator interpolatedRotation = FMath::RInterpTo(
                    currentRotation,       // 현재 회전
                    targetRotation,        // 목표 회전
                    DeltaTime,             // 델타 시간
                    rotationInterpSpeed    // 보간 속도
                );
                
                // 보간된 회전 적용
                controlledAllyNPC->SetActorRotation(interpolatedRotation);
                
                // 디버그 정보
                GEngine->AddOnScreenDebugMessage(
                    -1, 0.1f, FColor::Green,
                    FString::Printf(TEXT("보간 회전: Yaw=%.2f, 목표=%.2f, 이동 방향: X=%.2f, Y=%.2f"),
                    interpolatedRotation.Yaw, targetRotation.Yaw, 
                    currentMovementDirection.X, currentMovementDirection.Y)
                );
            }
        }
        
        // 위치 및 타이머 업데이트
        lastPosition = currentPosition;
        timeSinceLastDirectionUpdate = 0.0f;
    }
}

bool AAllyNPCAI::IsInFireRange() const
{
    // 플레이어와의 거리가 적절한지 확인 (여기서는 1000유닛 이내)
    const float DistanceToPlayer = FVector::Dist(controlledAllyNPC->GetActorLocation(), playerPawn->GetActorLocation());

    return DistanceToPlayer < 1000.0f;
}