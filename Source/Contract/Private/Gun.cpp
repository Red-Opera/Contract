#include "Gun.h"
#include "PlayerInventory.h"

#include "NiagaraFunctionLibrary.h"  
#include "NiagaraComponent.h"  

#include "Components/ArrowComponent.h"  
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"  

// Sets default values
AGun::AGun()
{
	PrimaryActorTick.bCanEverTick = true;

	mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    muzzle = CreateDefaultSubobject<UArrowComponent>(TEXT("Muzzle"));
    gunMuzzleFireNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GunFireNiagara"));

	RootComponent = mesh;
	muzzle->SetupAttachment(mesh);
	gunMuzzleFireNiagara->SetupAttachment(mesh);
	muzzle->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
}

// Called when the game starts or when spawned
void AGun::BeginPlay()
{
	Super::BeginPlay();

    player = GetWorld()->GetFirstPlayerController()->GetPawn();
    playerController = GetWorld()->GetFirstPlayerController();

    if (player == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player가 존재하지 않음."));
        return;
    }

    if (playerController == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerController가 존재하지 않음."));
        return;
    }

    UInputComponent* inputComponent = playerController->InputComponent;

    if (inputComponent == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InputComponent가 존재하지 않음."));
        return;
    }

    if (bulletBlueprint == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BulletBlueprint가 존재하지 않음."));
        return;
    }

    // Niagara 이펙트 존재 확인
    if (gunMuzzleFireNiagara == nullptr)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Gun Fire Niagara가 존재하지 않음."));
        return;
    }

    FString assetPath = TEXT("DataAsset'/Game/PlayerInventory/PlayerInventory.PlayerInventory'");
    playerInventory = Cast<UPlayerInventory>(StaticLoadObject(UPlayerInventory::StaticClass(), nullptr, *assetPath));

	if (playerInventory == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Inventory가 존재하지 않음."));
		return;
	}

    EnableInput(playerController);

    // 입력 바인딩 설정  
    inputComponent->BindAction(TEXT("GunFire"), IE_Pressed, this, &AGun::StartFire);
    inputComponent->BindAction(TEXT("GunFire"), IE_Released, this, &AGun::StopFire);
	inputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &AGun::Reload);
}

// Called every frame
void AGun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGun::Fire()
{
    if (currentAmmoEquipped <= 0)
    {
        StopFire();
        return;
    }

	currentAmmoEquipped--;

    FVector muzzleLocation = muzzle->GetComponentLocation();
    FRotator muzzleRotation = muzzle->GetComponentRotation();
    FRotator gunFireRotation = gunMuzzleFireNiagara->GetComponentRotation();

    UNiagaraFunctionLibrary::SpawnSystemAtLocation
    (
        GetWorld(),
        gunMuzzleFireNiagara->GetAsset(),
        muzzleLocation,
        gunFireRotation
    );

    FActorSpawnParameters params;

    params.Owner = this;
    params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    GetWorld()->SpawnActor<ABullet>(bulletBlueprint, muzzleLocation, muzzleRotation, params);
}

void AGun::StartFire()
{
    if (currentAmmoEquipped <= 0)
        return;

    isFire = true;

    // 처음 한 번 즉시 발사  
    Fire();

    // 이후 FireRate 간격으로 Fire() 함수를 반복 호출 (FireRate는 발사 간격)  
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_AutoFire, this, &AGun::Fire, fireRate, true);
}

void AGun::StopFire()
{
    isFire = false;

    // 타이머 핸들을 이용하여 반복 호출 중지  
    GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoFire);
}

void AGun::Reload()
{
	int requireAmmo = maxAmmoEquipped - currentAmmoEquipped;

	if (playerInventory->bulletCount >= requireAmmo)
	{
        playerInventory->bulletCount -= requireAmmo;
		currentAmmoEquipped += requireAmmo;
	}

	else
	{
		currentAmmoEquipped += playerInventory->bulletCount;
        playerInventory->bulletCount = 0;
	}

	// 남은 총알 수 출력
	FString ammoCountStr = FString::Printf(TEXT("현재 장착된 총알 수: %d"), currentAmmoEquipped);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, ammoCountStr);
}

FTransform AGun::GetRightHandGripTransform() const
{
    // Gun 메시가 유효한지 확인
    if (!mesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("AGun::GetRightHandGripTransform - Gun 메시가 없습니다."));
        return FTransform::Identity;
    }

    // 오른손 파지 소켓이 존재하는지 확인
    if (!mesh->DoesSocketExist(rightHandGripSocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("AGun::GetRightHandGripTransform - 소켓 '%s'을 찾을 수 없습니다. 기본 위치를 사용합니다."), *rightHandGripSocketName.ToString());
        
        // 소켓이 없는 경우 Gun의 손잡이 부분으로 기본 위치 계산 (오른손용)
        FVector gunLocation = GetActorLocation();
        FRotator gunRotation = GetActorRotation();
        
        // Gun의 손잡이 부분 (오른손이 잡을 위치)
        FVector rightHandOffset = FVector(-5.0f, -5.0f, 0.0f); // 뒤쪽 5cm, 왼쪽 5cm (오른손 기준)
        FVector rightHandLocation = gunLocation + gunRotation.RotateVector(rightHandOffset);
        
        return FTransform(gunRotation, rightHandLocation, FVector::OneVector);
    }

    // 소켓이 존재하는 경우 소켓의 월드 트랜스폼 반환
    FTransform socketTransform = mesh->GetSocketTransform(rightHandGripSocketName, RTS_World);
    return socketTransform;
}

FTransform AGun::GetLeftHandGripTransform() const
{
    // Gun 메시가 유효한지 확인
    if (!mesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("AGun::GetLeftHandGripTransform - Gun 메시가 없습니다."));
        return FTransform::Identity;
    }

    // 왼손 보조 파지 소켓이 존재하는지 확인
    if (!mesh->DoesSocketExist(leftHandGripSocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("AGun::GetLeftHandGripTransform - 소켓 '%s'을 찾을 수 없습니다. 기본 위치를 사용합니다."), *leftHandGripSocketName.ToString());
        
        // 소켓이 없는 경우 Gun의 전방 부분으로 기본 위치 계산 (왼손용)
        FVector gunLocation = GetActorLocation();
        FRotator gunRotation = GetActorRotation();
        
        // Gun의 전방 부분 (왼손이 잡을 위치)
        FVector leftHandOffset = FVector(15.0f, -3.0f, 0.0f); // 앞쪽 15cm, 약간 왼쪽 3cm
        FVector leftHandLocation = gunLocation + gunRotation.RotateVector(leftHandOffset);
        
        return FTransform(gunRotation, leftHandLocation, FVector::OneVector);
    }

    // 소켓이 존재하는 경우 소켓의 월드 트랜스폼 반환
    FTransform socketTransform = mesh->GetSocketTransform(leftHandGripSocketName, RTS_World);
    return socketTransform;
}

void AGun::SetEquippedByNPC(bool bEquipped)
{
    bIsEquippedByNPC = bEquipped;
    
    if (bIsEquippedByNPC)
    {
        UE_LOG(LogTemp, Log, TEXT("AGun::SetEquippedByNPC - Gun이 NPC에 장착되었습니다."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("AGun::SetEquippedByNPC - Gun이 NPC에서 해제되었습니다."));
    }
}