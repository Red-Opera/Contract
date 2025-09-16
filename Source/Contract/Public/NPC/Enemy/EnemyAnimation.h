#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimation.generated.h"

/**
 * 적 캐릭터의 애니메이션 블루프린트를 위한 클래스입니다.
 * 이동, 사격, 재장전, 엄폐, 피격, 사망 등의 애니메이션 상태를 관리합니다.
 */
UCLASS()
class CONTRACT_API UEnemyAnimation : public UAnimInstance
{
    GENERATED_BODY()

public:
    UEnemyAnimation();

    // 애니메이션 틱마다 호출되는 함수
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    // 애니메이션 초기화 함수
    virtual void NativeInitializeAnimation() override;

    // === IK 지원 함수 ===
    // 오른손 IK 타겟 위치 가져오기 (애니메이션 시스템에서 사용)
    UFUNCTION(BlueprintCallable, Category = "Animation")
    FTransform GetRightHandIKTransform() const;

    // 왼손 IK 타겟 위치 가져오기 (애니메이션 시스템에서 사용)
    UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe = "true"))
    FTransform GetLeftHandIKTransform() const;

    // 추가: 스레드 안전한 개별 값 반환 함수들
    UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe = "true"))
    FVector GetLeftHandIKLocation() const { return LeftHandIKLocation; }

    UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe = "true"))
    FRotator GetLeftHandIKRotation() const { return LeftHandIKRotation; }

    UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe = "true"))
    bool GetLeftHandIKValid() const { return isLeftHandIKValid; }

protected:
    // 소유 캐릭터의 참조
    UPROPERTY(BlueprintReadOnly, Category = "Character")
    class ACharacter* owningCharacter;

    // 이동 관련 변수
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float Speed;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float Direction;

    // 벡터 기반 이동 시스템 추가
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    FVector2D MovementVector;

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

    // 엄폐 관련 변수
    UPROPERTY(BlueprintReadWrite, Category = "Cover")
    bool isPeekingLeft;

    UPROPERTY(BlueprintReadWrite, Category = "Cover")
    bool isPeekingRight;

    // 조준 오프셋
    UPROPERTY(BlueprintReadOnly, Category = "Aiming")
    float aimOffsetYaw;

    UPROPERTY(BlueprintReadOnly, Category = "Aiming")
    float aimOffsetPitch;

    // IK 활성화 플래그
    UPROPERTY(BlueprintReadWrite, Category = "IK")
    bool enableHandIK;

    // 조준 보간 속도
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aiming")
    float aimInterpSpeed = 5.0f;

    // === 몽타주 관련 변수 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    class UAnimMontage* fireMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    class UAnimMontage* reloadMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    class UAnimMontage* equipMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    class UAnimMontage* throwGrenadeMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    class UAnimMontage* hitReactMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    class UAnimMontage* deathMontage;

    // === 이펙트 및 사운드 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
    class UNiagaraSystem* MuzzleFlash;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
    class UNiagaraSystem* ejectedShell;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
    class USoundBase* fireSound;

protected:
    // 캐시된 IK 데이터 (스레드 안전)
    UPROPERTY(BlueprintReadOnly, Category = "IK")
    FVector LeftHandIKLocation;

    UPROPERTY(BlueprintReadOnly, Category = "IK")
    FRotator LeftHandIKRotation;

    UPROPERTY(BlueprintReadOnly, Category = "IK")
    bool isLeftHandIKValid;

    UPROPERTY(BlueprintReadOnly, Category = "IK")
    FTransform cachedLeftHandIKTransform;

private:
    // IK 캐시 업데이트 함수
    void UpdateLeftHandIKCache();

public:
    // === 몽타주 재생 함수들 ===
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayFireMontage();

    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayReloadMontage();

    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayEquipMontage();

    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayThrowGrenadeMontage();

    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayHitReactMontage(FName HitDirection = FName("Front"));

    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayDeathMontage();
};
