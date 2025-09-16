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

/**
 * AAllyNPCAI - 동맹 NPC 캐릭터를 제어하는 AI 컨트롤러 클래스
 * 플레이어를 따라다니며 전투 지원을 제공하는 AI 로직 구현
 */
UCLASS()
class CONTRACT_API AAllyNPCAI : public ADetourCrowdAIController
{
	GENERATED_BODY()

public:
	// 생성자 및 기본 오버라이드 함수
	AAllyNPCAI();
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// AI 인식 시스템
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* AIPerceptionComp;

	// AI 행동 결정 시스템
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBehaviorTree* BehaviorTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBlackboardData* blackboardData;

	// AI 이동 파라미터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float followDistance = 500.0f;  // 플레이어로부터 유지할 거리

	// 플레이어 추적 기능
	UFUNCTION(BlueprintCallable, Category = "AI")
	void MoveToPlayer();  // 플레이어 위치로 이동 시작

	// 전투 관련 함수
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartFiring();  // 발사 시작

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StopFiring();   // 발사 중지

	// 인식 시스템 이벤트 핸들러
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

protected:
	// 기본 라이프사이클 함수
	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;

private:
	// 캐릭터 참조
	UPROPERTY()
	AAllyNPC* controlledAllyNPC;  // 현재 제어 중인 NPC

	UPROPERTY()
	APawn* playerPawn;  // 플레이어 캐릭터 참조

	// 전투 관련 변수
	bool isFiring;                         // 현재 발사 중인지 여부
	float timeSinceLastShotDecision;       // 마지막 발사 결정 이후 경과 시간
	float decisionUpdateInterval = 0.5f;   // 발사 결정 업데이트 간격

	// 이동 방향 계산 변수
	FVector lastPosition;                          // 이전 위치
	FVector currentMovementDirection;              // 현재 이동 방향
	float movementDirectionUpdateInterval = 0.01f; // 방향 업데이트 간격
	float timeSinceLastDirectionUpdate;            // 마지막 방향 업데이트 이후 경과 시간

	// 헬퍼 함수
	APawn* GetPlayerPawn();  // 플레이어 폰 참조 가져오기

	// 상태 업데이트 함수
	void UpdateCombatState(float DeltaTime);       // 전투 로직 업데이트
	void UpdateMovementState(float DeltaTime);     // 이동 로직 업데이트
	void UpdateMovementDirection(float DeltaTime); // 이동 방향 업데이트

	// 전투 유틸리티 함수
	bool IsInFireRange() const;  // 발사 가능 거리 확인

	// 이동 및 회전 파라미터
	float rotationInterpSpeed = 5.0f;  // 회전 보간 속도 (높을수록 빠르게 회전)

	// 부드러운 이동을 위한 속도 보간 변수
	float currentSpeed;   // 현재 적용된 이동 속도
	float targetSpeed;    // 목표 이동 속도
	float speedInterpRate;  // 속도 보간
};