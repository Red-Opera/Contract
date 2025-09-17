#include "AllyNPC.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

AAllyNPC::AAllyNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// Pawn 감지 컴포넌트 설정
	pawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComponent"));
	pawnSensingComp->SetPeripheralVisionAngle(60.0f);
	pawnSensingComp->SightRadius = 1500.0f;

	// Gun 시스템 초기화
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

	// 이동 설정 - 이동 방향으로만 회전하도록 설정
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	
	// 자연스러운 회전을 위한 설정
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 200.0f, 0.0f);
	
	// 가속 및 감속 설정 - 부드러운 이동 시작/정지
	UCharacterMovementComponent* movementComp = GetCharacterMovement();
	if (movementComp)
	{
		movementComp->MaxAcceleration = 800.0f;
		movementComp->BrakingDecelerationWalking = 800.0f;
		movementComp->GroundFriction = 4.0f;
		movementComp->bRequestedMoveUseAcceleration = true;
		movementComp->RotationRate = FRotator(0.0f, 200.0f, 0.0f);
	}
	
	// 공중에서도 자연스러운 이동을 위한 설정
	GetCharacterMovement()->BrakingDecelerationFalling = 600.0f;
	GetCharacterMovement()->AirControl = 0.3f;
	
	// Pawn이 컨트롤러 회전을 사용하지 않도록 설정
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// 전투 변수 초기화
	isFiring = false;
	timeSinceLastShot = 0.0f;

	// 애니메이션 변수 초기화
	previousMovementVector = FVector2D::ZeroVector;
}

void AAllyNPC::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 Gun 장착
	EquipGun();
}

void AAllyNPC::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	// Gun 발사 로직 처리
	if (isFiring && equippedGun)
	{
		timeSinceLastShot += deltaTime;
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

// === Gun 시스템 구현 ===

void AAllyNPC::EquipGun()
{
    // Gun 블루프린트가 설정되어 있는지 확인
    if (!gunBlueprint)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Gun 블루프린트가 설정되지 않음! 블루프린트에서 설정 필요!"));

        return;
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("Gun 블루프린트: %s"), *gunBlueprint->GetName()));

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
		// Gun을 왼손 소켓에 부착
		AttachGunToSocket();
		
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("Gun이 성공적으로 장착되었습니다.")));
	}

	else
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("Gun 액터 생성에 실패했습니다.")));
}

void AAllyNPC::UnequipGun()
{
	if (equippedGun == nullptr)
		return;

	// Gun을 소켓에서 분리
	DetachGunFromSocket();

	// Gun 액터 삭제
	equippedGun->Destroy();
	equippedGun = nullptr;

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Gun이 성공적으로 해제되었습니다."));
}

void AAllyNPC::AttachGunToSocket()
{
    if (!equippedGun)
        return;

    USkeletalMeshComponent* characterMesh = GetMesh();
    if (!characterMesh)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("캐릭터 메시를 찾을 수 없음!"));

        return;
    }

    // 오른손 소켓 존재 확인 (Gun을 오른손에 장착)
    if (!characterMesh->DoesSocketExist(rightHandSocketName))
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("소켓 '%s'을 찾을 수 없음!"), *rightHandSocketName.ToString()));

        return;
    }

    // Gun을 오른손 소켓에 부착 - SnapToTarget 대신 KeepRelativeTransform 사용
    equippedGun->AttachToComponent(
        characterMesh,
        FAttachmentTransformRules::KeepRelativeTransform,
        rightHandSocketName
    );

    // Gun 위치와 회전을 더 정확하게 조정
    FTransform adjustedTransform = gunAttachOffset;
    
    // Gun이 캐릭터의 앞을 향하도록 회전 조정
    FRotator adjustedRotation = adjustedTransform.GetRotation().Rotator();
    adjustedRotation.Yaw += 0.0f;   // 필요에 따라 조정
    adjustedRotation.Pitch += 0.0f; // 필요에 따라 조정
    adjustedRotation.Roll += 0.0f;  // 필요에 따라 조정
    
    adjustedTransform.SetRotation(adjustedRotation.Quaternion());
    equippedGun->SetActorRelativeTransform(adjustedTransform);

    // NPC 장착 상태 설정
    equippedGun->SetEquippedByNPC(true);

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Gun이 소켓에 '%s'에 부착되었습니다."), *rightHandSocketName.ToString()));
    
    // 디버그 정보 출력
    FVector gunLocation = equippedGun->GetActorLocation();
    FRotator gunRotation = equippedGun->GetActorRotation();
}

void AAllyNPC::DetachGunFromSocket()
{
	if (equippedGun)
	{
		// NPC 장착 상태 해제
		equippedGun->SetEquippedByNPC(false);
		
		equippedGun->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}

void AAllyNPC::StartGunFiring()
{
	if (equippedGun)
	{
		isFiring = true;
		timeSinceLastShot = fireRate; // 즉시 첫 발사 가능
		equippedGun->StartFire(); // Gun의 발사 함수 호출

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Gun 발사 시작!"));
	}
}

void AAllyNPC::StopGunFiring()
{
	if (equippedGun)
	{
		isFiring = false;
		equippedGun->StopFire(); // Gun의 발사 중지 함수 호출

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Gun 발사 중지!"));
	}
}

void AAllyNPC::ReloadGun()
{
	if (equippedGun)
	{
		equippedGun->Reload(); // Gun의 재장전 함수 호출

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Gun 재장전 중..."));
	}
}

FTransform AAllyNPC::GetRightHandIKTransform() const
{
    // Gun이 장착되어 있지 않은 경우
    if (!equippedGun)
    {
        // 기본 오른손 위치 반환 (캐릭터 메시의 오른손 소켓 위치)
        USkeletalMeshComponent* characterMesh = GetMesh();

        if (characterMesh && characterMesh->DoesSocketExist(rightHandSocketName))
            return characterMesh->GetSocketTransform(rightHandSocketName, RTS_World);
        
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Gun이 장착되지 않음! AllyNPC::GetRightHandIKTransform() 호출됨."));

        return FTransform::Identity;
    }

    // Gun에서 오른손 파지 위치 가져오기
    return equippedGun->GetRightHandGripTransform();
}

FTransform AAllyNPC::GetLeftHandIKTransform() const
{
    // Gun이 장착되어 있지 않은 경우
    if (!equippedGun)
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("Gun이 장착되지 않음! 기본 왼손 위치 사용"));
        
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
        GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("Gun 왼손 IK 위치가 유효하지 않음"));
        
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
        FString::Printf(TEXT("NPC 왼손 IK: %s"), *leftHandTransform.GetLocation().ToString()));
    
    return leftHandTransform;
}

// === 이동 시스템 ===

void AAllyNPC::UpdateMovementState(bool isRunning, const FVector& direction)
{
    // 이동 벡터를 애니메이션 시스템을 위해 업데이트
    UpdateMovementVector(direction, isRunning);
}

void AAllyNPC::UpdateMovementVector(const FVector& direction, bool isRunning)
{
    // 월드 좌표계의 이동 방향을 캐릭터 로컬 좌표계로 변환
    FVector localDirection = GetActorTransform().InverseTransformVectorNoScale(direction);
    
    // 목표 이동 벡터 계산 (달리기 상태에 따라 스케일 조정)
    FVector2D targetMovementVector;
    targetMovementVector.X = localDirection.X * (isRunning ? 2.0f : 1.0f);  // 앞/뒤 이동
    targetMovementVector.Y = localDirection.Y * (isRunning ? 2.0f : 1.0f);  // 좌/우 이동
    
    // 부드러운 변화를 위한 이동 벡터 보간 적용
    movementVector.X = FMath::FInterpTo(movementVector.X, targetMovementVector.X, GetWorld()->GetDeltaSeconds(), movementVectorInterpSpeed);
    movementVector.Y = FMath::FInterpTo(movementVector.Y, targetMovementVector.Y, GetWorld()->GetDeltaSeconds(), movementVectorInterpSpeed);
    
    // 이전 벡터 저장
    previousMovementVector = movementVector;
}

// === 기존 함수들 (호환성 유지) ===

void AAllyNPC::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
    Super::SetupPlayerInputComponent(playerInputComponent);
    // NPC이므로 플레이어 입력은 사용하지 않음
}

void AAllyNPC::StartFiring()
{
    // 새로운 Gun 시스템 사용
    StartGunFiring();
}

void AAllyNPC::StopFiring()
{
    // 새로운 Gun 시스템 사용
    StopGunFiring();
}