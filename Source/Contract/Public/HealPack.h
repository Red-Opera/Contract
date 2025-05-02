#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "HealPack.generated.h"

// ȸ�� Ÿ���� �����ϴ� ������
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

	void AddHealPack(); // ȸ�� �������� �߰��ϴ� �޼ҵ�

    // ������ ��� �޼ҵ� �������̵�
    virtual void UseItem() override;

    // �ӽ÷� ������ �ִ� ü�� (�����δ� ĳ���Ϳ��� �����;� ��)
    static constexpr int maxHealth = 1000;

    // ���� ü�� (�ӽ÷� ������ ��, �����δ� ĳ���Ϳ��� �����;� ��)
	static int currentHealth; 

    // �� ������ ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int healItemID = 3;

protected:
    virtual void BeginPlay() override;

private:
	void RemoveHealPack(); // ȸ�� ������ ���� �޼ҵ�

    // ȸ�� Ÿ�� (������ �Ǵ� �ۼ�Ʈ ���)
    UPROPERTY(EditAnywhere, Category = "Healing")
    HealingType HealType;

    // ���� ȸ����
    UPROPERTY(EditAnywhere, Category = "Healing", meta = (EditCondition = "HealType == HealingType::Fixed"))
    int fixedHealAmount = 25;

    // �ۼ�Ʈ ��� ȸ���� (0.0 ~ 1.0)
    UPROPERTY(EditAnywhere, Category = "Healing", meta = (EditCondition = "HealType == HealingType::Percentage", ClampMin = "0.0", ClampMax = "1.0"))
    float healPercentage = 0.3f;

    // ȸ�� ȿ����
    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* healSound;

    // ȸ�� ����Ʈ
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UParticleSystem* healEffect;
};