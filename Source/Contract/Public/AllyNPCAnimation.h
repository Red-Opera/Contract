#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AllyNPCAnimation.generated.h"

/**
 * 총 사격하는 병사 캐릭터의 애니메이션 블루프린트를 위한 클래스입니다.
 * 이동, 사격, 재장전, 엄폐, 피격, 사망 등의 애니메이션 상태를 관리합니다.
 */
UCLASS()
class CONTRACT_API UAllyNPCAnimation : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAllyNPCAnimation();

	// 애니메이션 틱마다 호출되는 함수
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 애니메이션 초기화 함수
	virtual void NativeInitializeAnimation() override;

protected:
	// 소유 캐릭터의 참조
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	class ACharacter* owningCharacter;

	// 이동 관련 변수
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool isInAir;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool isCrouching;

	// 무기 관련 변수
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool isAiming;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool isFiring;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool isReloading;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool isEquipping;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	int32 currentWeaponType; // 0: 없음, 1: 권총, 2: 소총, 3: 샷건, 4: 스나이퍼

	// 전투 관련 변수
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool isHit;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool isDead;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool isThrowingGrenade;

	// 몽타주 재생 함수들
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayFireMontage();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayReloadMontage();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayEquipMontage();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayThrowGrenadeMontage();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayHitReactMontage(FName HitDirection = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayDeathMontage();

	// 특수 동작 변수
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool isPeekingLeft;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool isPeekingRight;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float aimOffsetYaw;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float aimOffsetPitch;

private:
	// 몽타주 레퍼런스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* fireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* reloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* equipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* throwGrenadeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* hitReactMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* deathMontage;

	// 목표물 조준 보간 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	float aimInterpSpeed = 15.0f;

	// IK 관련 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	bool enableHandIK;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandIKTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	FTransform RightHandIKTransform;

	// 사격 효과음 및 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class USoundBase* fireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* ejectedShell;
};