#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Bullet.h"
#include "Gun.generated.h"

UCLASS()
class CONTRACT_API AGun : public AActor
{
    GENERATED_BODY()
    
public:	
    AGun();
    virtual void Tick(float DeltaTime) override;

    // 메시 컴포넌트
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    class UStaticMeshComponent* mesh;

    // 총구 컴포넌트
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    class UArrowComponent* muzzle;

    // 총알 Blueprint
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    TSubclassOf<class ABullet> bulletBlueprint;

    // 발사 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    bool isFire;

    // 발사 딜레이
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    float fireRate = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    int maxAmmoEquipped = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    int currentAmmoEquipped = 30; // 기본값 설정

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    class UNiagaraComponent* gunMuzzleFireNiagara;

    // === NPC가 사용할 수 있는 공개 함수들 ===
    
    // 발사 제어 함수
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartFire();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StopFire();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void Fire();

    // 재장전 함수
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void Reload();

    // === NPC 전용 소켓 및 파지 포인트 ===
    
    // 오른손 파지 소켓 (Gun 메시에 추가해야 함)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Setup")
    FName rightHandGripSocketName = TEXT("RightGunTarget");

    // 왼손 보조 파지 소켓 (Gun 메시에 추가해야 함)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Setup")
    FName leftHandGripSocketName = TEXT("LeftGunTarget");

public:
    // 오른손 파지 위치 가져오기 (NPC IK용)
    UFUNCTION(BlueprintCallable, Category = "NPC Setup")
    FTransform GetRightHandGripTransform() const;

    // 왼손 보조 파지 위치 가져오기 (NPC IK용)
    UFUNCTION(BlueprintCallable, Category = "NPC Setup", meta = (BlueprintThreadSafe = "true"))
    FTransform GetLeftHandGripTransform() const;

    // 왼손 IK 트랜스폼 함수 추가
    UFUNCTION(BlueprintCallable, Category = "NPC Setup", meta = (BlueprintThreadSafe = "true"))
    FTransform GetLeftHandIKTransform() const;

    // Gun이 NPC에 장착되었는지 확인
    UFUNCTION(BlueprintCallable, Category = "NPC Setup")
    bool IsEquippedByNPC() const { return bIsEquippedByNPC; }

    // NPC 장착 상태 설정
    UFUNCTION(BlueprintCallable, Category = "NPC Setup")
    void SetEquippedByNPC(bool bEquipped);

    // 소켓 존재 여부 확인 함수 수정
    UFUNCTION(BlueprintCallable, Category = "Debug")
    bool DoesLeftHandSocketExist() const
    {
        return mesh && mesh->DoesSocketExist(leftHandGripSocketName);
    }

    UFUNCTION(BlueprintCallable, Category = "Debug")
    TArray<FName> GetAvailableSockets() const
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

    // 더 간단한 디버그 함수 추가
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void PrintAvailableSockets() const
    {
        if (mesh && mesh->GetStaticMesh())
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

protected:
    virtual void BeginPlay() override;

private:
    // 기존 private 멤버들
    class AActor* player;
    class APlayerController* playerController;
    class UPlayerInventory* playerInventory;

    // 헤더 파일에 타이머 핸들 선언
    FTimerHandle TimerHandle_AutoFire;

    // NPC 장착 상태 플래그
    bool bIsEquippedByNPC = false;
};
