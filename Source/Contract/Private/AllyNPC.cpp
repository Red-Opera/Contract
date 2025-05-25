#include "AllyNPC.h"
#include "DrawDebugHelpers.h"

#include "Components/SkeletalMeshComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AAllyNPC::AAllyNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// Pawn 감지 컴포넌트 설정
	pawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComponent"));
	pawnSensingComp->SetPeripheralVisionAngle(60.0f);
	pawnSensingComp->SightRadius = 1500.0f;

	// 무기 메시 생성 및 캐릭터 메시에 부착
	weaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	weaponMesh->SetupAttachment(GetMesh());

	// 이동 설정 - 이동 방향으로만 회전하도록 설정
	GetCharacterMovement()->bUseControllerDesiredRotation = false; // AI 컨트롤러 회전 비활성화
	GetCharacterMovement()->bOrientRotationToMovement = true;      // 이동 방향으로 자동 회전 활성화
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	
	// 자연스러운 회전을 위한 설정
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 200.0f, 0.0f); // 더 느린 회전 속도로 조정
	
	// 가속 및 감속 설정 - 부드러운 이동 시작/정지를 위함
	UCharacterMovementComponent* movementComp = GetCharacterMovement();
	if (movementComp)
	{
		// 가속/감속 값 조정 - 낮은 값일수록 더 부드러운 전환
		movementComp->MaxAcceleration = 800.0f;            // 기본값보다 낮춤 (원래 2048.0f)
		movementComp->BrakingDecelerationWalking = 800.0f; // 기본값보다 낮춤 (원래 2048.0f)
		movementComp->GroundFriction = 4.0f;               // 기본값보다 낮춤 (원래 8.0f)
		
		// 부드러운 이동을 위한 추가 설정
		movementComp->bRequestedMoveUseAcceleration = true;
		
		// 회전 속도 조정
		movementComp->RotationRate = FRotator(0.0f, 200.0f, 0.0f);
	}
	
	// 공중에서도 자연스러운 이동을 위한 설정
	GetCharacterMovement()->BrakingDecelerationFalling = 600.0f;
	GetCharacterMovement()->AirControl = 0.3f; // 공중에서 약간의 제어 허용
	
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

	// 게임 시작 시 무기 장착
	EquipWeapon();
}

// Called every frame
void AAllyNPC::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	// 발사 로직 처리
	// AI 컨트롤러에서 발사 여부를 결정하고, 실제 발사는 여기서 수행
	if (isFiring)
	{
		// 발사 간격 타이머 증가
		timeSinceLastShot += deltaTime;

		// 발사 간격에 도달하면 발사 실행
		if (timeSinceLastShot >= fireRate)
		{
			FireWeapon();
			timeSinceLastShot = 0.0f; // 타이머 초기화
		}
	}
}

// Called to bind functionality to input
void AAllyNPC::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
	Super::SetupPlayerInputComponent(playerInputComponent);
	// NPC이므로 플레이어 입력은 사용하지 않음
}

void AAllyNPC::UpdateMovementState(bool isRunning, const FVector& direction)
{
	// 이동 벡터를 애니메이션 시스템을 위해 업데이트
	// 실제 이동은 AI 컨트롤러에서 처리되고, 여기서는 애니메이션 파라미터만 설정
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
	
	// 이전 벡터 저장 (필요시 사용)
	previousMovementVector = movementVector;
}

void AAllyNPC::StartFiring()
{
	isFiring = true;
	timeSinceLastShot = fireRate; // 즉시 첫 발사가 가능하도록 설정
}

void AAllyNPC::StopFiring()
{
	isFiring = false;
}

void AAllyNPC::EquipWeapon()
{
	// 무기 장착 로직 (향후 구현)
}

void AAllyNPC::FireWeapon()
{
	// 총알 발사 효과 및 로직
	FVector muzzleLocation = weaponMesh->GetSocketLocation("MuzzleFlash");
	FRotator muzzleRotation = weaponMesh->GetSocketRotation("MuzzleFlash");

	// 라인 트레이스를 사용한 간단한 총알 로직
	FVector traceEnd = muzzleLocation + muzzleRotation.Vector() * 10000.0f;

	// 충돌 검사 설정
	FHitResult hitResult;
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this); // 자기 자신과의 충돌 무시

	// 라인 트레이스 수행
	if (GetWorld()->LineTraceSingleByChannel(hitResult, muzzleLocation, traceEnd, ECC_Visibility, queryParams))
	{
		// 물체에 명중했을 경우 (빨간색 라인으로 표시)
		DrawDebugLine(GetWorld(), muzzleLocation, hitResult.ImpactPoint, FColor::Red, false, 1.0f, 0, 1.0f);

		// 데미지 처리 로직 (향후 구현)
	}
	else
	{
		// 명중하지 않았을 경우 (파란색 라인으로 표시)
		DrawDebugLine(GetWorld(), muzzleLocation, traceEnd, FColor::Blue, false, 1.0f, 0, 1.0f);
	}
}