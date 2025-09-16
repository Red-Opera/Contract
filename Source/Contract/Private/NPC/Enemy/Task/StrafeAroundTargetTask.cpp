#include "StrafeAroundTargetTask.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"

UStrafeAroundTargetTask::UStrafeAroundTargetTask()
{
    NodeName = TEXT("Retreat From Target");
    
    bNotifyTick = true;
    bCreateNodeInstance = true;
    
    // 블랙보드 키 설정
    targetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UStrafeAroundTargetTask, targetActorKey), AActor::StaticClass());
    fireDistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UStrafeAroundTargetTask, fireDistanceKey));
    
    // 기본값 설정
    isUseFireDistanceCondition = false;
    fireDistanceThreshold = 800.0f;
    retreatStartDistance = 700.0f;
    retreatTargetDistance = 900.0f;
    retreatSpeed = 600.0f;
    distanceTolerance = 80.0f;
    maxExecutionTime = 15.0f;
}

EBTNodeResult::Type UStrafeAroundTargetTask::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
    // 메모리 초기화
    FStrafeAroundTargetTaskMemory* taskMemory = reinterpret_cast<FStrafeAroundTargetTaskMemory*>(nodeMemory);
    taskMemory->Initialize();
    
    // 필수 컴포넌트 확인
    AAIController* aiController = ownerComp.GetAIOwner();
    APawn* controlledPawn = aiController ? aiController->GetPawn() : nullptr;

    if (!aiController || !controlledPawn)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NullAIController, StrafeAroundTargetTask.cpp) : AI Controller 또는 Pawn이 없습니다!"));

        return EBTNodeResult::Failed;
    }

    // FireDistance 조건 확인
    if (isUseFireDistanceCondition && !CheckFireDistanceCondition(ownerComp))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            TEXT("Warning (FireDistanceCondition, StrafeAroundTargetTask.cpp) : FireDistance 조건 불만족 - 태스크 종료"));

        return EBTNodeResult::Failed;
    }

    // 타겟 정보 확인
    FVector targetLocation;
    float currentDistance;
    
    if (!GetTargetInfo(ownerComp, targetLocation, currentDistance))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (NoValidTarget, StrafeAroundTargetTask.cpp) : 유효한 타겟을 찾을 수 없습니다!"));

        return EBTNodeResult::Failed;
    }
    
    // 메모리에 정보 저장
    taskMemory->startTime = ownerComp.GetWorld()->GetTimeSeconds();
    
    // 캐릭터 이동 속도 설정
    if (ACharacter* character = Cast<ACharacter>(controlledPawn))
    {
        if (UCharacterMovementComponent* movementComp = character->GetCharacterMovement())
            movementComp->MaxWalkSpeed = retreatSpeed;
    }

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
        FString::Printf(TEXT("StrafeAroundTargetTask: 후퇴 태스크 시작 - 현재 거리: %.1f"), currentDistance));

    return EBTNodeResult::InProgress;
}

void UStrafeAroundTargetTask::TickTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds)
{
    FStrafeAroundTargetTaskMemory* taskMemory = reinterpret_cast<FStrafeAroundTargetTaskMemory*>(nodeMemory);
    
    AAIController* aiController = ownerComp.GetAIOwner();
    APawn* controlledPawn = aiController ? aiController->GetPawn() : nullptr;

    if (aiController == nullptr || controlledPawn == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
            TEXT("Error (NullAIController, StrafeAroundTargetTask.cpp) : AIController 또는 ControlledPawn이 없습니다!"));
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);

        return;
    }
    
    // 최대 실행 시간 체크
    float currentTime = ownerComp.GetWorld()->GetTimeSeconds();
    float elapsedTime = currentTime - taskMemory->startTime;
    
    if (elapsedTime >= maxExecutionTime)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
            TEXT("Info (MaxExecutionTimeReached, StrafeAroundTargetTask.cpp) : 최대 실행 시간 도달 - 종료"));
        FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);

        return;
    }
    
    // FireDistance 조건 지속 확인
    if (isUseFireDistanceCondition && !CheckFireDistanceCondition(ownerComp))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            TEXT("Warning (FireDistanceConditionChanged, StrafeAroundTargetTask.cpp) : FireDistance 조건 변경 - 태스크 종료"));
        FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);

        return;
    }
    
    // 타겟 정보 업데이트
    FVector targetLocation;
    float currentDistance;
    
    if (!GetTargetInfo(ownerComp, targetLocation, currentDistance))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("Error (TargetInfoUpdateFailed, StrafeAroundTargetTask.cpp) : 타겟 정보를 가져올 수 없습니다!"));
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);

        return;
    }
    
    // 거리 유효성 체크
    if (currentDistance <= 0.0f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            FString::Printf(TEXT("Error (InvalidDistance, StrafeAroundTargetTask.cpp) : 비정상적인 거리 값: %.1f"), currentDistance));
        FinishLatentTask(ownerComp, EBTNodeResult::Failed);

        return;
    }
    
    // 후퇴 필요성 확인
    if (!ShouldRetreat(currentDistance))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
            FString::Printf(TEXT("Success (SafeDistanceAchieved, StrafeAroundTargetTask.cpp) : 안전 거리 확보 (%.1f) - 성공 종료"), currentDistance));
        FinishLatentTask(ownerComp, EBTNodeResult::Succeeded);

        return;
    }
    
    // 후퇴 위치 계산
    FVector currentLocation = controlledPawn->GetActorLocation();
    FVector retreatPosition = CalculateRetreatPosition(currentLocation, targetLocation, currentDistance);
    
    // 후퇴 위치 유효성 검사
    if (retreatPosition.IsZero() || FVector::Dist(currentLocation, retreatPosition) < 10.0f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            FString::Printf(TEXT("Warning (InvalidRetreatPosition, StrafeAroundTargetTask.cpp) : 후퇴 위치가 부적절합니다. Current: %s, Retreat: %s"), 
            *currentLocation.ToString(), *retreatPosition.ToString()));
        
        // 강제로 후퇴 위치 재계산
        FVector awayFromTarget = (currentLocation - targetLocation).GetSafeNormal();
        retreatPosition = currentLocation + (awayFromTarget * 200.0f);
    }
    
    // 네비게이션 가능한 위치로 조정
    FVector navigablePosition = FindNavigablePosition(retreatPosition, ownerComp);
    
    if (navigablePosition.IsZero())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
            TEXT("Warning (NavigationPositionNotFound, StrafeAroundTargetTask.cpp) : 네비게이션 위치를 찾을 수 없음 - 원본 위치 사용"));

        navigablePosition = retreatPosition;
    }
    
    // 이동 로직
    float moveDistance = FVector::Dist(currentLocation, navigablePosition);
    float timeSinceLastMove = currentTime - taskMemory->lastMoveTime;
    
    // 이동 조건 체크
    bool shouldMove = false;
    
    if (moveDistance >= 10.0f)
    {
        if (!taskMemory->isMoving) 
            shouldMove = true;

        else if (timeSinceLastMove > 0.5f)
        {
            float distanceToCurrentTarget = FVector::Dist(navigablePosition, taskMemory->currentTargetPosition);

            if (distanceToCurrentTarget > 25.0f)
                shouldMove = true;
        }

        else if (timeSinceLastMove > 1.5f)
            shouldMove = true;
    }
    
    if (shouldMove)
    {
        // 이동 요청 생성 및 실행
        taskMemory->currentTargetPosition = navigablePosition;
        taskMemory->lastMoveTime = currentTime;
        
        FAIMoveRequest moveRequest;
        moveRequest.SetGoalLocation(navigablePosition);
        moveRequest.SetAcceptanceRadius(distanceTolerance * 0.8f);
        moveRequest.SetUsePathfinding(true);
        moveRequest.SetAllowPartialPath(true);
        moveRequest.SetReachTestIncludesAgentRadius(true);
        moveRequest.SetCanStrafe(true);
        moveRequest.SetProjectGoalLocation(true);
        moveRequest.SetNavigationFilter(nullptr);
        
        FPathFollowingRequestResult moveRequestResult = aiController->MoveTo(moveRequest);
        
        if (moveRequestResult.Code == EPathFollowingRequestResult::RequestSuccessful)
        {
            taskMemory->moveRequestID = moveRequestResult.MoveId;
            taskMemory->isMoving = true;
            
            GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green,
                FString::Printf(TEXT("Success (MoveRequested, StrafeAroundTargetTask.cpp) : 이동 요청 성공 - 거리 %.1f"), moveDistance));
        }

        else if (moveRequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
            taskMemory->isMoving = false;

        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Orange,
                FString::Printf(TEXT("Warning (MoveRequestFailed, StrafeAroundTargetTask.cpp) : 이동 실패하지만 계속 시도 (코드: %d)"), (int32)moveRequestResult.Code));
            
            taskMemory->isMoving = false;
            taskMemory->lastMoveTime = currentTime - 1.0f;
        }
    }
    
    // 이동 상태 체크
    if (taskMemory->isMoving && taskMemory->moveRequestID.IsValid())
    {
        if (UPathFollowingComponent* pathComp = aiController->GetPathFollowingComponent())
        {
            EPathFollowingStatus::Type status = pathComp->GetStatus();

            if (status == EPathFollowingStatus::Moving || status == EPathFollowingStatus::Waiting)
            {
                // 정상 이동 중
            }

            else if (status == EPathFollowingStatus::Idle)
            {
                taskMemory->isMoving = false;
                taskMemory->moveRequestID = FAIRequestID::InvalidRequest;
                
                GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan,
                    TEXT("Info (MovementCompleted, StrafeAroundTargetTask.cpp) : 이동 완료"));
            }

            else if (status == EPathFollowingStatus::Paused)
            {
                if (timeSinceLastMove > 2.0f)
                {
                    taskMemory->isMoving = false;
                    taskMemory->moveRequestID = FAIRequestID::InvalidRequest;
                    
                    GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow,
                        TEXT("Warning (MovementPaused, StrafeAroundTargetTask.cpp) : 일시정지 상태 - 재시도 준비"));
                }
            }
        }
    }

    else
    {
        // 이동 정지 상태 감지 시 강제 재시작
        if (timeSinceLastMove > 2.0f && moveDistance > 50.0f)
        {
            taskMemory->isMoving = false;
            taskMemory->lastMoveTime = currentTime - 1.0f;
            
            GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Magenta,
                TEXT("Warning (MovementStalled, StrafeAroundTargetTask.cpp) : 이동 정지 상태 감지 - 강제 재시작"));
        }
    }
    
    // 디버그 정보
    GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange,
        FString::Printf(TEXT("Retreat: Dist %.1f (Target: %.1f) MoveDist: %.1f %s"), 
        currentDistance, retreatTargetDistance, moveDistance, 
        taskMemory->isMoving ? TEXT("Moving") : TEXT("Idle")));
}

uint16 UStrafeAroundTargetTask::GetInstanceMemorySize() const
{
    return sizeof(FStrafeAroundTargetTaskMemory);
}

void UStrafeAroundTargetTask::OnTaskFinished(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, EBTNodeResult::Type taskResult)
{
    // 이동 중단
    if (AAIController* aiController = ownerComp.GetAIOwner())
        aiController->StopMovement();
    
    // 메모리 정리
    FStrafeAroundTargetTaskMemory* taskMemory = reinterpret_cast<FStrafeAroundTargetTaskMemory*>(nodeMemory);
    taskMemory->isMoving = false;
    taskMemory->moveRequestID = FAIRequestID::InvalidRequest;
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        FString::Printf(TEXT("StrafeAroundTargetTask: 종료 - Result: %d"), (int32)taskResult));
    
    Super::OnTaskFinished(ownerComp, nodeMemory, taskResult);
}

// 헬퍼 함수들

bool UStrafeAroundTargetTask::CheckFireDistanceCondition(UBehaviorTreeComponent& ownerComp) const
{
    // FireDistance 사용이 비활성화된 경우 항상 true 반환
    if (!isUseFireDistanceCondition)
        return true;
    
    UBlackboardComponent* blackboardComp = ownerComp.GetBlackboardComponent();

    if (blackboardComp == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            TEXT("Warning (NullBlackboard, StrafeAroundTargetTask.cpp) : BlackboardComponent가 없음 - FireDistance 조건 무시"));

        return true;
    }
    
    // FireDistance 키 확인
    if (fireDistanceKey.SelectedKeyName.IsNone())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            TEXT("Warning (NoFireDistanceKey, StrafeAroundTargetTask.cpp) : FireDistance 키가 설정되지 않음 - 조건 무시하고 실행"));

        return true;
    }
    
    float fireDistance = blackboardComp->GetValueAsFloat(fireDistanceKey.SelectedKeyName);
    
    // FireDistance 값 유효성 검사
    if (fireDistance <= 0.0f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            FString::Printf(TEXT("Warning (InvalidFireDistance, StrafeAroundTargetTask.cpp) : FireDistance 값이 유효하지 않음 (%.1f) - 조건 무시"), fireDistance));

        return true;
    }
    
    bool conditionMet = fireDistance <= fireDistanceThreshold;
    
    // FireDistance 디버깅 정보
    static float lastLogTime = 0.0f;
    float currentTime = ownerComp.GetWorld()->GetTimeSeconds();

    if (currentTime - lastLogTime > 1.0f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.f, conditionMet ? FColor::Green : FColor::Red,
            FString::Printf(TEXT("FireDistance: %.1f (임계값: %.1f) - %s"), 
            fireDistance, fireDistanceThreshold, conditionMet ? TEXT("조건 만족") : TEXT("조건 불만족")));

        lastLogTime = currentTime;
    }
    
    return conditionMet;
}

bool UStrafeAroundTargetTask::GetTargetInfo(UBehaviorTreeComponent& ownerComp, FVector& outTargetLocation, float& outCurrentDistance) const
{
    UBlackboardComponent* blackboardComp = ownerComp.GetBlackboardComponent();
    APawn* controlledPawn = ownerComp.GetAIOwner()->GetPawn();

    if (!blackboardComp || !controlledPawn)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
            TEXT("Error (NullComponent, StrafeAroundTargetTask.cpp) : BlackboardComp 또는 ControlledPawn이 없습니다!"));

        return false;
    }

    // 블랙보드 키 체크
    if (targetActorKey.SelectedKeyName.IsNone())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
            TEXT("Error (NoTargetActorKey, StrafeAroundTargetTask.cpp) : TargetActorKey가 설정되지 않았습니다!"));

        return false;
    }

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, 
        FString::Printf(TEXT("설정된 타겟 키: %s"), *targetActorKey.SelectedKeyName.ToString()));

    // 타겟 액터 가져오기
    UObject* targetObject = blackboardComp->GetValueAsObject(targetActorKey.SelectedKeyName);
    AActor* targetActor = Cast<AActor>(targetObject);

    // 타겟이 자기 자신이거나 없는 경우 플레이어 검색
    if (!targetActor || targetActor == controlledPawn || targetActorKey.SelectedKeyName.ToString().Contains(TEXT("Self")))
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, 
            TEXT("Warning (SelfTarget, StrafeAroundTargetTask.cpp) : 타겟이 자기 자신이거나 없음 - 플레이어 검색 시도"));

        // 일반적인 플레이어 키들 확인
        TArray<FName> playerKeys = {
            TEXT("PlayerCharacter"), 
            TEXT("Player"), 
            TEXT("TargetActor"),
            TEXT("Target"),
            TEXT("PlayerPawn")
        };
        
        bool foundPlayer = false;

        for (const FName& keyName : playerKeys)
        {
            UObject* testObj = blackboardComp->GetValueAsObject(keyName);
            AActor* testActor = Cast<AActor>(testObj);
            
            if (testActor && testActor != controlledPawn)
            {
                targetActor = testActor;
                foundPlayer = true;
                
                GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, 
                    FString::Printf(TEXT("플레이어 발견: %s 키에서 %s"), 
                    *keyName.ToString(), *testActor->GetName()));

                break;
            }
        }
        
        // 블랙보드에서 플레이어를 찾지 못한 경우 월드에서 직접 검색
        if (!foundPlayer)
        {
            UWorld* world = ownerComp.GetWorld();

            if (world == nullptr)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (NullWorld, StrafeAroundTargetTask.cpp) : 월드가 유효하지 않습니다!"));

                return false;
            }

            APawn* playerPawn = world->GetFirstPlayerController()->GetPawn();

            if (playerPawn == nullptr)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (NullPlayerPawn, StrafeAroundTargetTask.cpp) : 플레이어 Pawn이 유효하지 않습니다!"));

                return false;
			}

            if (playerPawn != controlledPawn)
            {
                targetActor = playerPawn;
                foundPlayer = true;

                GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
                    FString::Printf(TEXT("Success (PlayerFoundInWorld, StrafeAroundTargetTask.cpp) : 월드에서 플레이어 발견: %s"), *playerPawn->GetName()));

                // 블랙보드에 플레이어 정보 업데이트
                for (const FName& keyName : playerKeys)
                    blackboardComp->SetValueAsObject(keyName, playerPawn);
            }
        }
        
        if (!foundPlayer)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
                TEXT("Error (PlayerNotFound, StrafeAroundTargetTask.cpp) : 플레이어를 찾을 수 없습니다!"));

            return false;
        }
    }

    if (!IsValid(targetActor))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
            TEXT("Error (InvalidTargetActor, StrafeAroundTargetTask.cpp) : TargetActor가 유효하지 않습니다!"));

        return false;
    }

    FVector currentLocation = controlledPawn->GetActorLocation();
    outTargetLocation = targetActor->GetActorLocation();
    outCurrentDistance = FVector::Dist(currentLocation, outTargetLocation);
    
    // 상세한 위치 정보 디버깅
    static float lastLogTime = 0.0f;
    float currentTime = ownerComp.GetWorld()->GetTimeSeconds();
    if (currentTime - lastLogTime > 2.0f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, 
            FString::Printf(TEXT("타겟: %s"), *targetActor->GetName()));

        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, 
            FString::Printf(TEXT("현재 위치: %s"), *currentLocation.ToString()));

        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, 
            FString::Printf(TEXT("타겟 위치: %s"), *outTargetLocation.ToString()));

        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, 
            FString::Printf(TEXT("계산된 거리: %.1f"), outCurrentDistance));
        lastLogTime = currentTime;
    }
    
    // 거리가 0인 경우 처리
    if (outCurrentDistance <= 0.0f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
            FString::Printf(TEXT("Error (ZeroDistance, StrafeAroundTargetTask.cpp) : 여전히 거리가 0입니다! 타겟: %s"), *targetActor->GetName()));

        return false;
    }
    
    return true;
}

bool UStrafeAroundTargetTask::ShouldRetreat(float currentDistance) const
{
    return currentDistance < retreatTargetDistance;
}

FVector UStrafeAroundTargetTask::CalculateRetreatPosition(const FVector& currentLocation, const FVector& targetLocation, float currentDistance) const
{
    // 타겟에서 멀어지는 방향 계산
    FVector awayFromTarget = (currentLocation - targetLocation).GetSafeNormal();
    
    // 후퇴 강도 계산
    float retreatIntensity = FMath::Clamp((retreatTargetDistance - currentDistance) / retreatTargetDistance, 0.1f, 1.0f);
    
    // 후퇴 거리 계산
    float baseRetreatDistance = 150.0f;
    float retreatDistance = baseRetreatDistance * retreatIntensity;
    retreatDistance = FMath::Max(retreatDistance, 50.0f);
    
    // 랜덤성 추가
    float randomAngle = FMath::RandRange(-15.0f, 15.0f);
    FVector rotatedDirection = awayFromTarget.RotateAngleAxis(randomAngle, FVector::UpVector);
    
    return currentLocation + (rotatedDirection * retreatDistance);
}

FVector UStrafeAroundTargetTask::FindNavigablePosition(const FVector& desiredPosition, UBehaviorTreeComponent& ownerComp) const
{
    UNavigationSystemV1* navSystem = UNavigationSystemV1::GetCurrent(ownerComp.GetWorld());

    if (navSystem == nullptr)
        return desiredPosition;
    
    FNavLocation navLocation;
    
    // 원하는 위치에서 네비게이션 가능한 위치 찾기
    if (navSystem->ProjectPointToNavigation(desiredPosition, navLocation, FVector(200.0f, 200.0f, 500.0f)))
        return navLocation.Location;
    
    // 주변에서 네비게이션 가능한 위치 찾기
    TArray<float> searchRadii = {150.0f, 300.0f, 500.0f};
    
    for (float radius : searchRadii)
    {
        if (navSystem->GetRandomReachablePointInRadius(desiredPosition, radius, navLocation))
            return navLocation.Location;
    }
    
    return FVector::ZeroVector;
}