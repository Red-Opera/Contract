// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Gun.h"
#include "Engine/TimerHandle.h"
#include "Enemy.generated.h"

// 전방 선언
class UPawnSensingComponent;
class AAIController;
class UBlackboardComponent;
class UAnimMontage;
class AFloatingDamage;
class UIDToItem;

/**
 * AEnemy - TPS Kit GASP 시스템을 구현한 적 캐릭터 클래스
 * 전투, 경계, 패트롤 상태를 가지며 AI 컨트롤러와 연동되어 동작
 */
UCLASS()
class CONTRACT_API AEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    // Sets default values for this character's properties
    AEnemy();

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // === 데미지 시스템 ===
    void SetDamage(FVector hitLocation, int damage);

    // 데미지 표기 나이아가라
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<AFloatingDamage> damageParticle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    float textOffset = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TArray<UTexture*> digitImage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    bool isDead = false;

    // === HP 시스템 ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float maxHP = 1000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    float currentHP = 1000.0f;

    // === 아이템 시스템 ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
    TArray<int> itemCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
    int money = 0;

    // === 🔧 TPS Kit GASP 시스템 - 전투 상태 (중복 제거됨) ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat AI")
    bool isInCombat = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat AI")
    bool isFiring = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat AI")
    bool isBurstFiring = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat AI")
    bool canBurstFire = true;

    // 현재 타겟 (플레이어)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat AI")
    AActor* currentTarget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat AI")
    float CurrentAmmo = 30.0f;  // UpdateBlackboardValues에서 사용

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat AI")
    float MaxAmmo = 30.0f;  // BeginPlay에서 사용

    // === 전투 설정 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat AI")
    float engagementDistance = 1200.0f;  // 교전 시작 거리

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat AI")
    float disengagementDistance = 1500.0f;  // 교전 해제 거리

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat AI")
    float optimalCombatDistance = 800.0f;  // 최적 전투 거리

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat AI")
    float burstFireDuration = 3.0f;  // 버스트 파이어 지속 시간

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat AI")
    float burstFireCooldown = 5.0f;  // 버스트 파이어 쿨다운

    // === 타겟 메모리 시스템 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat AI")
    float targetMemoryDuration = 7.0f;  // 타겟 손실 후 기억 시간

    // 마지막으로 타겟을 본 위치
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat AI")
    FVector lastKnownTargetLocation;

    // 타겟을 마지막으로 본 시간
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat AI")
    float lastTargetVisibleTime = 0.0f;

    // === 애니메이션 시스템 ===
    // 애니메이션 파라미터 - 이동 벡터 (X: 전/후, Y: 좌/우)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    FVector2D movementVector;

    // === 무기 시스템 ===
    // Gun 액터 참조 - 유일한 무기 시스템
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    AGun* equippedGun;

    // 장착할 Gun 블루프린트 클래스 - 블루프린트에서 설정 가능
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TSubclassOf<AGun> gunBlueprint;

    // 무기 장착 소켓들 - 블루프린트에서 수정 가능
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FName rightHandSocketName = TEXT("RightGunTarget"); // 오른손 무기 소켓

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FName leftHandSocketName = TEXT("LeftGunTarget"); // 왼손 소켓 (장착용)

    // Gun 위치 조정을 위한 오프셋 트랜스폼 - 블루프린트에서 조정 가능
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FTransform gunAttachOffset; // Gun을 소켓에 부착할 때 추가 오프셋

    // === 발사 시스템 ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    bool isFire = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float fireRate = 0.1f;

    // === 이동 시스템 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float walkSpeed = 250.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float runSpeed = 500.0f;

    // === TPS Kit GASP 시스템 - 전투 함수 ===
    UFUNCTION(BlueprintCallable, Category = "Combat AI")
    void EnterCombatMode(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Combat AI")
    void ExitCombatMode();

    UFUNCTION(BlueprintCallable, Category = "Combat AI")
    void StartBurstFire();

    UFUNCTION(BlueprintCallable, Category = "Combat AI")
    void StopBurstFire();

    UFUNCTION(BlueprintCallable, Category = "Combat AI")
    void SetTargetActor(AActor* Target);

    // 타겟 유효성 확인
    UFUNCTION(BlueprintCallable, Category = "Combat AI")
    bool HasValidTarget() const;

    // 타겟과의 거리 확인
    UFUNCTION(BlueprintCallable, Category = "Combat AI")
    float GetDistanceToTarget() const;

    // 최적 전투 거리 내에 있는지 확인
    UFUNCTION(BlueprintCallable, Category = "Combat AI")
    bool IsInOptimalCombatRange() const;

    // === 🔧 상태 확인 함수들 (StartFireTask에서 사용) ===
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsFiring() const { return isFiring; }
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsBurstFiring() const { return isBurstFiring; }
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsInCombat() const { return isInCombat; }
    
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool CanBurstFire() const { return canBurstFire; }

    // === 무기 관련 함수 ===
    // Gun 장착/해제 함수
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void EquipGun();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void UnequipGun();

    // Gun 발사 제어
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StartGunFiring();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StopGunFiring();

    // Gun 재장전
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void ReloadGun();

    // === IK 지원 함수 ===
    // 오른손 IK 타겟 위치 가져오기 (애니메이션 시스템에서 사용)
    UFUNCTION(BlueprintCallable, Category = "Animation")
    FTransform GetRightHandIKTransform() const;

    // 왼손 IK 타겟 위치 가져오기 (애니메이션 시스템에서 사용)
    UFUNCTION(BlueprintCallable, Category = "Animation")
    FTransform GetLeftHandIKTransform() const;

    // Gun이 장착되어 있는지 확인
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    bool HasGunEquipped() const { return equippedGun != nullptr; }

    // === 이동 상태 업데이트 함수 ===
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void UpdateMovementState(bool isRunning, const FVector& direction);

    // 🔧 추가된 디버그 함수들
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ForceInitializeBlackboard();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // === TPS Kit GASP 시스템 - 업데이트 함수들 ===
    void UpdateTargetTracking(float deltaTime);
    void UpdateCombatBehavior(float deltaTime);
    void UpdateMovementParameters(float deltaTime);

    // === 🔧 전투 상태 변수 (중복 제거, 시간 관련만 protected) ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    float timeSinceLastShot = 0.0f;    // 마지막 발사 이후 경과 시간

    // === 애니메이션 보간 변수 ===
    FVector2D previousMovementVector;           // 이전 이동 벡터 (보간용)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float movementVectorInterpSpeed = 5.0f;     // 이동 벡터 보간 속도

    // === 타이머 핸들들 ===
    FTimerHandle burstFireTimer;
    FTimerHandle burstFireCooldownTimer;
    FTimerHandle TimerHandle_AutoFire;

private:
    // === 기본 시스템 변수들 ===
    AActor* player = nullptr;
    APlayerController* playerController = nullptr;
    UIDToItem* idToItem = nullptr;

    // === 발사 시스템 ===
    void Fire();
    void StartFire();
    void StopFire();

    // === 생명 관리 ===
    void Death();

    // === 무기 시스템 헬퍼 함수 ===
    void AttachGunToSocket();   // Gun을 소켓에 부착
    void DetachGunFromSocket(); // Gun을 소켓에서 분리

    // === 애니메이션 헬퍼 함수 ===
    // 이동 벡터 업데이트 함수 - 애니메이션 파라미터 계산
    void UpdateMovementVector(const FVector& Direction, bool bIsRunning);

    // === TPS Kit GASP 시스템 - 헬퍼 함수들 ===
    void UpdateBlackboardValues();  // 블랙보드 값 업데이트
    bool CanEngageTarget(AActor* Target) const;  // 타겟 교전 가능 여부 확인
    void HandleTargetLoss();  // 타겟 손실 처리

    // 🔧 추가된 디버그 헬퍼 함수
    void DebugBlackboardValues();  // 블랙보드 값 디버그 출력
};
