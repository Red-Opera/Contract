// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Gun.h"
#include "Enemy.generated.h"

class UPawnSensingComponent;

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

    void SetDamage(FVector hitLocation, int damage);

    // 데미지 표기 나이아가라
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class AFloatingDamage> damageParticle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    float textOffset = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TArray<UTexture*> digitImage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    bool isDead = false;

    // 발사 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    bool isFire;

    // 발사 딜레이
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    float fireRate = 0.1f;

    // 적이 갖고 있는 아이템
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
    TArray<int> itemCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
    int money = 0;

    // === 애니메이션 시스템을 위한 추가 부분 ===
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

    // === 이동 시스템 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float walkSpeed = 250.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float runSpeed = 500.0f;

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

    // === 이동 상태 업데이트 함수 (AllyNPC와 동일) ===
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void UpdateMovementState(bool isRunning, const FVector& direction);

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

private:
    // 발사 함수
    void Fire();
    void StartFire();
    void StopFire();

    void Death();

    class AActor* player;
    class APlayerController* playerController;
    class UIDToItem* idToItem;

    float hp = 1000.0f;

    FTimerHandle TimerHandle_AutoFire;

    // === 무기 시스템 헬퍼 함수 ===
    void AttachGunToSocket();   // Gun을 소켓에 부착
    void DetachGunFromSocket(); // Gun을 소켓에서 분리

    // 전투 상태 변수
    bool isFiring;              // 현재 발사 중인지 여부
    float timeSinceLastShot;    // 마지막 발사 이후 경과 시간

    // 애니메이션 보간 변수
    FVector2D previousMovementVector;           // 이전 이동 벡터 (보간용)
    float movementVectorInterpSpeed = 5.0f;     // 이동 벡터 보간 속도

    // 이동 벡터 업데이트 함수 - 애니메이션 파라미터 계산
    void UpdateMovementVector(const FVector& Direction, bool bIsRunning);
};
