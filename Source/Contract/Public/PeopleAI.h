#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "PeopleAI.generated.h"

UCLASS()
class CONTRACT_API APeopleAI : public ADetourCrowdAIController
{
	GENERATED_BODY()
	
public:
	void OnMoveCompleted(FAIRequestID requestID, const FPathFollowingResult& result) override;

	// ==============================================================
	//							Public Variable
	// ==============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float maxWalkSpeed = 150.0f;

	UPROPERTY(EditAnywhere, Category = "AI")
	float minWaitTime = 1.0f;					// 최소 대기 시간 (초)

	UPROPERTY(EditAnywhere, Category = "AI")
	float maxWaitTime = 3.0f;					// 최대 대기 시간 (초)

	UPROPERTY(EditAnywhere, Category = "AI")
	float playerMaxDistance = 10000.0f;			// 플레이어부터 떨어진 거리

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float conversationTriggerDistance = 300.0f; // 대화를 트리거하는 거리

	UPROPERTY(EditAnywhere, Category = "AI")
	float rotationSpeed = 5.0f; // 회전 속도 (초당 도)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool isTalk = false;

protected:
	void BeginPlay() override;
	void Tick(float deltaSeconds);

private:
	void GenerateRandomSearchLocation();
	void ChangeWait();

	void StartMoveSpeed();
	void IncreaseMoveSpeed(float deltaSecond);
	void LookAtLocation(FVector targetLocation);

	void InitiateConversation();
	void EndConversation();
	void CheckConversationTrigger();
	void LookAtPlayer();

	class UNavigationSystemV1* navArea;						// 네비게이션 시스템
	class UCharacterMovementComponent* movementComponent;	// 이동 관련 컴포넌트

	FTimerHandle waitMoveTimer;								// 다음 이동 타임머

	FVector toLocation = FVector();							// 사람이 이동할 위치

	float maxSpeed;											// 사람의 최대 이동 속도
	float increaseToMaxSpeedTime = 0.5f;					// 최대 속력까지 증가하는데 걸리는 시간

	AActor* player;							// 플레이어

	bool isWait;
};
