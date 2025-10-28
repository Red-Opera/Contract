#include "Gun.h"
#include "PlayerInventory.h"

#include "NiagaraFunctionLibrary.h"  
#include "NiagaraComponent.h"  

#include "Components/ArrowComponent.h"  
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerController.h"  
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AGun::AGun()
{
    PrimaryActorTick.bCanEverTick = true;

    mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    muzzle = CreateDefaultSubobject<UArrowComponent>(TEXT("Muzzle"));
    gunMuzzleFireNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GunFireNiagara"));
    audioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));

    RootComponent = mesh;
    muzzle->SetupAttachment(mesh);
    gunMuzzleFireNiagara->SetupAttachment(mesh);
    audioComponent->SetupAttachment(mesh);
    muzzle->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    
    // 오디오 컴포넌트 기본 설정
    audioComponent->bAutoActivate = false; // 자동 재생 방지

    // 기본값으로 아이콘 초기화
    itemIcon = nullptr;
}

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
    
    // 발사 사운드 재생
    PlayFireSound();

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
    {
        // 빈 탄창 소리 재생
        PlayEmptyClipSound();

        return;
    }

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
    
    // 재장전 사운드 재생
    PlayReloadSound();

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
        UE_LOG(LogTemp, Warning, TEXT("AGun::GetRightHandGripTransform - 소켓 '%s'을 찾을 수 없습니다."), *rightHandGripSocketName.ToString());
        
        // NPC에 장착된 경우: Gun의 월드 트랜스폼 기준으로 오른손 위치 계산
        if (bIsEquippedByNPC)
        {
            FVector gunLocation = GetActorLocation();
            FRotator gunRotation = GetActorRotation();
            
            // Gun이 이미 오른손에 부착되어 있으므로, Gun의 현재 위치가 오른손 파지 위치
            return FTransform(gunRotation, gunLocation, FVector::OneVector);
        }
        
        // 플레이어용 기본 위치
        FVector gunLocation = GetActorLocation();
        FRotator gunRotation = GetActorRotation();
        FVector rightHandOffset = FVector(-5.0f, -5.0f, 0.0f);
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
        UE_LOG(LogTemp, Warning, TEXT("AGun::GetLeftHandGripTransform - 소켓 '%s'을 찾을 수 없습니다. Gun 메시에 소켓을 추가해주세요."), *leftHandGripSocketName.ToString());
        
        // 디버그 메시지 추가
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
            FString::Printf(TEXT("Gun에 '%s' 소켓이 없습니다!"), *leftHandGripSocketName.ToString()));
        
        // 소켓이 없는 경우 Gun 기준으로 왼손 위치 추정
        FVector gunLocation = GetActorLocation();
        FRotator gunRotation = GetActorRotation();
        
        // Gun의 전방 부분에 왼손 위치 계산 (총신 중간 부분)
        FVector leftHandOffset = FVector(0.0f, 0.0f, 0.0f); // Gun 로컬 좌표계에서 앞쪽 15cm
        FVector leftHandLocation = gunLocation + gunRotation.RotateVector(leftHandOffset);
        
        return FTransform(gunRotation, leftHandLocation, FVector::OneVector);
    }

    // 소켓이 존재하는 경우 소켓의 월드 트랜스폼 반환
    FTransform socketTransform = mesh->GetSocketTransform(leftHandGripSocketName, RTS_World);
    
    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, 
        FString::Printf(TEXT("왼손 소켓 위치: %s"), *socketTransform.GetLocation().ToString()));
    
    return socketTransform;
}

FTransform AGun::GetLeftHandIKTransform() const
{
    // Gun 메시가 유효한지 확인
    if (!mesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("AGun::GetLeftHandIKTransform - Gun 메시가 없습니다."));
        return FTransform::Identity;
    }

    // 왼손 보조 파지 소켓이 존재하는지 확인
    if (!mesh->DoesSocketExist(leftHandGripSocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("AGun::GetLeftHandIKTransform - 소켓 '%s'을 찾을 수 없습니다. Gun 메시에 소켓을 추가해주세요."), *leftHandGripSocketName.ToString());
        
        // 디버그 메시지 추가
        GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, 
            FString::Printf(TEXT("Gun에 '%s' 소켓이 없습니다!"), *leftHandGripSocketName.ToString()));
        
        // 소켓이 없는 경우 Gun 기준으로 왼손 위치 추정
        FVector gunLocation = GetActorLocation();
        FRotator gunRotation = GetActorRotation();
        
        // Gun의 전방 부분에 왼손 위치 계산 (총신 중간 부분)
        FVector leftHandOffset = FVector(15.0f, 0.0f, 0.0f); // Gun 로컬 좌표계에서 앞쪽 15cm
        FVector leftHandLocation = gunLocation + gunRotation.RotateVector(leftHandOffset);
        
        return FTransform(gunRotation, leftHandLocation, FVector::OneVector);
    }

    // 소켓이 존재하는 경우 소켓의 월드 트랜스폼 반환
    FTransform socketTransform = mesh->GetSocketTransform(leftHandGripSocketName, RTS_World);
    
    GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, 
        FString::Printf(TEXT("왼손 소켓 위치: %s"), *socketTransform.GetLocation().ToString()));
    
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
        UE_LOG(LogTemp, Log, TEXT("AGun::SetEquippedByNPC - Gun이 NPC에서 해제되었습니다."));
}

bool AGun::DoesLeftHandSocketExist() const
{
    return mesh && mesh->DoesSocketExist(leftHandGripSocketName);
}

TArray<FName> AGun::GetAvailableSockets() const
{
    TArray<FName> socketNames;

    if (mesh && mesh->GetStaticMesh())
    {
        // 스태틱 메시의 소켓 정보 가져오기
        const TArray<UStaticMeshSocket*>& sockets = mesh->GetStaticMesh()->Sockets;
        for (const UStaticMeshSocket* socket : sockets)
        {
            if (socket)
            {
                socketNames.Add(socket->SocketName);
            }
        }
    }
    return socketNames;
}

void AGun::PrintAvailableSockets() const
{
    if (mesh != nullptr && mesh->GetStaticMesh())
    {
        const TArray<UStaticMeshSocket*>& sockets = mesh->GetStaticMesh()->Sockets;
        UE_LOG(LogTemp, Log, TEXT("Gun에서 사용 가능한 소켓들:"));
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, TEXT("Gun 소켓 목록:"));

        for (int32 i = 0; i < sockets.Num(); i++)
        {
            if (sockets[i])
            {
                FString socketInfo = FString::Printf(TEXT("소켓 %d: %s"), i, *sockets[i]->SocketName.ToString());
                UE_LOG(LogTemp, Log, TEXT("%s"), *socketInfo);
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, socketInfo);
            }
        }

        if (sockets.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Gun 메시에 소켓이 없습니다!"));
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Gun 메시에 소켓이 없습니다!"));
        }
    }

    else
    {
        UE_LOG(LogTemp, Error, TEXT("Gun 메시 또는 스태틱 메시가 없습니다!"));
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Gun 메시가 없습니다!"));
    }
}

void AGun::PlayFireSound()
{
    if (fireSoundCue == nullptr)
        return;

    // UGameplayStatics를 사용한 3D 사운드 재생
    UGameplayStatics::PlaySoundAtLocation(
        GetWorld(),
        fireSoundCue,
        GetActorLocation(),
        GetActorRotation(),
        1.0f, // 볼륨
        1.0f  // 피치
    );
}

void AGun::PlayReloadSound()
{
    if (reloadSoundCue == nullptr)
        return;

    // UGameplayStatics를 사용한 3D 사운드 재생
    UGameplayStatics::PlaySoundAtLocation(
        GetWorld(),
        reloadSoundCue,
        GetActorLocation(),
        GetActorRotation(),
        1.0f, // 볼륨
        1.0f  // 피치
    );
}

void AGun::PlayEmptyClipSound()
{
    if (emptyClipSoundCue == nullptr)
        return;

    // UGameplayStatics를 사용한 3D 사운드 재생
    UGameplayStatics::PlaySoundAtLocation(
        GetWorld(),
        emptyClipSoundCue,
        GetActorLocation(),
        GetActorRotation(),
        1.0f, // 볼륨
        1.0f  // 피치
    );
}