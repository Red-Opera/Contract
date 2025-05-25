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

	// 무기 메시 생성
	weaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	weaponMesh->SetupAttachment(GetMesh());

	// 이동 설정 - 이동 방향으로만 회전하도록 설정
	GetCharacterMovement()->bUseControllerDesiredRotation = false; // AI 컨트롤러 회전 비활성화
	GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향으로 자동 회전 활성화
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	
	// 자연스러운 회전을 위한 설정
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 270.0f, 0.0f); // 빠른 회전 속도
	
	// Pawn이 컨트롤러 회전을 사용하지 않도록 설정
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// 전투 변수 초기화
	isFiring = false;
	timeSinceLastShot = 0.0f;
}

void AAllyNPC::BeginPlay()
{
	Super::BeginPlay();

	// 무기 장착
	EquipWeapon();
}

// Called every frame
void AAllyNPC::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	// 발사 로직 - AI 컨트롤러가 발사 결정, 실제 발사는 여기서 수행
	if (isFiring)
	{
		timeSinceLastShot += deltaTime;

		if (timeSinceLastShot >= fireRate)
		{
			FireWeapon();

			timeSinceLastShot = 0.0f;
		}
	}
}

// Called to bind functionality to input
void AAllyNPC::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
	Super::SetupPlayerInputComponent(playerInputComponent);
}

void AAllyNPC::UpdateMovementState(bool isRunning, const FVector& direction)
{
	// 이동 벡터를 애니메이션 시스템을 위해 업데이트
	UpdateMovementVector(direction, isRunning);
	
	// 실제 이동은 AI 컨트롤러의 MoveTo에 의해 처리됨
	// 여기서는 애니메이션 파라미터만 설정
}

void AAllyNPC::UpdateMovementVector(const FVector& direction, bool isRunning)
{
	// 이동 벡터를 로컬 좌표계로 변환
	FVector localDirection = GetActorTransform().InverseTransformVectorNoScale(direction);
	
	// X: 앞/뒤 이동 (Forward/Backward)
	// Y: 좌/우 이동 (Right/Left)
	movementVector.X = localDirection.X * (isRunning ? 2.0f : 1.0f);  // 앞/뒤 이동
	movementVector.Y = localDirection.Y * (isRunning ? 2.0f : 1.0f);  // 좌/우 이동
}

void AAllyNPC::StartFiring()
{
	isFiring = true;
	timeSinceLastShot = fireRate; // 즉시 첫 발사
}

void AAllyNPC::StopFiring()
{
	isFiring = false;
}

void AAllyNPC::EquipWeapon()
{

}

void AAllyNPC::FireWeapon()
{
	// 총알 발사 효과 및 로직
	FVector muzzleLocation = weaponMesh->GetSocketLocation("MuzzleFlash");
	FRotator muzzleRotation = weaponMesh->GetSocketRotation("MuzzleFlash");

	// 라인 트레이스로 간단한 총알 로직 구현
	FVector traceEnd = muzzleLocation + muzzleRotation.Vector() * 10000.0f;

	FHitResult hitResult;
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(hitResult, muzzleLocation, traceEnd, ECC_Visibility, queryParams))
	{
		// 명중 효과 추가 (여기서는 디버그 라인만 표시)
		DrawDebugLine(GetWorld(), muzzleLocation, hitResult.ImpactPoint, FColor::Red, false, 1.0f, 0, 1.0f);

		// 데미지 로직은 필요시 추가
	}

	else
	{
		DrawDebugLine(GetWorld(), muzzleLocation, traceEnd, FColor::Blue, false, 1.0f, 0, 1.0f);
	}
}