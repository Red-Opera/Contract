#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "MolotovCocktail.generated.h"

/**
 * 화염병 아이템 클래스
 * 플레이어가 던질 수 있는 화염병으로, 충돌 시 화염 액터를 생성하여 화재를 발생시킵니다.
 * 화재 생성 시 시작/루프/종료 사운드를 재생하며, 시간이 지나면 스케일 애니메이션과 함께 소멸합니다.
 */
UCLASS()
class CONTRACT_API AMolotovCocktail : public AItem
{
    GENERATED_BODY()

public:
    AMolotovCocktail();

    /** 화염병 폭발 시 생성될 화염 액터의 클래스 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Equipment")
    TSubclassOf<AActor> fireMesh;
    
    /** 화염병이 깨질 때 재생될 사운드 큐 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
    USoundCue* molotovBreakSoundCue;
    
    /** 화염 시작 시 재생될 사운드 큐 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
    USoundCue* fireStartSoundCue;
    
    /** 화염이 지속되는 동안 반복 재생될 사운드 큐 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
    USoundCue* fireLoopSoundCue;
    
    /** 화염이 소멸할 때 재생될 사운드 큐 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
    USoundCue* fireEndSoundCue;
    
    /** 충돌 감지를 위한 캡슐 컴포넌트 */
    UPROPERTY(EditAnywhere)
    class UCapsuleComponent* collisionComponent;

protected:
    /** 게임 시작 시 초기화 작업 수행 */
    virtual void BeginPlay() override;
    
    /** 아이템 사용 시 호출되는 메서드 */
    virtual void UseItem() override;

private:
    /** 플레이어 인벤토리에 화염병을 추가하는 메서드 */
    void AddMolotovCocktail();
    
    /** 화염병과 화염 액터를 제거하는 메서드 */
    void RemoveActor();
    
    /** 지정된 위치에 화염 액터를 생성하는 메서드 */
    void SpawnFireActor(const FVector& spawnLocation);

    /** 화염 액터의 스케일 축소 애니메이션을 시작하는 메서드 */
    void StartScaleAnimation();
    
    /** 스케일 애니메이션을 프레임마다 업데이트하는 메서드 */
    void UpdateScaleAnimation();
    
    /** 화염 시작 사운드를 재생하는 메서드 */
    void PlayFireStartSound(const FVector& location);
    
    /** 화염 루프 사운드를 재생하는 메서드 */
    void PlayFireLoopSound(const FVector& location);
    
    /** 화염 종료 사운드를 재생하는 메서드 */
    void PlayFireEndSound(const FVector& location);
    
    /** 화염 루프 사운드를 중지하는 메서드 */
    void StopFireLoopSound();

    /** 컴포넌트 충돌 시 호출되는 델리게이트 함수 */
    UFUNCTION()
    void OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    /** 화염 시작 사운드 재생 완료 시 호출되는 콜백 함수 */
    UFUNCTION()
    void OnFireStartSoundFinished();

    /** 생성된 화염 액터에 대한 참조 */
    class AActor* fireActor;
    
    /** 화염 루프 사운드를 재생하는 오디오 컴포넌트 */
    UPROPERTY()
    UAudioComponent* fireLoopAudioComponent;

    /** 화염 시작 사운드를 재생하는 오디오 컴포넌트 */
    UPROPERTY()
    UAudioComponent* fireStartAudioComponent;
    
    /** 루프 사운드 재생 위치를 임시 저장하는 변수 */
    FVector pendingLoopSoundLocation;

    /** 화염병 제거용 타이머 핸들 */
    FTimerHandle timerHandle;
    
    /** 스케일 애니메이션용 타이머 핸들 */
    FTimerHandle scaleAnimationTimer;
    
    /** 스케일 애니메이션 시작 지연용 타이머 핸들 */
    FTimerHandle startScaleAnimationTimer;

    /** 화염 액터의 원본 스케일 값 */
    FVector originalScale;
    
    /** 스케일 애니메이션 지속 시간 (초) */
    float scaleAnimationDuration;
    
    /** 화염 루프 사운드 재생 시간 (초) */
    float fireLoopTime;
    
    /** 스케일 애니메이션 진행도 (0.0 ~ 1.0) */
    float scaleAnimationProgress;

    /** 화염병이 사용된 상태인지 나타내는 플래그 */
    bool isUse;
};
