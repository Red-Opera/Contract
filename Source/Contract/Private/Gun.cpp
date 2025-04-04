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
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player does not exist."));
        return;
    }

    if (playerController == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerController does not exist."));
        return;
    }

    UInputComponent* inputComponent = playerController->InputComponent;

    if (inputComponent == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InputComponent does not exist."));
        return;
    }

    if (bulletBlueprint == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BulletBlueprint does not exist."));
        return;
    }

    // Niagara 이펙트 존재 확인
    if (gunMuzzleFireNiagara == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Gun Fire Niagara does not exist."));
        return;
    }

    FString assetPath = TEXT("DataAsset'/Game/PlayerInventory/PlayerInventory.PlayerInventory'");
    playerInventory = Cast<UPlayerInventory>(StaticLoadObject(UPlayerInventory::StaticClass(), nullptr, *assetPath));

	if (playerInventory == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player Inventory is null! Check asset path."));
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
	FString ammoCountStr = FString::Printf(TEXT("Current Ammo: %d"), playerInventory->bulletCount);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, ammoCountStr);
}