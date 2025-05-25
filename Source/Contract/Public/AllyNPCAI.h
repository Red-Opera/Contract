#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIController.h"
#include "AllyNPCAI.generated.h"

class AAllyNPC;

UCLASS()
class CONTRACT_API AAllyNPCAI : public ADetourCrowdAIController
{
	GENERATED_BODY()

public:
	AAllyNPCAI();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// AI 관련 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* AIPerceptionComp;

	// 행동 트리 및 블랙보드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBehaviorTree* BehaviorTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBlackboardData* blackboardData;

	// AI 이동 및 전투 파라미터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float followDistance = 500.0f;

	// 플레이어 추적 관련 함수
	UFUNCTION(BlueprintCallable, Category = "AI")
	void MoveToPlayer();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartFiring();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StopFiring();

	// 적 감지 이벤트
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;

private:
	// AI가 제어하는 AllyNPC 캐릭터에 대한 참조
	UPROPERTY()
	AAllyNPC* controlledAllyNPC;

	UPROPERTY()
	APawn* playerPawn;

	bool isFiring;
	float timeSinceLastShotDecision;
	float decisionUpdateInterval = 0.5f;

	// 이동 방향 추적을 위한 변수
	FVector lastPosition;
	FVector currentMovementDirection;
	float movementDirectionUpdateInterval = 0.01f;
	float timeSinceLastDirectionUpdate;

	// 플레이어 찾기
	APawn* GetPlayerPawn();

	// 전투 상태 업데이트
	void UpdateCombatState(float DeltaTime);

	// 이동 상태 업데이트
	void UpdateMovementState(float DeltaTime);

	// 이동 방향 업데이트
	void UpdateMovementDirection(float DeltaTime);

	// 적절한 발사 거리인지 확인
	bool IsInFireRange() const;

	// 회전 보간 속도 (값이 클수록 더 빠르게 회전)
	float rotationInterpSpeed = 5.0f;
};