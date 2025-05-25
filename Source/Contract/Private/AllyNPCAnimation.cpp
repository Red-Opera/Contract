#include "AllyNPCAnimation.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h" // 수학 함수 사용을 위해 추가
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "Animation/AnimMontage.h"

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
}

void UAllyNPCAnimation::NativeUpdateAnimation(float DeltaSeconds)
{
    // 캐릭터 레퍼런스가 유효한지 확인
    if (!owningCharacter)
    {
        owningCharacter = Cast<ACharacter>(TryGetPawnOwner());
        if (!owningCharacter)
            return;
    }

    // 캐릭터 이동 컴포넌트 가져오기
    UCharacterMovementComponent* movementComponent = owningCharacter->GetCharacterMovement();
    if (!movementComponent)
        return;

    // 속도 계산
    FVector velocity = owningCharacter->GetVelocity();
    FVector lateralVelocity = FVector(velocity.X, velocity.Y, 0.0f);
    Speed = lateralVelocity.Size();

    // 이동 방향 계산 (전후좌우)
    if (Speed > 0.0f)
    {
        FRotator ActorRotation = owningCharacter->GetActorRotation();
        FVector forwardVector = ActorRotation.Vector();
        FVector rightVector = FRotator(0.0f, ActorRotation.Yaw, 0.0f).Vector().RightVector;

        // 정규화된 속도 벡터
        FVector normalizedVelocity = lateralVelocity.GetSafeNormal();

        // 전후 방향 (-1: 후방, 1: 전방)
        float forwardAmount = FVector::DotProduct(normalizedVelocity, forwardVector);
        // 좌우 방향 (-1: 왼쪽, 1: 오른쪽)
        float rightAmount = FVector::DotProduct(normalizedVelocity, rightVector);

        // 방향 값을 -180에서 180 사이의 각도로 변환
        Direction = FMath::RadiansToDegrees(FMath::Atan2(rightAmount, forwardAmount));
    }
    else
    {
        Direction = 0.0f;
    }

    // 떠 있는지 확인
    isInAir = movementComponent->IsFalling();

    // 앉아 있는지 확인
    isCrouching = movementComponent->IsCrouching();

    // 조준 오프셋 업데이트 (부드러운 보간)
    FRotator aimRotation = owningCharacter->GetBaseAimRotation();
    FRotator actorRotation = owningCharacter->GetActorRotation();
    FRotator deltaRotation = aimRotation - actorRotation;

    // 값을 -180에서 180 사이로 정규화
    // NormalizeAxis 대신 UKismetMathLibrary 사용
    deltaRotation.Yaw = UKismetMathLibrary::NormalizeAxis(deltaRotation.Yaw);
    deltaRotation.Pitch = UKismetMathLibrary::NormalizeAxis(deltaRotation.Pitch);

    // 부드럽게 보간
    aimOffsetYaw = FMath::FInterpTo(aimOffsetYaw, deltaRotation.Yaw, DeltaSeconds, aimInterpSpeed);
    aimOffsetPitch = FMath::FInterpTo(aimOffsetPitch, deltaRotation.Pitch, DeltaSeconds, aimInterpSpeed);

    // 죽은 상태일 경우 다른 모든 플래그 비활성화
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
}

void UAllyNPCAnimation::PlayFireMontage()
{
    if (!fireMontage || isReloading || isEquipping || isDead || isThrowingGrenade)
        return;

    isFiring = true;

    // 사운드 재생
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

    // 장비 장착 몽타주 재생
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

    // 맞은 방향에 따라 다른 섹션 재생
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
    float PlayRate = 1.0f;

    // 랜덤 사망 애니메이션 선택
    // GetSectionNames 함수가 없으므로 몽타주에서 섹션 이름 수동으로 처리
    TArray<FName> sectionNames;

    // DeathMontage에서 사용 가능한 섹션 이름을 직접 추가
    // 실제로는 몽타주에 정의된 섹션 이름에 맞게 수정 필요
    sectionNames.Add(FName("Death_Front"));
    sectionNames.Add(FName("Death_Back"));
    sectionNames.Add(FName("Death_Left"));
    sectionNames.Add(FName("Death_Right"));

    if (sectionNames.Num() > 0)
    {
        int32 sectionIndex = FMath::RandRange(0, sectionNames.Num() - 1);
        FName sectionName = sectionNames[sectionIndex];

        Montage_Play(deathMontage, PlayRate);
        Montage_JumpToSection(sectionName, deathMontage);
    }
    else
    {
        Montage_Play(deathMontage, PlayRate);
    }
}