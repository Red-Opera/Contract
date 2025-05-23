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
	class ACharacter* OwningCharacter;

	// 이동 관련 변수
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching;

	// 무기 관련 변수
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bIsAiming;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bIsFiring;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bIsReloading;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bIsEquipping;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	int32 CurrentWeaponType; // 0: 없음, 1: 권총, 2: 소총, 3: 샷건, 4: 스나이퍼

	// 전투 관련 변수
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsInCover;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsHit;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsDead;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsThrowingGrenade;

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
	bool bIsPeekingLeft;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool bIsPeekingRight;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float AimOffsetYaw;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float AimOffsetPitch;

private:
	// 몽타주 레퍼런스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* FireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* EquipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* ThrowGrenadeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* DeathMontage;

	// 목표물 조준 보간 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	float AimInterpSpeed = 15.0f;

	// IK 관련 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	bool bEnableHandIK;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandIKTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	FTransform RightHandIKTransform;

	// 사격 효과음 및 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class USoundBase* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* EjectedShell;
};