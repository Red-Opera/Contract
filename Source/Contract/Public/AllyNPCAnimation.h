#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AllyNPCAnimation.generated.h"

/**
 * �� ����ϴ� ���� ĳ������ �ִϸ��̼� �������Ʈ�� ���� Ŭ�����Դϴ�.
 * �̵�, ���, ������, ����, �ǰ�, ��� ���� �ִϸ��̼� ���¸� �����մϴ�.
 */
UCLASS()
class CONTRACT_API UAllyNPCAnimation : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAllyNPCAnimation();

	// �ִϸ��̼� ƽ���� ȣ��Ǵ� �Լ�
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// �ִϸ��̼� �ʱ�ȭ �Լ�
	virtual void NativeInitializeAnimation() override;

protected:
	// ���� ĳ������ ����
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	class ACharacter* OwningCharacter;

	// �̵� ���� ����
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching;

	// ���� ���� ����
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bIsAiming;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bIsFiring;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bIsReloading;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	bool bIsEquipping;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	int32 CurrentWeaponType; // 0: ����, 1: ����, 2: ����, 3: ����, 4: ��������

	// ���� ���� ����
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsInCover;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsHit;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsDead;

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bIsThrowingGrenade;

	// ��Ÿ�� ��� �Լ���
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

	// Ư�� ���� ����
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool bIsPeekingLeft;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool bIsPeekingRight;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float AimOffsetYaw;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float AimOffsetPitch;

private:
	// ��Ÿ�� ���۷���
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

	// ��ǥ�� ���� ���� �ӵ�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	float AimInterpSpeed = 15.0f;

	// IK ���� ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	bool bEnableHandIK;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandIKTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IK", meta = (AllowPrivateAccess = "true"))
	FTransform RightHandIKTransform;

	// ��� ȿ���� �� ����Ʈ
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class USoundBase* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* EjectedShell;
};