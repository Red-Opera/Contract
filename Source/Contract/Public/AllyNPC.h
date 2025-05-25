#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AllyNPC.generated.h"

class USkeletalMeshComponent;
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

    // 무기 시스템 - 시각적 표현 및 발사 기능
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    USkeletalMeshComponent* weaponMesh;

    // 무기 장착 소켓 - 캐릭터 메시에서 무기가 부착될 위치
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    FName weaponSocketName;

    // 이동 속도 설정 - 걷기 속도
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float walkSpeed = 300.0f;

    // 이동 속도 설정 - 달리기 속도
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float runSpeed = 600.0f;

    // 애니메이션 파라미터 - 이동 벡터 (X: 전/후, Y: 좌/우)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    FVector2D movementVector;

    // 이동 상태 업데이트 함수 - AI 컨트롤러에서 호출
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void UpdateMovementState(bool isRunning, const FVector& direction);

    // 전투 기능 - 발사 시작
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartFiring();

    // 전투 기능 - 발사 중지
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StopFiring();

    // 무기 장착 - 게임 시작 시 호출
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void EquipWeapon();

private:
    // 전투 상태 변수
    bool isFiring;              // 현재 발사 중인지 여부
    float timeSinceLastShot;    // 마지막 발사 이후 경과 시간
    float fireRate = 0.1f;      // 발사 간격 (초)

    // 애니메이션 보간 변수
    FVector2D previousMovementVector;    // 이전 이동 벡터 (보간용)
    float movementVectorInterpSpeed = 5.0f;  // 이동 벡터 보간 속도

    // 이동 벡터 업데이트 함수 - 애니메이션 파라미터 계산
    void UpdateMovementVector(const FVector& Direction, bool bIsRunning);
    
    // 무기 발사 구현 - 라인 트레이스 기반 히트 스캔
    void FireWeapon();
};