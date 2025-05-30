#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Gun.h"
#include "AllyNPC.generated.h"

class UPawnSensingComponent;

/**
 * AAllyNPC - 플레이어를 지원하는 동맹 NPC 캐릭터 클래스
 * AI 컨트롤러에 의해 제어되며 플레이어를 따라다니며 전투 지원을 제공
 */
UCLASS()
class CONTRACT_API AAllyNPC : public ACharacter
{
    GENERATED_BODY()

public:
    // 생성자 - 기본 컴포넌트 및 속성 초기화
    AAllyNPC();

protected:
    // 게임 시작 시 호출되는 초기화 함수
    virtual void BeginPlay() override;

public:
    // 매 프레임 호출되는 업데이트 함수
    virtual void Tick(float deltaTime) override;
    
    // 입력 바인딩 설정 (NPC는 직접 입력을 받지 않음)
    virtual void SetupPlayerInputComponent(class UInputComponent* playerInputComponent) override;

    // AI 센싱 컴포넌트 - 주변 환경 감지
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UPawnSensingComponent* pawnSensingComp;

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
    float walkSpeed = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float runSpeed = 600.0f;

    // 애니메이션 파라미터 - 이동 벡터 (X: 전/후, Y: 좌/우)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    FVector2D movementVector;

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

    // === 이동 함수 ===
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void UpdateMovementState(bool isRunning, const FVector& direction);

    // === 기존 전투 함수 (호환성 유지) ===
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartFiring();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StopFiring();

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

private:
    // 전투 상태 변수
    bool isFiring;              // 현재 발사 중인지 여부
    float timeSinceLastShot;    // 마지막 발사 이후 경과 시간
    float fireRate = 0.1f;      // 발사 간격 (초)

    // 애니메이션 보간 변수
    FVector2D previousMovementVector;           // 이전 이동 벡터 (보간용)
    float movementVectorInterpSpeed = 5.0f;     // 이동 벡터 보간 속도

    // 이동 벡터 업데이트 함수 - 애니메이션 파라미터 계산
    void UpdateMovementVector(const FVector& Direction, bool bIsRunning);
    
    // Gun 시스템 헬퍼 함수
    void AttachGunToSocket();   // Gun을 소켓에 부착
    void DetachGunFromSocket(); // Gun을 소켓에서 분리
};