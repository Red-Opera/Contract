#include "AllyNPCAI.h"
#include "AllyNPC.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AAllyNPCAI::AAllyNPCAI()
{
    PrimaryActorTick.bCanEverTick = true;

    // AI 인식 컴포넌트 설정
    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

    // Sight 감지 설정
    UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight Config"));
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 2000.0f;
    SightConfig->PeripheralVisionAngleDegrees = 60.0f;
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    AIPerceptionComp->ConfigureSense(*SightConfig);
    AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

    // 전투 변수 초기화
    isFiring = false;
    timeSinceLastShotDecision = 0.0f;
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

    // Perception 업데이트 콜백 등록
    AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AAllyNPCAI::OnPerceptionUpdated);
}

void AAllyNPCAI::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    // 회전 상태 업데이트 추가
    UpdateRotationState(deltaTime);

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

    // AI 컨트롤러 초기화  
    if (BehaviorTree && BlackboardData)
    {
        UBlackboardComponent* blackboardComp = GetBlackboardComponent();

        if (blackboardComp)
        {
            UseBlackboard(BlackboardData, blackboardComp);
            RunBehaviorTree(BehaviorTree);

            // 초기 Blackboard 값 설정  
            if (playerPawn)
            {
                blackboardComp->SetValueAsObject("PlayerTarget", playerPawn);
                blackboardComp->SetValueAsFloat("followDistance", followDistance);
            }
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

    // 플레이어 위치 기반으로 목표 지점 계산
    FVector PlayerLocation = playerPawn->GetActorLocation();
    FVector NPCLocation = controlledAllyNPC->GetActorLocation();
    
    // 플레이어 뒤쪽 적절한 거리로 목표 위치 설정
    FVector PlayerForward = playerPawn->GetActorForwardVector();
    FVector TargetLocation = PlayerLocation - (PlayerForward * followDistance * 0.8f); // 플레이어 뒤쪽
    
    // 높이는 플레이어와 동일하게
    TargetLocation.Z = PlayerLocation.Z;
    
    // MoveToActor 사용 (더 안정적)
    EPathFollowingRequestResult::Type Result = MoveToActor(playerPawn, followDistance * 0.7f);
    
    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Blue, FString::Printf(TEXT("이동 명령 결과: %d"), (int32)Result));
    
    // 캐릭터 속도 설정
    if (controlledAllyNPC->GetCharacterMovement())
    {
        float DistanceToPlayer = FVector::Dist(NPCLocation, PlayerLocation);
        bool bShouldRun = DistanceToPlayer > followDistance * 1.5f;
        
        controlledAllyNPC->GetCharacterMovement()->MaxWalkSpeed = bShouldRun ? controlledAllyNPC->runSpeed : controlledAllyNPC->walkSpeed;
    }
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
    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString::Printf(TEXT("플레이어와의 거리: %.2f / 팔로우 거리: %.2f"), distanceToPlayer, followDistance));

    // 수정된 조건: 거리가 followDistance보다 클 때 플레이어를 따라가야 함
    if (distanceToPlayer > followDistance)
    {
        // 플레이어를 따라가기
        bool isRunning = distanceToPlayer > followDistance * 1.5f;
        
        // 이동 방향 계산 (플레이어 방향으로)
        FVector moveDirection = (playerPawn->GetActorLocation() - controlledAllyNPC->GetActorLocation()).GetSafeNormal();
        
        // 목표 회전값 계산 및 설정
        FRotator lookAtRotation = FRotationMatrix::MakeFromX(moveDirection).Rotator();
        SetTargetRotation(lookAtRotation);
        
        // 캐릭터 이동 상태 업데이트
        controlledAllyNPC->UpdateMovementState(isRunning, moveDirection);
        
        // AI 이동 명령 실행 (회전이 완료되지 않았어도 이동 시작)
        MoveToPlayer();
        
        GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, FString::Printf(TEXT("플레이어 추적중 - %s"), isRunning ? TEXT("뛰기") : TEXT("걷기")));
    }   
    else
    {
        // 가까이 있을 때는 이동 중지하지만 플레이어를 바라보도록 회전
        StopMovement();
        controlledAllyNPC->UpdateMovementState(false, FVector::ZeroVector);
        
        // 플레이어 방향으로 회전 설정
        FVector LookDirection = (playerPawn->GetActorLocation() - controlledAllyNPC->GetActorLocation()).GetSafeNormal();
        FRotator LookAtRotation = FRotationMatrix::MakeFromX(LookDirection).Rotator();
        SetTargetRotation(LookAtRotation);
        
        GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, TEXT("이동 정지 - 플레이어 근처"));
    }
}

void AAllyNPCAI::UpdateRotationState(float DeltaTime)
{
    if (!controlledAllyNPC || !isRotating)
        return;

    FRotator CurrentRotation = controlledAllyNPC->GetActorRotation();
    
    // 목표 회전과 현재 회전의 차이 계산
    FRotator RotationDelta = targetRotation - CurrentRotation;
    RotationDelta.Normalize();
    
    // 회전 완료 체크
    if (FMath::Abs(RotationDelta.Yaw) <= rotationTolerance)
    {
        // 회전 완료
        controlledAllyNPC->SetActorRotation(targetRotation);
        isRotating = false;
        
        GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Cyan, TEXT("회전 완료"));
        return;
    }
    
    // 부드러운 회전 계산
    float RotationThisFrame = rotationSpeed * DeltaTime;
    
    // 회전 방향 결정 (최단 경로)
    float YawDelta = RotationDelta.Yaw;
    if (YawDelta > 180.0f)
        YawDelta -= 360.0f;
    else if (YawDelta < -180.0f)
        YawDelta += 360.0f;
    
    // 이번 프레임에서 회전할 각도 제한
    float ClampedYawDelta = FMath::Clamp(YawDelta, -RotationThisFrame, RotationThisFrame);
    
    // 새로운 회전값 적용
    FRotator NewRotation = CurrentRotation;
    NewRotation.Yaw += ClampedYawDelta;
    NewRotation.Normalize();
    
    controlledAllyNPC->SetActorRotation(NewRotation);
    
    // 디버그 정보
    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Orange, 
        FString::Printf(TEXT("회전중: %.1f -> %.1f (차이: %.1f)"), 
        CurrentRotation.Yaw, targetRotation.Yaw, YawDelta));
}

void AAllyNPCAI::SetTargetRotation(const FRotator& NewTargetRotation)
{
    // 새로운 목표 회전값이 현재 목표와 크게 다를 때만 업데이트
    FRotator CurrentTarget = targetRotation;
    FRotator RotationDiff = NewTargetRotation - CurrentTarget;
    RotationDiff.Normalize();
    
    // 회전 차이가 일정 값 이상일 때만 새로운 목표 설정
    if (FMath::Abs(RotationDiff.Yaw) > rotationTolerance || !isRotating)
    {
        targetRotation = NewTargetRotation;
        targetRotation.Normalize();
        isRotating = true;
        
        GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Purple, 
            FString::Printf(TEXT("새 목표 회전 설정: %.1f"), targetRotation.Yaw));
    }
}

bool AAllyNPCAI::IsInFireRange() const
{
    // 플레이어와의 거리가 적절한지 확인 (여기서는 1000유닛 이내)
    const float DistanceToPlayer = FVector::Dist(controlledAllyNPC->GetActorLocation(), playerPawn->GetActorLocation());

    return DistanceToPlayer < 1000.0f;
}