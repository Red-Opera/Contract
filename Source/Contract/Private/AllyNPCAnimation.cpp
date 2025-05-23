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
    bIsInAir = false;
    bIsCrouching = false;

    bIsAiming = false;
    bIsFiring = false;
    bIsReloading = false;
    bIsEquipping = false;
    CurrentWeaponType = 2; // 기본값으로 소총 설정

    bIsInCover = false;
    bIsHit = false;
    bIsDead = false;
    bIsThrowingGrenade = false;

    bIsPeekingLeft = false;
    bIsPeekingRight = false;
    AimOffsetYaw = 0.0f;
    AimOffsetPitch = 0.0f;

    bEnableHandIK = true;
}

void UAllyNPCAnimation::NativeInitializeAnimation()
{
    // 캐릭터 레퍼런스 초기화
    OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
}

void UAllyNPCAnimation::NativeUpdateAnimation(float DeltaSeconds)
{
    // 캐릭터 레퍼런스가 유효한지 확인
    if (!OwningCharacter)
    {
        OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
        if (!OwningCharacter)
            return;
    }

    // 캐릭터 이동 컴포넌트 가져오기
    UCharacterMovementComponent* MovementComponent = OwningCharacter->GetCharacterMovement();
    if (!MovementComponent)
        return;

    // 속도 계산
    FVector Velocity = OwningCharacter->GetVelocity();
    FVector LateralVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
    Speed = LateralVelocity.Size();

    // 이동 방향 계산 (전후좌우)
    if (Speed > 0.0f)
    {
        FRotator ActorRotation = OwningCharacter->GetActorRotation();
        FVector ForwardVector = ActorRotation.Vector();
        FVector RightVector = FRotator(0.0f, ActorRotation.Yaw, 0.0f).Vector().RightVector;

        // 정규화된 속도 벡터
        FVector NormalizedVelocity = LateralVelocity.GetSafeNormal();

        // 전후 방향 (-1: 후방, 1: 전방)
        float ForwardAmount = FVector::DotProduct(NormalizedVelocity, ForwardVector);
        // 좌우 방향 (-1: 왼쪽, 1: 오른쪽)
        float RightAmount = FVector::DotProduct(NormalizedVelocity, RightVector);

        // 방향 값을 -180에서 180 사이의 각도로 변환
        Direction = FMath::RadiansToDegrees(FMath::Atan2(RightAmount, ForwardAmount));
    }
    else
    {
        Direction = 0.0f;
    }

    // 떠 있는지 확인
    bIsInAir = MovementComponent->IsFalling();

    // 앉아 있는지 확인
    bIsCrouching = MovementComponent->IsCrouching();

    // 조준 오프셋 업데이트 (부드러운 보간)
    FRotator AimRotation = OwningCharacter->GetBaseAimRotation();
    FRotator ActorRotation = OwningCharacter->GetActorRotation();
    FRotator DeltaRotation = AimRotation - ActorRotation;

    // 값을 -180에서 180 사이로 정규화
    // NormalizeAxis 대신 UKismetMathLibrary 사용
    DeltaRotation.Yaw = UKismetMathLibrary::NormalizeAxis(DeltaRotation.Yaw);
    DeltaRotation.Pitch = UKismetMathLibrary::NormalizeAxis(DeltaRotation.Pitch);

    // 부드럽게 보간
    AimOffsetYaw = FMath::FInterpTo(AimOffsetYaw, DeltaRotation.Yaw, DeltaSeconds, AimInterpSpeed);
    AimOffsetPitch = FMath::FInterpTo(AimOffsetPitch, DeltaRotation.Pitch, DeltaSeconds, AimInterpSpeed);

    // 죽은 상태일 경우 다른 모든 플래그 비활성화
    if (bIsDead)
    {
        bIsAiming = false;
        bIsFiring = false;
        bIsReloading = false;
        bIsEquipping = false;
        bIsInCover = false;
        bIsThrowingGrenade = false;
        bIsPeekingLeft = false;
        bIsPeekingRight = false;
    }
}

void UAllyNPCAnimation::PlayFireMontage()
{
    if (!FireMontage || bIsReloading || bIsEquipping || bIsDead || bIsThrowingGrenade)
        return;

    bIsFiring = true;

    // 사운드 재생
    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            FireSound,
            OwningCharacter->GetActorLocation()
        );
    }

    // 총구 화염 이펙트 재생
    if (MuzzleFlash && OwningCharacter)
    {
        // 총구 소켓 위치에 이펙트 생성
        USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh();
        if (Mesh)
        {
            FName MuzzleSocketName = FName("MuzzleFlashSocket");
            if (Mesh->DoesSocketExist(MuzzleSocketName))
            {
                FVector MuzzleLocation = Mesh->GetSocketLocation(MuzzleSocketName);
                FRotator MuzzleRotation = Mesh->GetSocketRotation(MuzzleSocketName);

                UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                    GetWorld(),
                    MuzzleFlash,
                    MuzzleLocation,
                    MuzzleRotation
                );
            }
        }
    }

    // 탄피 배출 이펙트 재생
    if (EjectedShell && OwningCharacter)
    {
        // 탄피 배출 소켓 위치에 이펙트 생성
        USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh();
        if (Mesh)
        {
            FName ShellEjectSocketName = FName("ShellEjectSocket");
            if (Mesh->DoesSocketExist(ShellEjectSocketName))
            {
                FVector ShellLocation = Mesh->GetSocketLocation(ShellEjectSocketName);
                FRotator ShellRotation = Mesh->GetSocketRotation(ShellEjectSocketName);

                UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                    GetWorld(),
                    EjectedShell,
                    ShellLocation,
                    ShellRotation
                );
            }
        }
    }

    // 몽타주 재생
    float PlayRate = 1.0f;
    float StartingPosition = 0.0f;
    Montage_Play(FireMontage, PlayRate, EMontagePlayReturnType::MontageLength, StartingPosition);

    // 몽타주 종료 후 자동으로 발사 상태 해제
    FTimerHandle TimerHandle_ResetFiring;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetFiring, [this]()
        {
            bIsFiring = false;
        }, FireMontage->GetPlayLength() * (1.0f / PlayRate), false);
}

void UAllyNPCAnimation::PlayReloadMontage()
{
    if (!ReloadMontage || bIsReloading || bIsEquipping || bIsDead || bIsThrowingGrenade)
        return;

    bIsReloading = true;

    // 재장전 몽타주 재생
    float PlayRate = 1.0f;
    Montage_Play(ReloadMontage, PlayRate);

    // 몽타주 종료 후 자동으로 재장전 상태 해제
    FTimerHandle TimerHandle_ResetReloading;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetReloading, [this]()
        {
            bIsReloading = false;
        }, ReloadMontage->GetPlayLength() * (1.0f / PlayRate), false);
}

void UAllyNPCAnimation::PlayEquipMontage()
{
    if (!EquipMontage || bIsReloading || bIsEquipping || bIsDead || bIsThrowingGrenade)
        return;

    bIsEquipping = true;

    // 장비 장착 몽타주 재생
    float PlayRate = 1.0f;
    Montage_Play(EquipMontage, PlayRate);

    // 몽타주 종료 후 자동으로 장착 상태 해제
    FTimerHandle TimerHandle_ResetEquipping;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetEquipping, [this]()
        {
            bIsEquipping = false;
        }, EquipMontage->GetPlayLength() * (1.0f / PlayRate), false);
}

void UAllyNPCAnimation::PlayThrowGrenadeMontage()
{
    if (!ThrowGrenadeMontage || bIsReloading || bIsEquipping || bIsDead || bIsThrowingGrenade)
        return;

    bIsThrowingGrenade = true;

    // 수류탄 투척 몽타주 재생
    float PlayRate = 1.0f;
    Montage_Play(ThrowGrenadeMontage, PlayRate);

    // 몽타주 종료 후 자동으로 투척 상태 해제
    FTimerHandle TimerHandle_ResetThrowing;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetThrowing, [this]()
        {
            bIsThrowingGrenade = false;
        }, ThrowGrenadeMontage->GetPlayLength() * (1.0f / PlayRate), false);
}

void UAllyNPCAnimation::PlayHitReactMontage(FName HitDirection)
{
    if (!HitReactMontage || bIsDead)
        return;

    bIsHit = true;

    // 피격 몽타주 재생
    float PlayRate = 1.0f;
    FString SectionName = "HitDefault";

    // 맞은 방향에 따라 다른 섹션 재생
    if (HitDirection == FName("Front"))
        SectionName = "HitFront";
    else if (HitDirection == FName("Back"))
        SectionName = "HitBack";
    else if (HitDirection == FName("Left"))
        SectionName = "HitLeft";
    else if (HitDirection == FName("Right"))
        SectionName = "HitRight";

    Montage_Play(HitReactMontage, PlayRate);
    Montage_JumpToSection(FName(*SectionName), HitReactMontage);

    // 몽타주 종료 후 자동으로 피격 상태 해제
    FTimerHandle TimerHandle_ResetHit;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetHit, [this]()
        {
            bIsHit = false;
        }, HitReactMontage->GetPlayLength() * (1.0f / PlayRate), false);
}

void UAllyNPCAnimation::PlayDeathMontage()
{
    if (!DeathMontage || bIsDead)
        return;

    bIsDead = true;

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
        int32 SectionIndex = FMath::RandRange(0, sectionNames.Num() - 1);
        FName SectionName = sectionNames[SectionIndex];

        Montage_Play(DeathMontage, PlayRate);
        Montage_JumpToSection(SectionName, DeathMontage);
    }
    else
    {
        Montage_Play(DeathMontage, PlayRate);
    }
}