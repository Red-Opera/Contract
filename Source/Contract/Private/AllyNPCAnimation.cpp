#include "AllyNPCAnimation.h"
#include "AllyNPC.h"

#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundBase.h"

UAllyNPCAnimation::UAllyNPCAnimation()
{
    // 초기값 설정
    Speed = 0.0f;
    Direction = 0.0f;
    isInAir = false;
    isCrouching = false;

    isAiming = false;
    isFiring = false;
    isReloading = false;
    isEquipping = false;
    currentWeaponType = 2; // 기본값으로 소총 설정

    isHit = false;
    isDead = false;
    isThrowingGrenade = false;

    isPeekingLeft = false;
    isPeekingRight = false;
    aimOffsetYaw = 0.0f;
    aimOffsetPitch = 0.0f;

    enableHandIK = true;
}

void UAllyNPCAnimation::NativeInitializeAnimation()
{
    // 캐릭터 레퍼런스 초기화
    owningCharacter = Cast<ACharacter>(TryGetPawnOwner());
    
    // 소유 캐릭터 확인 로직 추가
    if (!owningCharacter)
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("애니메이션의 소유 캐릭터가 존재하지 않습니다!"));

    else
    {
        GEngine->AddOnScreenDebugMessage
        (
            -1, 5.0f, FColor::Green, 
            FString::Printf(TEXT("애니메이션 소유 캐릭터: %s"), *owningCharacter->GetName())
        );
            
        // 캐릭터가 초기화된 후 딜레이를 두고 한번 더 확인
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            if (!owningCharacter)
            {
                UE_LOG(LogTemp, Error, TEXT("UAllyNPCAnimation: 타이머 후에도 캐릭터가 null입니다."));
            }
        }, 0.5f, false);
    }
}

void UAllyNPCAnimation::NativeUpdateAnimation(float DeltaSeconds)
{
    if (owningCharacter == nullptr)
        return;

    // 캐릭터 이동 컴포넌트 가져오기
    UCharacterMovementComponent* movementComponent = owningCharacter->GetCharacterMovement();

    if (movementComponent == nullptr)
        return;

    // 이동 벡터 가져오기 (AAllyNPC에서 추가한 기능)
    AAllyNPC* allyNPC = Cast<AAllyNPC>(owningCharacter);

    // NPC의 이동 벡터 가져오기
    if (allyNPC != nullptr)
        MovementVector = allyNPC->movementVector;

    else
    {
        // 일반 캐릭터인 경우, 기존 Speed와 Direction 사용
        // 속도 계산
        FVector velocity = owningCharacter->GetVelocity();
        FVector lateralVelocity = FVector(velocity.X, velocity.Y, 0.0f);
        Speed = lateralVelocity.Size();

        // 이동 방향 계산 (전후좌우)
        if (Speed > 0.0f)
        {
            FRotator actorRotation = owningCharacter->GetActorRotation();
            FVector forwardVector = actorRotation.Vector();
            FVector rightVector = FRotator(0.0f, actorRotation.Yaw, 0.0f).Vector().RightVector;

            // 정규화된 속도 벡터
            FVector normalizedVelocity = lateralVelocity.GetSafeNormal();

            
            float forwardAmount = FVector::DotProduct(normalizedVelocity, forwardVector);   // 전방 비율(-1: 후진, 1 : 전진)
            float rightAmount = FVector::DotProduct(normalizedVelocity, rightVector);       // 좌우 비율 (-1: 왼쪽, 1: 오른쪽)

            // 방향 각도 -180부터 180 사이로 변환
            Direction = FMath::RadiansToDegrees(FMath::Atan2(rightAmount, forwardAmount));
        }

        else
            Direction = 0.0f;
    }

    // 공중에 있는지 확인
    isInAir = movementComponent->IsFalling();

    // 쪼그려 있는지 확인
    isCrouching = movementComponent->IsCrouching();

    // 조준 오프셋 업데이트 (상체 회전용)
    FRotator aimRotation = owningCharacter->GetBaseAimRotation();
    FRotator actorRotation = owningCharacter->GetActorRotation();
    FRotator deltaRotation = aimRotation - actorRotation;

    // 각도 -180부터 180 사이로 정규화
    // NormalizeAxis 사용 (UKismetMathLibrary 함수)
    deltaRotation.Yaw = UKismetMathLibrary::NormalizeAxis(deltaRotation.Yaw);
    deltaRotation.Pitch = UKismetMathLibrary::NormalizeAxis(deltaRotation.Pitch);

    // 부드러운 조준 보간
    aimOffsetYaw = FMath::FInterpTo(aimOffsetYaw, deltaRotation.Yaw, DeltaSeconds, aimInterpSpeed);
    aimOffsetPitch = FMath::FInterpTo(aimOffsetPitch, deltaRotation.Pitch, DeltaSeconds, aimInterpSpeed);

    // 사망 상태일 경우 다른 모든 플래그 비활성화
    if (isDead)
    {
        isAiming = false;
        isFiring = false;
        isReloading = false;
        isEquipping = false;
        isThrowingGrenade = false;
        isPeekingLeft = false;
        isPeekingRight = false;
    }

    // 왼손 IK 위치를 게임 스레드에서 미리 계산하여 캐시
    UpdateLeftHandIKCache();
}

void UAllyNPCAnimation::PlayFireMontage()
{
    if (!fireMontage || isReloading || isEquipping || isDead || isThrowingGrenade)
        return;

    isFiring = true;

    // 발사 소리
    if (fireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            fireSound,
            owningCharacter->GetActorLocation()
        );
    }

    // 총구 화염 이펙트 재생
    if (MuzzleFlash && owningCharacter)
    {
        // 총구 소켓 위치에 이펙트 생성
        USkeletalMeshComponent* mesh = owningCharacter->GetMesh();
        if (mesh)
        {
            FName muzzleSocketName = FName("MuzzleFlashSocket");
            if (mesh->DoesSocketExist(muzzleSocketName))
            {
                FVector muzzleLocation = mesh->GetSocketLocation(muzzleSocketName);
                FRotator muzzleRotation = mesh->GetSocketRotation(muzzleSocketName);

                UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                    GetWorld(),
                    MuzzleFlash,
                    muzzleLocation,
                    muzzleRotation
                );
            }
        }
    }

    // 탄피 배출 이펙트 재생
    if (ejectedShell && owningCharacter)
    {
        // 탄피 배출 소켓 위치에 이펙트 생성
        USkeletalMeshComponent* mesh = owningCharacter->GetMesh();

        if (mesh)
        {
            FName shellEjectSocketName = FName("ShellEjectSocket");

            if (mesh->DoesSocketExist(shellEjectSocketName))
            {
                FVector shellLocation = mesh->GetSocketLocation(shellEjectSocketName);
                FRotator shellRotation = mesh->GetSocketRotation(shellEjectSocketName);

                UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                    GetWorld(),
                    ejectedShell,
                    shellLocation,
                    shellRotation
                );
            }
        }
    }

    // 몽타주 재생
    float playRate = 1.0f;
    float startingPosition = 0.0f;
    Montage_Play(fireMontage, playRate, EMontagePlayReturnType::MontageLength, startingPosition);

    // 몽타주 종료 후 자동으로 발사 상태 해제
    FTimerHandle timerHandle_ResetFiring;
    GetWorld()->GetTimerManager().SetTimer(timerHandle_ResetFiring, [this]()
        {
            isFiring = false;
        }, fireMontage->GetPlayLength() * (1.0f / playRate), false);
}

void UAllyNPCAnimation::PlayReloadMontage()
{
    if (!reloadMontage || isReloading || isEquipping || isDead || isThrowingGrenade)
        return;

    isReloading = true;

    // 재장전 몽타주 재생
    float PlayRate = 1.0f;
    Montage_Play(reloadMontage, PlayRate);

    // 몽타주 종료 후 자동으로 재장전 상태 해제
    FTimerHandle TimerHandle_ResetReloading;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetReloading, [this]()
        {
            isReloading = false;
        }, reloadMontage->GetPlayLength() * (1.0f / PlayRate), false);
}

void UAllyNPCAnimation::PlayEquipMontage()
{
    if (!equipMontage || isReloading || isEquipping || isDead || isThrowingGrenade)
        return;

    isEquipping = true;

    // 무기 장착 몽타주 재생
    float PlayRate = 1.0f;
    Montage_Play(equipMontage, PlayRate);

    // 몽타주 종료 후 자동으로 장착 상태 해제
    FTimerHandle TimerHandle_ResetEquipping;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetEquipping, [this]()
    {
        isEquipping = false;
    }, equipMontage->GetPlayLength() * (1.0f / PlayRate), false);
}

void UAllyNPCAnimation::PlayThrowGrenadeMontage()
{
    if (!throwGrenadeMontage || isReloading || isEquipping || isDead || isThrowingGrenade)
        return;

    isThrowingGrenade = true;

    // 수류탄 투척 몽타주 재생
    float PlayRate = 1.0f;
    Montage_Play(throwGrenadeMontage, PlayRate);

    // 몽타주 종료 후 자동으로 투척 상태 해제
    FTimerHandle TimerHandle_ResetThrowing;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetThrowing, [this]()
    {
        isThrowingGrenade = false;
    }, throwGrenadeMontage->GetPlayLength() * (1.0f / PlayRate), false);
}

void UAllyNPCAnimation::PlayHitReactMontage(FName HitDirection)
{
    if (!hitReactMontage || isDead)
        return;

    isHit = true;

    // 피격 몽타주 재생
    float playRate = 1.0f;
    FString sectionName = "HitDefault";

    // 피격 방향에 따른 다른 섹션 재생
    if (HitDirection == FName("Front"))
        sectionName = "HitFront";

    else if (HitDirection == FName("Back"))
        sectionName = "HitBack";

    else if (HitDirection == FName("Left"))
        sectionName = "HitLeft";

    else if (HitDirection == FName("Right"))
        sectionName = "HitRight";

    Montage_Play(hitReactMontage, playRate);
    Montage_JumpToSection(FName(*sectionName), hitReactMontage);

    // 몽타주 종료 후 자동으로 피격 상태 해제
    FTimerHandle timerHandle_ResetHit;
    GetWorld()->GetTimerManager().SetTimer(timerHandle_ResetHit, [this]()
    {
        isHit = false;
    }, hitReactMontage->GetPlayLength() * (1.0f / playRate), false);
}

void UAllyNPCAnimation::PlayDeathMontage()
{
    if (!deathMontage || isDead)
        return;

    isDead = true;

    // 사망 몽타주 재생
    float playRate = 1.0f;

    // 랜덤 사망 애니메이션 선택
    // GetSectionNames 함수가 없으므로 몽타주에서 섹션 이름 직접 정의
    TArray<FName> sectionNames;

    // DeathMontage에서 사용 가능한 섹션 이름 직접 추가
    // 실제로는 몽타주에 정의된 섹션 이름에 맞게 수정 필요
    sectionNames.Add(FName("Death_Front"));
    sectionNames.Add(FName("Death_Back"));
    sectionNames.Add(FName("Death_Left"));
    sectionNames.Add(FName("Death_Right"));

    if (sectionNames.Num() > 0)
    {
        int32 sectionIndex = FMath::RandRange(0, sectionNames.Num() - 1);
        FName sectionName = sectionNames[sectionIndex];

        Montage_Play(deathMontage, playRate);
        Montage_JumpToSection(sectionName, deathMontage);
    }

    else
        Montage_Play(deathMontage, playRate);
}

FTransform UAllyNPCAnimation::GetRightHandIKTransform() const
{
    // 소유 캐릭터가 유효한지 확인
    if (!owningCharacter)
    {
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("캐릭터가 존재하지 않음"));

        return FTransform::Identity;
    }

    // AllyNPC로 캐스트
    AAllyNPC* allyNPC = Cast<AAllyNPC>(owningCharacter);

    if (!allyNPC)
    {
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("애니메이션: AllyNPC 캐스트 실패"));

        return FTransform::Identity;
    }

    // AllyNPC에서 오른손 IK 트랜스폼 가져오기
    return allyNPC->GetRightHandIKTransform();
}

void UAllyNPCAnimation::UpdateLeftHandIKCache()
{
    // 초기화
    isLeftHandIKValid = false;
    cachedLeftHandIKTransform = FTransform::Identity;
    LeftHandIKLocation = FVector::ZeroVector;
    LeftHandIKRotation = FRotator::ZeroRotator;

    // 소유 캐릭터가 유효한지 확인
    if (!IsValid(owningCharacter))
    {
        return;
    }

    AAllyNPC* allyNPC = Cast<AAllyNPC>(owningCharacter);
    if (!IsValid(allyNPC))
    {
        return;
    }
    
    // Gun이 장착되어 있고 유효한지 확인
    if (IsValid(allyNPC->equippedGun))
    {
        AGun* gun = allyNPC->equippedGun;
        
        // Gun의 메시가 유효한지 확인
        if (IsValid(gun->mesh))
        {
            // 소켓 존재 여부 확인
            if (gun->mesh->DoesSocketExist(gun->leftHandGripSocketName))
            {
                // 안전하게 소켓 트랜스폼 가져오기
                cachedLeftHandIKTransform = gun->mesh->GetSocketTransform(gun->leftHandGripSocketName, RTS_World);
                
                // 캐시된 값들 업데이트
                LeftHandIKLocation = cachedLeftHandIKTransform.GetLocation();
                LeftHandIKRotation = cachedLeftHandIKTransform.GetRotation().Rotator();
                isLeftHandIKValid = true;
            }
        }
    }
}

// 스레드 안전한 함수로 완전히 수정
FTransform UAllyNPCAnimation::GetLeftHandIKTransform() const
{
    // 캐시된 값만 반환 (스레드 안전)
    return cachedLeftHandIKTransform;
}