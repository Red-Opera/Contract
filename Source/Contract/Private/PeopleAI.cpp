#include "PeopleAI.h"
#include "NavigationSystem.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/PathFollowingComponent.h"

void APeopleAI::OnMoveCompleted(FAIRequestID requestID, const FPathFollowingResult& result)
{
	Super::OnMoveCompleted(requestID, result);

	// 최소 대기 시간과 최대 대기 시간 사이에서 랜덤 대기 시간 선택
	float waitTime = FMath::RandRange(minWaitTime, maxWaitTime);

	isWait = true;

	// 타이머 설정 로그 추가
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, *FString::Printf(TEXT("Setting Timer for %f seconds"), waitTime));

	// 대기 시간 후에 StartSearchPlayer 호출
	GetWorld()->GetTimerManager().SetTimer(waitMoveTimer, this, &APeopleAI::ChangeWait, waitTime, false);
}

void APeopleAI::BeginPlay()
{
	Super::BeginPlay();

	navArea = FNavigationSystem::GetCurrent<UNavigationSystemV1>(this);
	movementComponent = GetPawn()->FindComponentByClass<UCharacterMovementComponent>();

	// 최대 이동 속력을 저장함
	maxSpeed = maxWalkSpeed;

	player = GetWorld()->GetFirstPlayerController()->GetPawn();

	if (player == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *FString::Printf(TEXT("Player not exist")));
		return;
	}

	UInputComponent* playerInputComponent = player->InputComponent;
	playerInputComponent->BindAction("Interaction", IE_Pressed, this, &APeopleAI::CheckConversationTrigger);

	GenerateRandomSearchLocation();
	StartMoveSpeed();
	MoveToLocation(toLocation);
}

void APeopleAI::Tick(float deltaSeconds)
{
	if (player == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *FString::Printf(TEXT("Player not exist")));
		return;
	}

	FVector playerLocation = player->GetActorLocation();
	FVector aiLocation = GetPawn()->GetActorLocation();

	if (!isWait)
	{
		// 대화 중이 아닐 때만 이동
		IncreaseMoveSpeed(deltaSeconds);
		LookAtLocation(toLocation);
	}

	// 대화 중일 때 플레이어를 바라봄
	else
		LookAtPlayer();
}

void APeopleAI::ChangeWait()
{
	isWait = false;
	isTalk = false;

	GenerateRandomSearchLocation();
	StartMoveSpeed();
	MoveTo(toLocation);
}

void APeopleAI::StartMoveSpeed()
{
	movementComponent->MaxWalkSpeed = 0.0f;
}

void APeopleAI::IncreaseMoveSpeed(float deltaSecond)
{
	if (movementComponent->MaxWalkSpeed >= maxSpeed)
		return;

	float accelerationRate = maxSpeed / increaseToMaxSpeedTime; // 초당 증가할 속도
	float currentSpeed = movementComponent->MaxWalkSpeed;		// 현재 이동 속도

	currentSpeed = FMath::Min(currentSpeed + accelerationRate * deltaSecond, maxSpeed);
	movementComponent->MaxWalkSpeed = currentSpeed;
}

void APeopleAI::LookAtLocation(FVector targetLocation)
{
	APawn* monster = GetPawn();
	if (monster == nullptr)
		return;

	// 몬스터의 이동 방향 벡터를 가져옴
	FVector monsterVelocity = monster->GetVelocity();

	// 이동 방향이 없으면 회전하지 않음
	if (monsterVelocity.IsNearlyZero())
		return;

	// Z축(상하 축)의 영향을 제거하고, 방향 벡터를 정규화
	FVector direction = monsterVelocity.GetSafeNormal();
	direction.Z = 0.0f;

	// 목표 회전 값(이동 방향)을 계산
	FRotator targetRotation = FRotationMatrix::MakeFromX(direction).Rotator();
	FRotator currentRotation = monster->GetActorRotation();

	// 현재 회전 값에서 목표 회전 값으로 선형 보간하여 회전
	FRotator newRotation = FMath::RInterpTo(currentRotation, targetRotation, GetWorld()->GetDeltaSeconds(), rotationSpeed);
	monster->SetActorRotation(newRotation);
}

void APeopleAI::InitiateConversation()
{
	FVector playerLocation = player->GetActorLocation();
	FVector aiLocation = GetPawn()->GetActorLocation();

	// 플레이어를 바라보기
	LookAtLocation(playerLocation);

	// 이동을 멈춤
	movementComponent->StopMovementImmediately();
	movementComponent->MaxWalkSpeed = 0.0f;

	isWait = true; // 대화 중 대기 상태로 전환
	isTalk = true;
}

void APeopleAI::EndConversation()
{
	isWait = false;						// 대화 상태 해제
	isTalk = false;

	StartMoveSpeed();					// 이동 속도 초기화
	GenerateRandomSearchLocation();		// 다음 위치 생성
	MoveTo(toLocation);					// 이동 시작
}

void APeopleAI::CheckConversationTrigger()
{
	FVector playerLocation = player->GetActorLocation();
	FVector aiLocation = GetPawn()->GetActorLocation();

	// 거리 계산
	float distanceToPlayer = FVector::Dist(aiLocation, playerLocation);

	// 거리 조건 확인
	if (distanceToPlayer <= conversationTriggerDistance)
		InitiateConversation();
}

void APeopleAI::LookAtPlayer()
{
	FVector playerLocation = player->GetActorLocation();
	FVector aiLocation = GetPawn()->GetActorLocation();

	// 방향 벡터 계산
	FVector direction = (playerLocation - aiLocation).GetSafeNormal();
	direction.Z = 0.0f; // Z축 무시 (수평 회전만 적용)

	// 목표 회전 계산
	FRotator targetRotation = FRotationMatrix::MakeFromX(direction).Rotator();
	FRotator currentRotation = GetPawn()->GetActorRotation();

	// 선형 보간으로 부드럽게 회전
	FRotator newRotation = FMath::RInterpTo(currentRotation, targetRotation, GetWorld()->GetDeltaSeconds(), rotationSpeed);
	GetPawn()->SetActorRotation(newRotation);
}

void APeopleAI::GenerateRandomSearchLocation()
{
	FVector playerLocation = player->GetActorLocation();
	float distanceToPlayer = FVector::Dist(playerLocation, GetPawn()->GetActorLocation());

	// 플레이어로부터 일정 거리 이상 떨어져 있는 경우
	if (distanceToPlayer > playerMaxDistance)
	{
		// 플레이어 주변 일정 거리 내의 랜덤한 위치를 선택
		FNavLocation resultLocation;
		bool isSucceed = navArea->GetRandomReachablePointInRadius(playerLocation, playerMaxDistance, resultLocation);

		if (isSucceed)
		{
			toLocation = resultLocation.Location;
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, *FString::Printf(TEXT("Moving to player vicinity: %s"), *toLocation.ToString()));
		}
	}

	else
	{
		// 플레이어와의 거리가 조건에 부합하지 않을 때 현재 위치 주변의 랜덤 지점을 선택
		FNavLocation resultLocation;
		bool isSucceed = navArea->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), 10000.0f, resultLocation);

		if (isSucceed)
		{
			toLocation = resultLocation.Location;
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, *FString::Printf(TEXT("Moving to random location: %s"), *toLocation.ToString()));
		}
	}
}