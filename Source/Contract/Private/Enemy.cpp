// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "FloatingDamage.h"
#include "IDToItem.h"

// Sets default values
AEnemy::AEnemy()
{
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // === 무기 시스템 초기화 ===
    equippedGun = nullptr;

    // Gun 부착 오프셋 초기화 - 더 정확한 파지를 위한 조정
    gunAttachOffset = FTransform(
        FRotator(0.0f, 90.0f, 0.0f),    // Y축으로 90도 회전 (Gun이 앞을 향하도록)
        FVector(2.0f, 0.0f, 0.0f),      // 약간 앞으로 이동
        FVector(1.0f, 1.0f, 1.0f)       // 스케일
    );

    // 무기 장착 소켓들
    rightHandSocketName = TEXT("RightGunTarget");
    leftHandSocketName = TEXT("LeftGunTarget");

    // 전투 변수 초기화
    isFiring = false;
    timeSinceLastShot = 0.0f;

    // 애니메이션 변수 초기화
    previousMovementVector = FVector2D::ZeroVector;
    movementVector = FVector2D::ZeroVector;

    // === 이동 시스템 초기화 (수정된 버전) ===
    // 이동 설정 - 이동 방향으로만 회전하도록 설정
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

    // 자연스러운 회전을 위한 설정
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 200.0f, 0.0f);

    // 가속 및 감속 설정 - 부드러운 이동 시작/정지 (수정된 속성명)
    UCharacterMovementComponent* movementComp = GetCharacterMovement();
    if (movementComp)
    {
        movementComp->MaxAcceleration = 1000.0f;                    // 가속도
        movementComp->BrakingDecelerationWalking = 2000.0f;        // 걷기 감속도
        movementComp->BrakingFriction = 2.0f;                      // 마찰력
        movementComp->GroundFriction = 8.0f;                       // 지면 마찰력

        // 추가 이동 설정
        movementComp->BrakingDecelerationFalling = 600.0f;         // 낙하 감속도
        movementComp->AirControl = 0.3f;                           // 공중 제어력
        movementComp->AirControlBoostMultiplier = 2.0f;            // 공중 제어 배율
        movementComp->AirControlBoostVelocityThreshold = 25.0f;    // 공중 제어 속도 임계값

        // 추가적인 이동 개선 설정
        movementComp->bUseSeparateBrakingFriction = true;          // 별도 브레이킹 마찰 사용
        movementComp->BrakingFrictionFactor = 2.0f;                // 브레이킹 마찰 배율
    }

    // 공중에서도 자연스러운 이동을 위한 설정
    GetCharacterMovement()->BrakingDecelerationFalling = 600.0f;
    GetCharacterMovement()->AirControl = 0.3f;
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
    Super::BeginPlay();

    // 게임 시작 시 Gun 장착
    EquipGun();

    player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    idToItem = Cast<UIDToItem>(UGameplayStatics::GetGameInstance(GetWorld()));
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Gun 발사 로직 처리
    if (isFiring && equippedGun)
    {
        timeSinceLastShot += DeltaTime;
        if (timeSinceLastShot >= fireRate)
        {
            // Gun의 발사 함수 호출
            if (equippedGun->currentAmmoEquipped > 0)
            {
                equippedGun->Fire();
                timeSinceLastShot = 0.0f;
            }
        }
    }
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemy::SetDamage(FVector hitLocation, int damage)
{
    hp -= damage;

    if (hp <= 0)
    {
        isDead = true;
        Death();
    }
    else
    {
        // 데미지 표시 로직
        if (damageParticle)
        {
            FVector spawnLocation = hitLocation + FVector(0, 0, textOffset);
            GetWorld()->SpawnActor<AFloatingDamage>(damageParticle, spawnLocation, FRotator::ZeroRotator);
        }
    }
}

// === 이동 시스템 구현 (AllyNPC와 동일) ===

void AEnemy::UpdateMovementState(bool isRunning, const FVector& direction)
{
    // 이동 속도 설정
    float targetSpeed = isRunning ? runSpeed : walkSpeed;
    GetCharacterMovement()->MaxWalkSpeed = targetSpeed;

    // 이동 벡터를 애니메이션 시스템을 위해 업데이트
    UpdateMovementVector(direction, isRunning);

    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Cyan,
        FString::Printf(TEXT("Enemy MovementVector: X=%.2f, Y=%.2f"),
            movementVector.X, movementVector.Y));
}

void AEnemy::UpdateMovementVector(const FVector& direction, bool isRunning)
{
    // 월드 좌표계의 이동 방향을 캐릭터 로컬 좌표계로 변환
    FVector localDirection = GetActorTransform().InverseTransformVectorNoScale(direction);

    // 목표 이동 벡터 계산 (달리기 상태에 따라 스케일 조정)
    FVector2D targetMovementVector;
    targetMovementVector.X = localDirection.X * (isRunning ? 2.0f : 1.0f);  // 앞/뒤 이동
    targetMovementVector.Y = localDirection.Y * (isRunning ? 2.0f : 1.0f);  // 좌/우 이동

    // 부드러운 변화를 위한 이동 벡터 보간 적용 - 더 빠른 보간 속도
    if (GetWorld())
    {
        float deltaTime = GetWorld()->GetDeltaSeconds();
        // 보간 속도를 크게 증가 (5.0f -> 15.0f)
        movementVector = FMath::Vector2DInterpTo(movementVector, targetMovementVector, deltaTime, 15.0f);
    }
    else
    {
        movementVector = targetMovementVector;
    }

    // 이전 벡터 저장
    previousMovementVector = movementVector;
}

// === Gun 시스템 구현 ===

void AEnemy::EquipGun()
{
    // Gun 블루프린트가 설정되어 있는지 확인
    if (!gunBlueprint)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("적 Gun 블루프린트가 설정되지 않음! 블루프린트에서 설정 필요!"));
        return;
    }

    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("적 Gun 블루프린트: %s"), *gunBlueprint->GetName()));

    // 기존 Gun이 있다면 제거
    if (equippedGun)
    {
        UnequipGun();
    }

    // Gun 액터 생성
    FVector spawnLocation = GetActorLocation();
    FRotator spawnRotation = GetActorRotation();

    equippedGun = GetWorld()->SpawnActor<AGun>(gunBlueprint, spawnLocation, spawnRotation);

    if (equippedGun)
    {
        // Gun을 오른손 소켓에 부착
        AttachGunToSocket();

        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("적 Gun이 성공적으로 장착되었습니다."));
    }
    else
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("적 Gun 액터 생성에 실패했습니다."));
}

void AEnemy::UnequipGun()
{
    if (equippedGun == nullptr)
        return;

    // Gun을 소켓에서 분리
    DetachGunFromSocket();

    // Gun 액터 삭제
    equippedGun->Destroy();
    equippedGun = nullptr;

    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("적 Gun이 성공적으로 해제되었습니다."));
}

void AEnemy::AttachGunToSocket()
{
    if (!equippedGun)
        return;

    USkeletalMeshComponent* characterMesh = GetMesh();
    if (!characterMesh)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("적 캐릭터 메시를 찾을 수 없음!"));
        return;
    }

    // 오른손 소켓 존재 확인 (Gun을 오른손에 장착)
    if (!characterMesh->DoesSocketExist(rightHandSocketName))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("적 소켓 '%s'을 찾을 수 없음!"), *rightHandSocketName.ToString()));
        return;
    }

    // Gun을 오른손 소켓에 부착
    equippedGun->AttachToComponent(
        characterMesh,
        FAttachmentTransformRules::KeepRelativeTransform,
        rightHandSocketName
    );

    // Gun 위치와 회전을 더 정확하게 조정
    FTransform adjustedTransform = gunAttachOffset;
    equippedGun->SetActorRelativeTransform(adjustedTransform);

    // NPC 장착 상태 설정
    equippedGun->SetEquippedByNPC(true);

    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("적 Gun이 소켓 '%s'에 부착되었습니다."), *rightHandSocketName.ToString()));
}

void AEnemy::DetachGunFromSocket()
{
    if (equippedGun)
    {
        // NPC 장착 상태 해제
        equippedGun->SetEquippedByNPC(false);

        equippedGun->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    }
}

void AEnemy::StartGunFiring()
{
    if (equippedGun)
    {
        isFiring = true;
        timeSinceLastShot = fireRate; // 즉시 첫 발사 가능
        equippedGun->StartFire(); // Gun의 발사 함수 호출

        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("적 Gun 발사 시작!"));
    }
}

void AEnemy::StopGunFiring()
{
    if (equippedGun)
    {
        isFiring = false;
        equippedGun->StopFire(); // Gun의 발사 중지 함수 호출

        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("적 Gun 발사 중지!"));
    }
}

void AEnemy::ReloadGun()
{
    if (equippedGun)
    {
        equippedGun->Reload(); // Gun의 재장전 함수 호출

        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("적 Gun 재장전 중..."));
    }
}

FTransform AEnemy::GetRightHandIKTransform() const
{
    // Gun이 장착되어 있지 않은 경우
    if (!equippedGun)
    {
        // 기본 오른손 위치 반환 (캐릭터 메시의 오른손 소켓 위치)
        USkeletalMeshComponent* characterMesh = GetMesh();

        if (characterMesh && characterMesh->DoesSocketExist(rightHandSocketName))
            return characterMesh->GetSocketTransform(rightHandSocketName, RTS_World);

        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("적 Gun이 장착되지 않음! Enemy::GetRightHandIKTransform() 호출됨."));

        return FTransform::Identity;
    }

    // Gun에서 오른손 파지 위치 가져오기
    return equippedGun->GetRightHandGripTransform();
}

FTransform AEnemy::GetLeftHandIKTransform() const
{
    // Gun이 장착되어 있지 않은 경우
    if (!equippedGun)
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("적 Gun이 장착되지 않음! 기본 왼손 위치 사용"));

        // 기본 왼손 위치 반환 (캐릭터 메시의 왼손 소켓 위치)
        USkeletalMeshComponent* characterMesh = GetMesh();

        if (characterMesh && characterMesh->DoesSocketExist(leftHandSocketName))
        {
            FTransform defaultTransform = characterMesh->GetSocketTransform(leftHandSocketName, RTS_World);
            return defaultTransform;
        }

        return FTransform::Identity;
    }

    // Gun에서 왼손 보조 파지 위치 가져오기
    FTransform leftHandTransform = equippedGun->GetLeftHandIKTransform();

    // 유효성 검사 - Identity가 아닌지 확인
    if (leftHandTransform.Equals(FTransform::Identity))
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("적 Gun 왼손 IK 위치가 유효하지 않음"));

        // 대체 위치 계산: Gun의 현재 위치에서 앞쪽으로 오프셋
        if (equippedGun)
        {
            FVector gunLocation = equippedGun->GetActorLocation();
            FRotator gunRotation = equippedGun->GetActorRotation();
            FVector leftHandOffset = FVector(20.0f, 0.0f, 0.0f); // 앞쪽 20cm
            FVector leftHandLocation = gunLocation + gunRotation.RotateVector(leftHandOffset);
            leftHandTransform = FTransform(gunRotation, leftHandLocation, FVector::OneVector);
        }
    }

    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Cyan,
        FString::Printf(TEXT("적 왼손 IK: %s"), *leftHandTransform.GetLocation().ToString()));

    return leftHandTransform;
}

void AEnemy::Fire()
{
    // 새로운 Gun 시스템 사용
    StartGunFiring();
}

void AEnemy::StartFire()
{
    isFire = true;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_AutoFire, this, &AEnemy::Fire, fireRate, true);
}

void AEnemy::StopFire()
{
    isFire = false;
    GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoFire);

    // 새로운 Gun 시스템도 중지
    StopGunFiring();
}

void AEnemy::Death()
{
    // 사망 처리
    SetActorEnableCollision(false);

    // 무기 발사 중지
    StopFire();

    // 아이템 드롭 등 기존 로직...
}