#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "HealPack.generated.h"

// 회복 타입을 정의하는 열거형
UENUM(BlueprintType)
enum class HealingType : uint8
{
    Fixed       UMETA(DisplayName = "Fixed"),
    Percentage  UMETA(DisplayName = "Percentage")
};

UCLASS()
class CONTRACT_API AHealPack : public AItem
{
    GENERATED_BODY()

public:
    AHealPack() = default;

	void AddHealPack(); // 회복 아이템을 추가하는 메소드

    // 아이템 사용 메소드 오버라이드
    virtual void UseItem() override;

    // 임시로 설정한 최대 체력 (실제로는 캐릭터에서 가져와야 함)
    static constexpr int maxHealth = 1000;

    // 현재 체력 (임시로 설정한 값, 실제로는 캐릭터에서 가져와야 함)
	static int currentHealth; 

    // 힐 아이템 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int healItemID = 3;

protected:
    virtual void BeginPlay() override;

private:
	void RemoveHealPack(); // 회복 아이템 제거 메소드

    // 회복 타입 (고정값 또는 퍼센트 기반)
    UPROPERTY(EditAnywhere, Category = "Healing")
    HealingType HealType;

    // 고정 회복량
    UPROPERTY(EditAnywhere, Category = "Healing", meta = (EditCondition = "HealType == HealingType::Fixed"))
    int fixedHealAmount = 25;

    // 퍼센트 기반 회복량 (0.0 ~ 1.0)
    UPROPERTY(EditAnywhere, Category = "Healing", meta = (EditCondition = "HealType == HealingType::Percentage", ClampMin = "0.0", ClampMax = "1.0"))
    float healPercentage = 0.3f;

    // 회복 효과음
    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* healSound;

    // 회복 이펙트
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UParticleSystem* healEffect;
};