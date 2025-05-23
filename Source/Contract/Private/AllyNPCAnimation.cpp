#include "AllyNPCAnimation.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h" // ���� �Լ� ����� ���� �߰�
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "Animation/AnimMontage.h"

UAllyNPCAnimation::UAllyNPCAnimation()
{
    // �ʱⰪ ����
    Speed = 0.0f;
    Direction = 0.0f;
    bIsInAir = false;
    bIsCrouching = false;

    bIsAiming = false;
    bIsFiring = false;
    bIsReloading = false;
    bIsEquipping = false;
    CurrentWeaponType = 2; // �⺻������ ���� ����

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
    // ĳ���� ���۷��� �ʱ�ȭ
    OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
}

void UAllyNPCAnimation::NativeUpdateAnimation(float DeltaSeconds)
{
    // ĳ���� ���۷����� ��ȿ���� Ȯ��
    if (!OwningCharacter)
    {
        OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
        if (!OwningCharacter)
            return;
    }

    // ĳ���� �̵� ������Ʈ ��������
    UCharacterMovementComponent* MovementComponent = OwningCharacter->GetCharacterMovement();
    if (!MovementComponent)
        return;

    // �ӵ� ���
    FVector Velocity = OwningCharacter->GetVelocity();
    FVector LateralVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
    Speed = LateralVelocity.Size();

    // �̵� ���� ��� (�����¿�)
    if (Speed > 0.0f)
    {
        FRotator ActorRotation = OwningCharacter->GetActorRotation();
        FVector ForwardVector = ActorRotation.Vector();
        FVector RightVector = FRotator(0.0f, ActorRotation.Yaw, 0.0f).Vector().RightVector;

        // ����ȭ�� �ӵ� ����
        FVector NormalizedVelocity = LateralVelocity.GetSafeNormal();

        // ���� ���� (-1: �Ĺ�, 1: ����)
        float ForwardAmount = FVector::DotProduct(NormalizedVelocity, ForwardVector);
        // �¿� ���� (-1: ����, 1: ������)
        float RightAmount = FVector::DotProduct(NormalizedVelocity, RightVector);

        // ���� ���� -180���� 180 ������ ������ ��ȯ
        Direction = FMath::RadiansToDegrees(FMath::Atan2(RightAmount, ForwardAmount));
    }
    else
    {
        Direction = 0.0f;
    }

    // �� �ִ��� Ȯ��
    bIsInAir = MovementComponent->IsFalling();

    // �ɾ� �ִ��� Ȯ��
    bIsCrouching = MovementComponent->IsCrouching();

    // ���� ������ ������Ʈ (�ε巯�� ����)
    FRotator AimRotation = OwningCharacter->GetBaseAimRotation();
    FRotator ActorRotation = OwningCharacter->GetActorRotation();
    FRotator DeltaRotation = AimRotation - ActorRotation;

    // ���� -180���� 180 ���̷� ����ȭ
    // NormalizeAxis ��� UKismetMathLibrary ���
    DeltaRotation.Yaw = UKismetMathLibrary::NormalizeAxis(DeltaRotation.Yaw);
    DeltaRotation.Pitch = UKismetMathLibrary::NormalizeAxis(DeltaRotation.Pitch);

    // �ε巴�� ����
    AimOffsetYaw = FMath::FInterpTo(AimOffsetYaw, DeltaRotation.Yaw, DeltaSeconds, AimInterpSpeed);
    AimOffsetPitch = FMath::FInterpTo(AimOffsetPitch, DeltaRotation.Pitch, DeltaSeconds, AimInterpSpeed);

    // ���� ������ ��� �ٸ� ��� �÷��� ��Ȱ��ȭ
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

    // ���� ���
    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            FireSound,
            OwningCharacter->GetActorLocation()
        );
    }

    // �ѱ� ȭ�� ����Ʈ ���
    if (MuzzleFlash && OwningCharacter)
    {
        // �ѱ� ���� ��ġ�� ����Ʈ ����
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

    // ź�� ���� ����Ʈ ���
    if (EjectedShell && OwningCharacter)
    {
        // ź�� ���� ���� ��ġ�� ����Ʈ ����
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

    // ��Ÿ�� ���
    float PlayRate = 1.0f;
    float StartingPosition = 0.0f;
    Montage_Play(FireMontage, PlayRate, EMontagePlayReturnType::MontageLength, StartingPosition);

    // ��Ÿ�� ���� �� �ڵ����� �߻� ���� ����
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

    // ������ ��Ÿ�� ���
    float PlayRate = 1.0f;
    Montage_Play(ReloadMontage, PlayRate);

    // ��Ÿ�� ���� �� �ڵ����� ������ ���� ����
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

    // ��� ���� ��Ÿ�� ���
    float PlayRate = 1.0f;
    Montage_Play(EquipMontage, PlayRate);

    // ��Ÿ�� ���� �� �ڵ����� ���� ���� ����
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

    // ����ź ��ô ��Ÿ�� ���
    float PlayRate = 1.0f;
    Montage_Play(ThrowGrenadeMontage, PlayRate);

    // ��Ÿ�� ���� �� �ڵ����� ��ô ���� ����
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

    // �ǰ� ��Ÿ�� ���
    float PlayRate = 1.0f;
    FString SectionName = "HitDefault";

    // ���� ���⿡ ���� �ٸ� ���� ���
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

    // ��Ÿ�� ���� �� �ڵ����� �ǰ� ���� ����
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

    // ��� ��Ÿ�� ���
    float PlayRate = 1.0f;

    // ���� ��� �ִϸ��̼� ����
    // GetSectionNames �Լ��� �����Ƿ� ��Ÿ�ֿ��� ���� �̸� �������� ó��
    TArray<FName> sectionNames;

    // DeathMontage���� ��� ������ ���� �̸��� ���� �߰�
    // �����δ� ��Ÿ�ֿ� ���ǵ� ���� �̸��� �°� ���� �ʿ�
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