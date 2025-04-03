#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Bullet.h"
#include "Gun.generated.h"

UCLASS()
class CONTRACT_API AGun : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGun();

	// Called every frame
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
	int currentAmmoEquipped = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	class UNiagaraComponent* gunMuzzleFireNiagara;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// 발사 함수
	void Fire();
	void StartFire();
	void StopFire();

	void Reload();

	class AActor* player;
	class APlayerController* playerController;
	class UPlayerItem* playerData;

	// 헤더 파일에 타이머 핸들 선언
	FTimerHandle TimerHandle_AutoFire;

};
