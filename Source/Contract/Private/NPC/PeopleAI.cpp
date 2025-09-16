#include "PeopleAI.h"
#include "NavigationSystem.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/PathFollowingComponent.h"

void APeopleAI::OnMoveCompleted(FAIRequestID requestID, const FPathFollowingResult& result)
{
	Super::OnMoveCompleted(requestID, result);

	// �ּ� ��� �ð��� �ִ� ��� �ð� ���̿��� ���� ��� �ð� ����
	float waitTime = FMath::RandRange(minWaitTime, maxWaitTime);

	isWait = true;

	// Ÿ�̸� ���� �α� �߰�
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, *FString::Printf(TEXT("Setting Timer for %f seconds"), waitTime));

	// ��� �ð� �Ŀ� StartSearchPlayer ȣ��
	GetWorld()->GetTimerManager().SetTimer(waitMoveTimer, this, &APeopleAI::ChangeWait, waitTime, false);
}

void APeopleAI::BeginPlay()
{
	Super::BeginPlay();

	navArea = FNavigationSystem::GetCurrent<UNavigationSystemV1>(this);
	movementComponent = GetPawn()->FindComponentByClass<UCharacterMovementComponent>();

	// �ִ� �̵� �ӷ��� ������
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
		// ��ȭ ���� �ƴ� ���� �̵�
		IncreaseMoveSpeed(deltaSeconds);
		LookAtLocation(toLocation);
	}

	// ��ȭ ���� �� �÷��̾ �ٶ�
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

	float accelerationRate = maxSpeed / increaseToMaxSpeedTime; // �ʴ� ������ �ӵ�
	float currentSpeed = movementComponent->MaxWalkSpeed;		// ���� �̵� �ӵ�

	currentSpeed = FMath::Min(currentSpeed + accelerationRate * deltaSecond, maxSpeed);
	movementComponent->MaxWalkSpeed = currentSpeed;
}

void APeopleAI::LookAtLocation(FVector targetLocation)
{
	APawn* monster = GetPawn();
	if (monster == nullptr)
		return;

	// ������ �̵� ���� ���͸� ������
	FVector monsterVelocity = monster->GetVelocity();

	// �̵� ������ ������ ȸ������ ����
	if (monsterVelocity.IsNearlyZero())
		return;

	// Z��(���� ��)�� ������ �����ϰ�, ���� ���͸� ����ȭ
	FVector direction = monsterVelocity.GetSafeNormal();
	direction.Z = 0.0f;

	// ��ǥ ȸ�� ��(�̵� ����)�� ���
	FRotator targetRotation = FRotationMatrix::MakeFromX(direction).Rotator();
	FRotator currentRotation = monster->GetActorRotation();

	// ���� ȸ�� ������ ��ǥ ȸ�� ������ ���� �����Ͽ� ȸ��
	FRotator newRotation = FMath::RInterpTo(currentRotation, targetRotation, GetWorld()->GetDeltaSeconds(), rotationSpeed);
	monster->SetActorRotation(newRotation);
}

void APeopleAI::InitiateConversation()
{
	FVector playerLocation = player->GetActorLocation();
	FVector aiLocation = GetPawn()->GetActorLocation();

	// �÷��̾ �ٶ󺸱�
	LookAtLocation(playerLocation);

	// �̵��� ����
	movementComponent->StopMovementImmediately();
	movementComponent->MaxWalkSpeed = 0.0f;

	isWait = true; // ��ȭ �� ��� ���·� ��ȯ
	isTalk = true;
}

void APeopleAI::EndConversation()
{
	isWait = false;						// ��ȭ ���� ����
	isTalk = false;

	StartMoveSpeed();					// �̵� �ӵ� �ʱ�ȭ
	GenerateRandomSearchLocation();		// ���� ��ġ ����
	MoveTo(toLocation);					// �̵� ����
}

void APeopleAI::CheckConversationTrigger()
{
	FVector playerLocation = player->GetActorLocation();
	FVector aiLocation = GetPawn()->GetActorLocation();

	// �Ÿ� ���
	float distanceToPlayer = FVector::Dist(aiLocation, playerLocation);

	// �Ÿ� ���� Ȯ��
	if (distanceToPlayer <= conversationTriggerDistance)
		InitiateConversation();
}

void APeopleAI::LookAtPlayer()
{
	FVector playerLocation = player->GetActorLocation();
	FVector aiLocation = GetPawn()->GetActorLocation();

	// ���� ���� ���
	FVector direction = (playerLocation - aiLocation).GetSafeNormal();
	direction.Z = 0.0f; // Z�� ���� (���� ȸ���� ����)

	// ��ǥ ȸ�� ���
	FRotator targetRotation = FRotationMatrix::MakeFromX(direction).Rotator();
	FRotator currentRotation = GetPawn()->GetActorRotation();

	// ���� �������� �ε巴�� ȸ��
	FRotator newRotation = FMath::RInterpTo(currentRotation, targetRotation, GetWorld()->GetDeltaSeconds(), rotationSpeed);
	GetPawn()->SetActorRotation(newRotation);
}

void APeopleAI::GenerateRandomSearchLocation()
{
	FVector playerLocation = player->GetActorLocation();
	float distanceToPlayer = FVector::Dist(playerLocation, GetPawn()->GetActorLocation());

	// �÷��̾�κ��� ���� �Ÿ� �̻� ������ �ִ� ���
	if (distanceToPlayer > playerMaxDistance)
	{
		// �÷��̾� �ֺ� ���� �Ÿ� ���� ������ ��ġ�� ����
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
		// �÷��̾���� �Ÿ��� ���ǿ� �������� ���� �� ���� ��ġ �ֺ��� ���� ������ ����
		FNavLocation resultLocation;
		bool isSucceed = navArea->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), 10000.0f, resultLocation);

		if (isSucceed)
		{
			toLocation = resultLocation.Location;
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, *FString::Printf(TEXT("Moving to random location: %s"), *toLocation.ToString()));
		}
	}
}