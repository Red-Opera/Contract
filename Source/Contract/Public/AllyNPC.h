#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AllyNPC.generated.h"

class USkeletalMeshComponent;
class UPawnSensingComponent;

UCLASS()
class CONTRACT_API AAllyNPC : public ACharacter
{
	GENERATED_BODY()

public:
	AAllyNPC();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float deltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* playerInputComponent) override;

	// AI 관련 컴포넌트에서 AllyNPCAI로 이동
	// 센싱 컴포넌트만 캐릭터에 남김
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UPawnSensingComponent* pawnSensingComp;

	// 무기 시스템
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* weaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName weaponSocketName;

	// 이동 속도 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float walkSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float runSpeed = 600.0f;

	// 애니메이션 파라미터
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	FVector2D movementVector;

	// 이동 상태 업데이트 함수 - AI 컨트롤러에서 호출
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void UpdateMovementState(bool isRunning, const FVector& direction);

	// 전투 기능
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartFiring();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StopFiring();

	// 무기 장착
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon();

private:
	bool isFiring;
	float timeSinceLastShot;
	float fireRate = 0.1f;

	void UpdateMovementVector(const FVector& Direction, bool bIsRunning);
	void FireWeapon();
};