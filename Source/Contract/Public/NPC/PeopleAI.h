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
	float minWaitTime = 1.0f;					// �ּ� ��� �ð� (��)

	UPROPERTY(EditAnywhere, Category = "AI")
	float maxWaitTime = 3.0f;					// �ִ� ��� �ð� (��)

	UPROPERTY(EditAnywhere, Category = "AI")
	float playerMaxDistance = 10000.0f;			// �÷��̾���� ������ �Ÿ�

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float conversationTriggerDistance = 300.0f; // ��ȭ�� Ʈ�����ϴ� �Ÿ�

	UPROPERTY(EditAnywhere, Category = "AI")
	float rotationSpeed = 5.0f; // ȸ�� �ӵ� (�ʴ� ��)

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

	class UNavigationSystemV1* navArea;						// �׺���̼� �ý���
	class UCharacterMovementComponent* movementComponent;	// �̵� ���� ������Ʈ

	FTimerHandle waitMoveTimer;								// ���� �̵� Ÿ�Ӹ�

	FVector toLocation = FVector();							// ����� �̵��� ��ġ

	float maxSpeed;											// ����� �ִ� �̵� �ӵ�
	float increaseToMaxSpeedTime = 0.5f;					// �ִ� �ӷ±��� �����ϴµ� �ɸ��� �ð�

	AActor* player;							// �÷��̾�

	bool isWait;
};
