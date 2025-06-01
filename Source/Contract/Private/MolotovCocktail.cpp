#include "MolotovCocktail.h"
#include "PlayerInventory.h"
#include "GameFramework/Character.h"

#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AMolotovCocktail::AMolotovCocktail()
{
    // 충돌 컴포넌트 생성 및 설정
    collisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("collisionComponent"));
    collisionComponent->SetupAttachment(itemMesh);
    
    // 오디오 컴포넌트 초기화
    fireLoopAudioComponent = nullptr;
    fireStartAudioComponent = nullptr;

    // 애니메이션 및 화재 지속 시간 설정
    scaleAnimationDuration = 4.0f; // 스케일 축소 애니메이션 시간: 4초
    fireLoopTime = 8.0f;            // 화재 지속 시간: 8초
    scaleAnimationProgress = 0.0f;
    originalScale = FVector::OneVector;
}

void AMolotovCocktail::BeginPlay()
{
    Super::BeginPlay();

    // 상호작용 입력 바인딩
    playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AMolotovCocktail::AddMolotovCocktail);

    // 충돌 컴포넌트 설정
    if (collisionComponent != nullptr)
    {
        // 물리 쿼리와 충돌 모두 활성화
        collisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        collisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);

        // 기본적으로 모든 채널을 Block하고, Pawn은 Overlap으로 설정
        collisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
        collisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

        // Hit 및 Overlap 이벤트 활성화
        collisionComponent->SetNotifyRigidBodyCollision(true);
        collisionComponent->SetGenerateOverlapEvents(true);

        // Hit 이벤트 바인딩
        collisionComponent->OnComponentHit.AddDynamic(this, &AMolotovCocktail::OnComponentHit);

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("화염병 충돌 컴포넌트 설정 완료"));
    }

    // 아이템 메시 물리 설정
    if (itemMesh != nullptr)
    {
        // 물리 시뮬레이션 및 충돌 활성화
        itemMesh->SetSimulatePhysics(true);
        itemMesh->SetNotifyRigidBodyCollision(true);
        itemMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

        // Hit 이벤트 바인딩
        itemMesh->OnComponentHit.AddDynamic(this, &AMolotovCocktail::OnComponentHit);

        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("화염병 메시 설정 완료"));
    }

    isUse = false;
}

void AMolotovCocktail::OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // 화염병이 사용된 상태이고, 다른 액터와 충돌했을 때 화염 생성
    if (isUse && OtherActor && OtherActor != this)
    {
        // 화염병 깨지는 소리 재생
        if (molotovBreakSoundCue != nullptr)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), molotovBreakSoundCue, Hit.ImpactPoint);
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("Molotov break sound played"));
        }

        // 아이템 메시 제거
        RemoveItemMesh();

        // 충돌 지점에 화염 액터 생성
        FVector SpawnLocation = Hit.ImpactPoint;
        SpawnFireActor(SpawnLocation);
        
        // 화염 시작 사운드 재생
        PlayFireStartSound(SpawnLocation);

        isUse = false;

        // 충돌 정보 디버그 출력
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("화염병 충돌: %s"), *Hit.GetActor()->GetName()));
    }
}

void AMolotovCocktail::UseItem()
{
    Super::UseItem();

    // 아이템 사용 상태로 변경
    isUse = true;
}

void AMolotovCocktail::AddMolotovCocktail()
{
    // 플레이어와의 거리 및 획득 가능 상태 확인
    if (!CheckPlayerIsClose() || !isGetable)
        return;

    // 가장 가까운 상호작용 가능한 아이템 찾기
    AItem* closestItem = AItem::GetClosestInteractableItem(player);

    // 현재 아이템이 가장 가까운 아이템이 아니면 상호작용 무시
    if (closestItem != this)
        return;

    // 플레이어 인벤토리에 화염병 추가
    playerInventory->AddItem(1);
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("화염병 추가"));

    // 아이템 오브젝트 제거
    Destroy();
}

void AMolotovCocktail::RemoveActor()
{
    // 모든 애니메이션 타이머 정리
    GetWorld()->GetTimerManager().ClearTimer(scaleAnimationTimer);
    GetWorld()->GetTimerManager().ClearTimer(startScaleAnimationTimer);
    
    // 남아있을 수 있는 루프 사운드 완전 정지
    StopFireLoopSound();

    // 화염 액터 제거
    if (fireActor != nullptr)
    {
        fireActor->Destroy();
        fireActor = nullptr;
    }

    // 화염병 오브젝트 제거
    Destroy();
}

void AMolotovCocktail::SpawnFireActor(const FVector& spawnLocation)
{
    // 화염 메시 클래스 유효성 검사
    if (!fireMesh)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("폭파 메쉬가 설정되지 않음"));
        return;
    }

    // 월드 객체 유효성 검사
    UWorld* world = GetWorld();
    if (world == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("월드가 유효하지 않음"));
        return;
    }

    // 액터 생성 파라미터 설정
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 충돌 위치에서 약간 위쪽으로 조정하여 화염 액터 생성
    FVector AdjustedLocation = spawnLocation + FVector(0, 0, 10.0f);
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // 화염 액터 스폰
    fireActor = world->SpawnActor<AActor>(fireMesh, AdjustedLocation, SpawnRotation, SpawnParams);

    if (fireActor == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("화재 액터 생성 실패"));
        return;
    }

    // 스케일 애니메이션을 위한 원본 스케일 저장
    originalScale = fireActor->GetActorScale3D();
    
    // 화염 액터 생성 성공 로그
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        FString::Printf(TEXT("Fire actor spawned at %s"), *AdjustedLocation.ToString()));

    // 화염 지속 시간 후 스케일 애니메이션 시작 예약
    GetWorld()->GetTimerManager().SetTimer(startScaleAnimationTimer, this, &AMolotovCocktail::StartScaleAnimation, fireLoopTime, false);
    
    // 전체 지속 시간 (화염 시간 + 애니메이션 시간) 후 완전 제거
    GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &AMolotovCocktail::RemoveActor, fireLoopTime + scaleAnimationDuration, false);
}

void AMolotovCocktail::StartScaleAnimation()
{
    // 화염 액터 유효성 검사
    if (fireActor == nullptr || !IsValid(fireActor))
        return;
    
    // 애니메이션 진행도 초기화
    scaleAnimationProgress = 0.0f;

    // 루프 사운드 정지
    StopFireLoopSound();

    // 화염 종료 사운드 재생
    PlayFireEndSound(fireActor->GetActorLocation());
    
    // 프레임별 스케일 업데이트를 위한 타이머 설정 (50fps)
    GetWorld()->GetTimerManager().SetTimer(scaleAnimationTimer, this, &AMolotovCocktail::UpdateScaleAnimation, 0.02f, true);
    
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Fire scale animation started"));
}

void AMolotovCocktail::UpdateScaleAnimation()
{
    // 화염 액터 유효성 재검사
    if (fireActor == nullptr || !IsValid(fireActor))
    {
        GetWorld()->GetTimerManager().ClearTimer(scaleAnimationTimer);
        return;
    }
    
    // 애니메이션 진행도 업데이트
    scaleAnimationProgress += 0.02f / scaleAnimationDuration;
    
    // 애니메이션 완료 체크
    if (scaleAnimationProgress >= 1.0f)
    {
        scaleAnimationProgress = 1.0f;
        GetWorld()->GetTimerManager().ClearTimer(scaleAnimationTimer);
        
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Scale animation completed - Playing fire end sound"));
    }
    
    // Ease-Out 곡선을 사용한 부드러운 스케일 변화
    float easedProgress = 1.0f - FMath::Pow(1.0f - scaleAnimationProgress, 3.0f);
    
    // X축은 완전히 0으로, Y/Z축은 30%까지 축소
    float currentXScale = FMath::Lerp(originalScale.X, 0.0f, easedProgress);
    float currentYScale = FMath::Lerp(originalScale.Y, originalScale.Y * 0.3f, easedProgress);
    float currentZScale = FMath::Lerp(originalScale.Z, originalScale.Z * 0.3f, easedProgress);
    
    FVector newScale = FVector(currentXScale, currentYScale, currentZScale);
    fireActor->SetActorScale3D(newScale);
}

void AMolotovCocktail::PlayFireStartSound(const FVector& location)
{
    // 시작 사운드 큐 유효성 검사
    if (fireStartSoundCue == nullptr)
        return;

    // 루프 사운드 재생 위치 저장
    pendingLoopSoundLocation = location;

    // 화염 시작 사운드 재생
    fireStartAudioComponent = UGameplayStatics::SpawnSoundAtLocation
    (
        GetWorld(),
        fireStartSoundCue,
        location,
        FRotator::ZeroRotator,
        1.0f,  // Volume
        1.0f,  // Pitch
        0.0f,  // Start Time
        nullptr,
        nullptr,
        false  // Auto Destroy
    );

    if (fireStartAudioComponent != nullptr)
    {
        // 시작 사운드 90% 지점에서 루프 사운드를 낮은 볼륨으로 시작 (부드러운 전환)
        float soundDuration = fireStartSoundCue->GetDuration();
        float overlapStartTime = soundDuration * 0.9f;

        if (overlapStartTime > 0.0f)
        {
            FTimerHandle overlapStartTimer;
            GetWorld()->GetTimerManager().SetTimer(overlapStartTimer, [this, location]()
            {
                // 루프 사운드를 30% 볼륨으로 시작
                PlayFireLoopSound(location);

                if (fireLoopAudioComponent == nullptr)
                    return;

                fireLoopAudioComponent->SetVolumeMultiplier(0.3f);

                // 0.2초에 걸쳐 풀 볼륨으로 페이드 인
                FTimerHandle VolumeUpTimer;
                GetWorld()->GetTimerManager().SetTimer(VolumeUpTimer, [this]()
                {
                    if (fireLoopAudioComponent == nullptr)
                        return;

                    if (IsValid(fireLoopAudioComponent))
                        fireLoopAudioComponent->SetVolumeMultiplier(1.0f);
                }, 0.2f, false);

            }, overlapStartTime, false);
        }

        // 시작 사운드 완료 콜백 등록
        fireStartAudioComponent->OnAudioFinished.AddDynamic(this, &AMolotovCocktail::OnFireStartSoundFinished);
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("Fire start sound with overlap"));
    }

    // 시작 사운드 재생 실패 시 바로 루프 사운드 재생
    else
        PlayFireLoopSound(location);
}

void AMolotovCocktail::OnFireStartSoundFinished()
{
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Fire start sound finished, starting loop sound"));
    
    // 시작 사운드 완료 후 루프 사운드 재생
    PlayFireLoopSound(pendingLoopSoundLocation);
    
    // 시작 사운드 컴포넌트 정리
    if (fireStartAudioComponent)
        fireStartAudioComponent = nullptr;
}

void AMolotovCocktail::PlayFireLoopSound(const FVector& location)
{
    // 루프 사운드 큐 유효성 검사
    if (fireLoopSoundCue == nullptr)
        return;

    // 기존 루프 사운드가 재생 중이면 정지
    StopFireLoopSound();

    // 루프 사운드 재생을 위한 새 오디오 컴포넌트 생성
    fireLoopAudioComponent = UGameplayStatics::SpawnSoundAtLocation(
        GetWorld(),
        fireLoopSoundCue,
        location,
        FRotator::ZeroRotator,
        1.0f,  // Volume
        1.0f,  // Pitch
        0.0f,  // Start Time
        nullptr,
        nullptr,
        true   // Auto Destroy
    );

    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("Fire loop sound started"));
}

void AMolotovCocktail::PlayFireEndSound(const FVector& location)
{
    // 종료 사운드 큐 유효성 검사
    if (fireEndSoundCue == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("fireEndSoundCue is null in PlayFireEndSound!"));
        return;
    }

    // 화염 종료 사운드 원샷 재생
    UGameplayStatics::PlaySoundAtLocation(GetWorld(), fireEndSoundCue, location);
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("Fire end sound played"));
}

void AMolotovCocktail::StopFireLoopSound()
{
    // 루프 오디오 컴포넌트 유효성 및 재생 상태 검사
    if (fireLoopAudioComponent != nullptr && IsValid(fireLoopAudioComponent))
    {
        if (fireLoopAudioComponent->IsPlaying())
        {
            fireLoopAudioComponent->Stop();
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("Fire loop sound stopped."));
        }

        else
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Fire loop sound was not playing when StopFireLoopSound was called."));

        fireLoopAudioComponent = nullptr; // 컴포넌트 참조 정리
    }

    else
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, TEXT("Fire loop audio component was null or invalid when trying to stop."));
}