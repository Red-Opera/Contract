// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "FloatingDamage.h"
#include "IDToItem.h"

// TPS Kit GASP 시스템을 위한 추가 헤더
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"

// AI 시스템 관련 헤더 추가
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

// Sets default values
AEnemy::AEnemy()
{
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // === HP 시스템 초기화 ===
    currentHP = maxHP;

    // === TPS Kit GASP 시스템 초기화 ===
    isInCombat = false;
    isBurstFiring = false;
    canBurstFire = true;
    currentTarget = nullptr;
    lastKnownTargetLocation = FVector::ZeroVector;
    lastTargetVisibleTime = 0.0f;

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

    // === 이동 시스템 초기화 ===
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

    // 자연스러운 회전을 위한 설정
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 200.0f, 0.0f);

    // 가속 및 감속 설정
    UCharacterMovementComponent* movementComp = GetCharacterMovement();
    if (movementComp)
    {
        movementComp->MaxAcceleration = 1000.0f;
        movementComp->BrakingDecelerationWalking = 2000.0f;
        movementComp->BrakingFriction = 2.0f;
        movementComp->GroundFriction = 8.0f;
        movementComp->BrakingDecelerationFalling = 600.0f;
        movementComp->AirControl = 0.3f;
        movementComp->AirControlBoostMultiplier = 2.0f;
        movementComp->AirControlBoostVelocityThreshold = 25.0f;
        movementComp->bUseSeparateBrakingFriction = true;
        movementComp->BrakingFrictionFactor = 2.0f;
    }
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
    Super::BeginPlay();
    
    // 초기화
    UpdateBlackboardValues();
    
    // === 블랙보드 초기화 ===
    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
        {
            // 🔧 초기값 설정
            BlackboardComp->SetValueAsBool(TEXT("IsAlert"), false);
            BlackboardComp->SetValueAsFloat(TEXT("Health"), currentHP);
            BlackboardComp->SetValueAsFloat(TEXT("Ammo"), MaxAmmo);
            BlackboardComp->SetValueAsVector(TEXT("StartLocation"), GetActorLocation());
            BlackboardComp->SetValueAsVector(TEXT("PatrolLocation"), GetActorLocation());
            
            UE_LOG(LogTemp, Log, TEXT("Enemy: 블랙보드 초기화 완료"));
        }
    }

    // 게임 시작 시 Gun 장착
    EquipGun();

    // 플레이어 참조 설정
    player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    idToItem = Cast<UIDToItem>(UGameplayStatics::GetGameInstance(GetWorld()));

    // HP 초기화
    currentHP = maxHP;
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 매 프레임 업데이트
    UpdateBlackboardValues();
    
    // 기존 업데이트 함수들
    UpdateTargetTracking(DeltaTime);
    UpdateCombatBehavior(DeltaTime);
    UpdateMovementParameters(DeltaTime);
    
    // 디버그 출력 (5초마다)
    static float debugTimer = 0.0f;
    debugTimer += DeltaTime;
    if (debugTimer >= 5.0f)
    {
        if (GEngine->bEnableOnScreenDebugMessages)
        {
            DebugBlackboardValues();
        }
        debugTimer = 0.0f;
    }
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemy::SetDamage(FVector hitLocation, int damage)
{
    currentHP -= damage;

    if (currentHP <= 0)
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

        // 데미지를 받으면 전투 모드 진입
        if (player && !isInCombat)
        {
            EnterCombatMode(player);
        }
    }
}

// === TPS Kit GASP 시스템 - 전투 상태 관리 ===

void AEnemy::EnterCombatMode(AActor* Target)
{
    if (!Target || isDead) return;

    // 전투 상태 변경
    isInCombat = true;
    currentTarget = Target;
    lastKnownTargetLocation = Target->GetActorLocation();
    lastTargetVisibleTime = GetWorld()->GetTimeSeconds();

    // 이동 속도를 전투 모드로 변경
    GetCharacterMovement()->MaxWalkSpeed = runSpeed;

    // === 블랙보드 즉시 업데이트 ===
    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
        {
            BlackboardComp->SetValueAsBool(TEXT("IsInCombat"), true);
            BlackboardComp->SetValueAsObject(TEXT("TargetActor"), Target);
            BlackboardComp->SetValueAsVector(TEXT("LastKnownPlayerLocation"), Target->GetActorLocation());
            BlackboardComp->SetValueAsBool(TEXT("IsAlert"), false); // Combat 중에는 Alert 해제
            
            // 거리 계산
            float distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
            BlackboardComp->SetValueAsFloat(TEXT("FireDistance"), distance);
            
            UE_LOG(LogTemp, Warning, TEXT("Enemy: IsInCombat = true 설정, Target = %s, Distance = %.1f"), 
                *Target->GetName(), distance);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Enemy entered combat mode with target: %s"), 
        *Target->GetName());
}

void AEnemy::ExitCombatMode()
{
    if (!isInCombat) return;

    // 전투 상태 해제
    isInCombat = false;
    currentTarget = nullptr;

    // 버스트 파이어 중단
    if (isBurstFiring)
    {
        StopBurstFire();
    }

    // 일반 발사도 중단
    if (isFiring)
    {
        StopGunFiring();
    }

    // 이동 속도를 일반 모드로 변경
    GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

    // 블랙보드 업데이트
    UpdateBlackboardValues();

    UE_LOG(LogTemp, Warning, TEXT("Enemy exited combat mode"));
}

void AEnemy::StartBurstFire()
{
    if (!canBurstFire || !isInCombat || !HasValidTarget()) return;

    isBurstFiring = true;
    canBurstFire = false;

    // 버스트 파이어 시작
    StartGunFiring();

    // 안전한 블랙보드 접근
    if (AAIController* aiController = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* blackboard = aiController->GetBlackboardComponent())
        {
            blackboard->SetValueAsBool(TEXT("IsBurstFiring"), true);
        }
    }

    // 버스트 지속 시간 후 자동 중지
    GetWorld()->GetTimerManager().SetTimer(burstFireTimer, this, &AEnemy::StopBurstFire, burstFireDuration, false);

    UE_LOG(LogTemp, Warning, TEXT("Enemy started burst fire"));
}

void AEnemy::StopBurstFire()
{
    if (!isBurstFiring) return;

    isBurstFiring = false;
    StopGunFiring();

    // 안전한 블랙보드 접근
    if (AAIController* aiController = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* blackboard = aiController->GetBlackboardComponent())
        {
            blackboard->SetValueAsBool(TEXT("IsBurstFiring"), false);
        }
    }

    // 쿨다운 타이머 설정
    GetWorld()->GetTimerManager().SetTimer(burstFireCooldownTimer, [this]()
    {
        canBurstFire = true;
    }, burstFireCooldown, false);

    UE_LOG(LogTemp, Warning, TEXT("Enemy stopped burst fire"));
}

void AEnemy::SetTargetActor(AActor* Target)
{
    // 🔧 currentTarget만 사용
    currentTarget = Target;
    
    if (currentTarget)
    {
        UE_LOG(LogTemp, Log, TEXT("Enemy: 타겟 설정됨 - %s"), *currentTarget->GetName());
        
        // 즉시 블랙보드 업데이트
        if (AAIController* AIController = Cast<AAIController>(GetController()))
        {
            if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
            {
                BlackboardComp->SetValueAsObject(TEXT("TargetActor"), currentTarget);
                BlackboardComp->SetValueAsVector(TEXT("TargetLocation"), currentTarget->GetActorLocation());
                BlackboardComp->SetValueAsBool(TEXT("IsAlert"), true);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Enemy: 타겟 해제됨"));
    }
}

bool AEnemy::HasValidTarget() const
{
    return currentTarget != nullptr && IsValid(currentTarget) && !isDead;
}

float AEnemy::GetDistanceToTarget() const
{
    if (!HasValidTarget()) return -1.0f;
    
    return FVector::Dist(GetActorLocation(), currentTarget->GetActorLocation());
}

bool AEnemy::IsInOptimalCombatRange() const
{
    if (!HasValidTarget()) return false;
    
    float distance = GetDistanceToTarget();
    return distance <= optimalCombatDistance && distance >= 200.0f; // 최소 200유닛 거리 유지
}

// === TPS Kit GASP 시스템 - 업데이트 함수들 ===

void AEnemy::UpdateTargetTracking(float deltaTime)
{
    // 타겟이 없고 전투 상태가 아니면 플레이어 탐지
    if (!HasValidTarget() && !isInCombat && player)
    {
        float distanceToPlayer = FVector::Dist(GetActorLocation(), player->GetActorLocation());
        
        if (distanceToPlayer <= engagementDistance)
        {
            // 시야각 체크 (간단한 구현)
            FVector toPlayer = (player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
            FVector forward = GetActorForwardVector();
            float dotProduct = FVector::DotProduct(forward, toPlayer);
            
            // 전방 120도 시야각 내에 플레이어가 있으면 전투 모드 진입
            if (dotProduct > -0.5f) // cos(120도) = -0.5
            {
                UE_LOG(LogTemp, Warning, TEXT("Enemy: 플레이어 시야 감지! 전투 모드 진입"));
                EnterCombatMode(player);
                return; // 즉시 리턴하여 아래 로직 실행 방지
            }
        }
    }

    // 타겟 메모리 시스템
    if (HasValidTarget() && currentTarget)
    {
        // 타겟이 보이는 경우
        lastKnownTargetLocation = currentTarget->GetActorLocation();
        lastTargetVisibleTime = GetWorld()->GetTimeSeconds();
        
        // 타겟이 너무 멀어지면 전투 해제
        float distance = GetDistanceToTarget();
        if (distance > disengagementDistance)
        {
            HandleTargetLoss();
        }
    }
    else if (isInCombat)
    {
        // 타겟을 잃었지만 기억 중인 경우
        float currentTime = GetWorld()->GetTimeSeconds();
        float timeSinceLastSeen = currentTime - lastTargetVisibleTime;
        
        if (timeSinceLastSeen > targetMemoryDuration)
        {
            HandleTargetLoss();
        }
    }
}

void AEnemy::UpdateCombatBehavior(float deltaTime)
{
    if (!isInCombat || !HasValidTarget()) return;

    // 타겟을 향해 바라보기
    FVector toTarget = (currentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    FRotator targetRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), currentTarget->GetActorLocation());
    
    // 부드러운 회전
    FRotator currentRotation = GetActorRotation();
    FRotator newRotation = UKismetMathLibrary::RInterpTo(currentRotation, targetRotation, deltaTime, 3.0f);
    
    // Pitch와 Roll은 제한
    newRotation.Pitch = currentRotation.Pitch;
    newRotation.Roll = currentRotation.Roll;
    
    SetActorRotation(newRotation);
}

void AEnemy::UpdateMovementParameters(float deltaTime)
{
    // 현재 속도 벡터 가져오기
    FVector velocity = GetVelocity();
    velocity.Z = 0.0f; // Z축 속도 제거
    
    // 로컬 좌표계로 변환
    FVector localVelocity = GetActorTransform().InverseTransformVectorNoScale(velocity);
    
    // 이동 벡터 정규화 및 스케일링
    float maxSpeed = GetCharacterMovement()->MaxWalkSpeed;
    FVector2D targetMovementVector = FVector2D::ZeroVector;
    
    if (maxSpeed > 0.0f)
    {
        targetMovementVector.X = localVelocity.X / maxSpeed;
        targetMovementVector.Y = localVelocity.Y / maxSpeed;
    }
    
    // 부드러운 보간
    movementVector = FMath::Vector2DInterpTo(movementVector, targetMovementVector, deltaTime, movementVectorInterpSpeed);
}

void AEnemy::UpdateBlackboardValues()
{
    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
        {
            // === 전투 상태를 최우선으로 업데이트 ===
            BlackboardComp->SetValueAsBool(TEXT("IsInCombat"), isInCombat);
            BlackboardComp->SetValueAsBool(TEXT("IsAlert"), isInCombat || (currentTarget != nullptr));
            BlackboardComp->SetValueAsBool(TEXT("IsBurstFiring"), isBurstFiring);
            
            // 발사 관련
            BlackboardComp->SetValueAsFloat(TEXT("FireDelay"), fireRate);
            
            // MoveTo 결정에 중요한 FireDistance
            float distanceToTarget = currentTarget ? 
                FVector::Dist(GetActorLocation(), currentTarget->GetActorLocation()) : 9999.0f;

            BlackboardComp->SetValueAsFloat(TEXT("FireDistance"), distanceToTarget);
            
            // 타겟 정보 업데이트
            if (currentTarget && IsValid(currentTarget))
            {
                BlackboardComp->SetValueAsObject(TEXT("TargetActor"), currentTarget);
                BlackboardComp->SetValueAsVector(TEXT("LastKnownPlayerLocation"), currentTarget->GetActorLocation());
            }
            else
            {
                BlackboardComp->SetValueAsObject(TEXT("TargetActor"), nullptr);
            }
        }
    }
}

bool AEnemy::CanEngageTarget(AActor* Target) const
{
    if (!Target || isDead) return false;
    
    float distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
    return distance <= engagementDistance;
}

void AEnemy::HandleTargetLoss()
{
    if (currentTarget)
    {
        lastKnownTargetLocation = currentTarget->GetActorLocation();
        currentTarget = nullptr;
        
        // Alert 상태로 전환하지만 즉시 Combat 해제하지 않음
        if (AAIController* aiController = Cast<AAIController>(GetController()))
        {
            if (UBlackboardComponent* blackboard = aiController->GetBlackboardComponent())
            {
                blackboard->SetValueAsBool(TEXT("IsAlert"), true);
                blackboard->SetValueAsVector(TEXT("LastKnownPlayerLocation"), lastKnownTargetLocation);
            }
        }
    }
}

// === 이동 시스템 구현 ===

void AEnemy::UpdateMovementState(bool isRunning, const FVector& direction)
{
    // 이동 속도 설정
    float targetSpeed = isRunning ? runSpeed : walkSpeed;
    GetCharacterMovement()->MaxWalkSpeed = targetSpeed;

    // 이동 벡터를 애니메이션 시스템을 위해 업데이트
    UpdateMovementVector(direction, isRunning);
}

void AEnemy::UpdateMovementVector(const FVector& direction, bool isRunning)
{
    // 월드 좌표계의 이동 방향을 캐릭터 로컬 좌표계로 변환
    FVector localDirection = GetActorTransform().InverseTransformVectorNoScale(direction);

    // 목표 이동 벡터 계산 (달리기 상태에 따라 스케일 조정)
    FVector2D targetMovementVector;
    targetMovementVector.X = localDirection.X * (isRunning ? 2.0f : 1.0f);
    targetMovementVector.Y = localDirection.Y * (isRunning ? 2.0f : 1.0f);

    // 부드러운 변화를 위한 이동 벡터 보간 적용
    if (GetWorld())
    {
        float deltaTime = GetWorld()->GetDeltaSeconds();
        movementVector = FMath::Vector2DInterpTo(movementVector, targetMovementVector, deltaTime, movementVectorInterpSpeed);
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
    if (!gunBlueprint)
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy Gun Blueprint not set! Please set in Blueprint."));
        return;
    }

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
        UE_LOG(LogTemp, Log, TEXT("Enemy Gun equipped successfully."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn Enemy Gun actor."));
    }
}

void AEnemy::UnequipGun()
{
    if (equippedGun == nullptr) return;

    // Gun을 소켓에서 분리
    DetachGunFromSocket();

    // Gun 액터 삭제
    equippedGun->Destroy();
    equippedGun = nullptr;

    UE_LOG(LogTemp, Log, TEXT("Enemy Gun unequipped successfully."));
}

void AEnemy::AttachGunToSocket()
{
    if (!equippedGun) return;

    USkeletalMeshComponent* characterMesh = GetMesh();
    if (!characterMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Enemy character mesh not found!"));
        return;
    }

    // 오른손 소켓 존재 확인
    if (!characterMesh->DoesSocketExist(rightHandSocketName))
    {
        UE_LOG(LogTemp, Error, TEXT("Enemy socket '%s' not found!"), *rightHandSocketName.ToString());
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

    UE_LOG(LogTemp, Log, TEXT("Enemy Gun attached to socket '%s'."), *rightHandSocketName.ToString());
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
        equippedGun->StartFire();
        UE_LOG(LogTemp, Log, TEXT("Enemy Gun firing started!"));
    }
}

void AEnemy::StopGunFiring()
{
    if (equippedGun)
    {
        isFiring = false;
        equippedGun->StopFire();
        UE_LOG(LogTemp, Log, TEXT("Enemy Gun firing stopped!"));
    }
}

void AEnemy::ReloadGun()
{
    if (equippedGun)
    {
        equippedGun->Reload();
        UE_LOG(LogTemp, Log, TEXT("Enemy Gun reloading..."));
    }
}

FTransform AEnemy::GetRightHandIKTransform() const
{
    // Gun이 장착되어 있지 않은 경우
    if (!equippedGun)
    {
        USkeletalMeshComponent* characterMesh = GetMesh();
        if (characterMesh && characterMesh->DoesSocketExist(rightHandSocketName))
            return characterMesh->GetSocketTransform(rightHandSocketName, RTS_World);
        
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
        USkeletalMeshComponent* characterMesh = GetMesh();
        if (characterMesh && characterMesh->DoesSocketExist(leftHandSocketName))
        {
            return characterMesh->GetSocketTransform(leftHandSocketName, RTS_World);
        }
        return FTransform::Identity;
    }

    // Gun에서 왼손 보조 파지 위치 가져오기
    FTransform leftHandTransform = equippedGun->GetLeftHandIKTransform();

    // 유효성 검사
    if (leftHandTransform.Equals(FTransform::Identity))
    {
        // 대체 위치 계산: Gun의 현재 위치에서 앞쪽으로 오프셋
        if (equippedGun)
        {
            FVector gunLocation = equippedGun->GetActorLocation();
            FRotator gunRotation = equippedGun->GetActorRotation();
            FVector leftHandOffset = FVector(20.0f, 0.0f, 0.0f);
            FVector leftHandLocation = gunLocation + gunRotation.RotateVector(leftHandOffset);
            leftHandTransform = FTransform(gunRotation, leftHandLocation, FVector::OneVector);
        }
    }

    return leftHandTransform;
}

// === 기존 호환성을 위한 함수들 ===

void AEnemy::Fire()
{
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
    StopGunFiring();
}

void AEnemy::Death()
{
    // 사망 상태 설정
    isDead = true;
    
    // 모든 전투 활동 중지
    ExitCombatMode();
    
    // 물리적 상호작용 비활성화
    SetActorEnableCollision(false);

    // 무기 발사 중지
    StopFire();

    // AI 비활성화
    if (AAIController* aiController = Cast<AAIController>(GetController()))
    {
        //aiController->GetBrainComponent()->StopLogic(TEXT("Dead"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Enemy has died"));

    // 아이템 드롭 등 추가 로직은 여기에...
}

// Enemy.cpp에 추가할 함수들

void AEnemy::ForceInitializeBlackboard()
{
    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
        {
            UE_LOG(LogTemp, Warning, TEXT("=== 강제 블랙보드 초기화 ==="));
            
            // 🔧 기본값 강제 설정
            BlackboardComp->SetValueAsBool(TEXT("IsAlert"), false);
            BlackboardComp->SetValueAsFloat(TEXT("Health"), currentHP);
            BlackboardComp->SetValueAsFloat(TEXT("Ammo"), CurrentAmmo);
            BlackboardComp->SetValueAsVector(TEXT("StartLocation"), GetActorLocation());
            BlackboardComp->SetValueAsVector(TEXT("SelfLocation"), GetActorLocation());
            BlackboardComp->SetValueAsFloat(TEXT("MovementSpeed"), walkSpeed);
            
            // 🔧 currentTarget만 사용
            if (currentTarget && IsValid(currentTarget))
            {
                BlackboardComp->SetValueAsObject(TEXT("TargetActor"), currentTarget);
                BlackboardComp->SetValueAsVector(TEXT("TargetLocation"), currentTarget->GetActorLocation());
                BlackboardComp->SetValueAsVector(TEXT("LastKnownLocation"), currentTarget->GetActorLocation());
            }
            else
            {
                BlackboardComp->SetValueAsObject(TEXT("TargetActor"), nullptr);
                BlackboardComp->SetValueAsVector(TEXT("TargetLocation"), FVector::ZeroVector);
                BlackboardComp->SetValueAsVector(TEXT("LastKnownLocation"), FVector::ZeroVector);
            }
            
            UE_LOG(LogTemp, Warning, TEXT("블랙보드 강제 초기화 완료"));
            DebugBlackboardValues();
        }
    }
}

void AEnemy::DebugBlackboardValues()
{
    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
        {
            bool isInCombatValue = BlackboardComp->GetValueAsBool(TEXT("IsInCombat"));
            bool isAlertValue = BlackboardComp->GetValueAsBool(TEXT("IsAlert"));
            float fireDistance = BlackboardComp->GetValueAsFloat(TEXT("FireDistance"));
            
            // 콘솔과 화면에 출력
            FString debugText = FString::Printf(TEXT("AI 상태: 전투=%s, 경계=%s, 발사거리=%.2f"),
                isInCombatValue ? TEXT("O") : TEXT("X"),
                isAlertValue ? TEXT("O") : TEXT("X"),
                fireDistance);
            
            GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, debugText);
            UE_LOG(LogTemp, Warning, TEXT("%s"), *debugText);
            
            // 💡 현재 활성화된 브랜치 출력
            FString activeBranch = TEXT("활성 브랜치: ");
            if (isInCombatValue) 
            {
                activeBranch += TEXT("Combat > ");
                if (BlackboardComp->GetValueAsBool(TEXT("IsBurstFiring")))
                    activeBranch += TEXT("Burst Fire");
                else if (fireDistance > 800.0f)
                    activeBranch += TEXT("Chase");
                else
                    activeBranch += TEXT("Strafe");
            }
            else if (isAlertValue)
                activeBranch += TEXT("Alert");
            else
                activeBranch += TEXT("Patrol");
                
            GEngine->AddOnScreenDebugMessage(-2, 1.0f, FColor::Cyan, activeBranch);
        }
    }
}