#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FloatingDamage.generated.h"

UCLASS()
class CONTRACT_API AFloatingDamage : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AFloatingDamage();

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class UStaticMeshComponent* mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class UWidgetComponent* damageWidget;

    // 추가: damage 값을 설정하기 위한 함수
    void SetDamageValue(int32 NewDamage)
    {
        damage = NewDamage;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int damage;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

private:
};
