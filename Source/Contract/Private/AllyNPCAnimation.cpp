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
    isInAir = false;
    isCrouching = false;

    isAiming = false;
    isFiring = false;
    isReloading = false;
    isEquipping = false;
    currentWeaponType = 2; // �⺻������ ���� ����

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
    // ĳ���� ���۷��� �ʱ�ȭ
    owningCharacter = Cast<ACharacter>(TryGetPawnOwner());
}

void UAllyNPCAnimation::NativeUpdateAnimation(float DeltaSeconds)
{
    // ĳ���� ���۷����� ��ȿ���� Ȯ��
    if (!owningCharacter)
    {
        owningCharacter = Cast<ACharacter>(TryGetPawnOwner());
        if (!owningCharacter)
            return;
    }

    // ĳ���� �̵� ������Ʈ ��������
    UCharacterMovementComponent* movementComponent = owningCharacter->GetCharacterMovement();
    if (!movementComponent)
        return;

    // �ӵ� ���
    FVector velocity = owningCharacter->GetVelocity();
    FVector lateralVelocity = FVector(velocity.X, velocity.Y, 0.0f);
    Speed = lateralVelocity.Size();

    // �̵� ���� ��� (�����¿�)
    if (Speed > 0.0f)
    {
        FRotator ActorRotation = owningCharacter->GetActorRotation();
        FVector forwardVector = ActorRotation.Vector();
        FVector rightVector = FRotator(0.0f, ActorRotation.Yaw, 0.0f).Vector().RightVector;

        // ����ȭ�� �ӵ� ����
        FVector normalizedVelocity = lateralVelocity.GetSafeNormal();

        // ���� ���� (-1: �Ĺ�, 1: ����)
        float forwardAmount = FVector::DotProduct(normalizedVelocity, forwardVector);
        // �¿� ���� (-1: ����, 1: ������)
        float rightAmount = FVector::DotProduct(normalizedVelocity, rightVector);

        // ���� ���� -180���� 180 ������ ������ ��ȯ
        Direction = FMath::RadiansToDegrees(FMath::Atan2(rightAmount, forwardAmount));
    }
    else
    {
        Direction = 0.0f;
    }

    // �� �ִ��� Ȯ��
    isInAir = movementComponent->IsFalling();

    // �ɾ� �ִ��� Ȯ��
    isCrouching = movementComponent->IsCrouching();

    // ���� ������ ������Ʈ (�ε巯�� ����)
    FRotator aimRotation = owningCharacter->GetBaseAimRotation();
    FRotator actorRotation = owningCharacter->GetActorRotation();
    FRotator deltaRotation = aimRotation - actorRotation;

    // ���� -180���� 180 ���̷� ����ȭ
    // NormalizeAxis ��� UKismetMathLibrary ���
    deltaRotation.Yaw = UKismetMathLibrary::NormalizeAxis(deltaRotation.Yaw);
    deltaRotation.Pitch = UKismetMathLibrary::NormalizeAxis(deltaRotation.Pitch);

    // �ε巴�� ����
    aimOffsetYaw = FMath::FInterpTo(aimOffsetYaw, deltaRotation.Yaw, DeltaSeconds, aimInterpSpeed);
    aimOffsetPitch = FMath::FInterpTo(aimOffsetPitch, deltaRotation.Pitch, DeltaSeconds, aimInterpSpeed);

    // ���� ������ ��� �ٸ� ��� �÷��� ��Ȱ��ȭ
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

    // ���� ���
    if (fireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            fireSound,
            owningCharacter->GetActorLocation()
        );
    }

    // �ѱ� ȭ�� ����Ʈ ���
    if (MuzzleFlash && owningCharacter)
    {
        // �ѱ� ���� ��ġ�� ����Ʈ ����
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

    // ź�� ���� ����Ʈ ���
    if (ejectedShell && owningCharacter)
    {
        // ź�� ���� ���� ��ġ�� ����Ʈ ����
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

    // ��Ÿ�� ���
    float playRate = 1.0f;
    float startingPosition = 0.0f;
    Montage_Play(fireMontage, playRate, EMontagePlayReturnType::MontageLength, startingPosition);

    // ��Ÿ�� ���� �� �ڵ����� �߻� ���� ����
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

    // ������ ��Ÿ�� ���
    float PlayRate = 1.0f;
    Montage_Play(reloadMontage, PlayRate);

    // ��Ÿ�� ���� �� �ڵ����� ������ ���� ����
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

    // ��� ���� ��Ÿ�� ���
    float PlayRate = 1.0f;
    Montage_Play(equipMontage, PlayRate);

    // ��Ÿ�� ���� �� �ڵ����� ���� ���� ����
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

    // ����ź ��ô ��Ÿ�� ���
    float PlayRate = 1.0f;
    Montage_Play(throwGrenadeMontage, PlayRate);

    // ��Ÿ�� ���� �� �ڵ����� ��ô ���� ����
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

    // �ǰ� ��Ÿ�� ���
    float playRate = 1.0f;
    FString sectionName = "HitDefault";

    // ���� ���⿡ ���� �ٸ� ���� ���
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

    // ��Ÿ�� ���� �� �ڵ����� �ǰ� ���� ����
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