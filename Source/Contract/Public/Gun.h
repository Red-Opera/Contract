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

	// �޽� ������Ʈ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	class UStaticMeshComponent* mesh;

	// �ѱ� ������Ʈ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	class UArrowComponent* muzzle;

	// �Ѿ� Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	TSubclassOf<class ABullet> bulletBlueprint;

	// �߻� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	bool isFire;

	// �߻� ������
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
	// �߻� �Լ�
	void Fire();
	void StartFire();
	void StopFire();

	void Reload();

	class AActor* player;
	class APlayerController* playerController;
	class UPlayerItem* playerData;

	// ��� ���Ͽ� Ÿ�̸� �ڵ� ����
	FTimerHandle TimerHandle_AutoFire;

};
