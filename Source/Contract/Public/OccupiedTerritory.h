#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "OccupiedTerritory.generated.h"

// 영역 소유자 타입 열거형
UENUM(BlueprintType)
enum class ETerritoryOwner : uint8
{
    Neutral     UMETA(DisplayName = "중립"),
    Friendly    UMETA(DisplayName = "아군"),
    Enemy       UMETA(DisplayName = "적군")
};

UCLASS()
class CONTRACT_API AOccupiedTerritory : public AActor
{
    GENERATED_BODY()
    
public:	
    AOccupiedTerritory();

protected:
    virtual void BeginPlay() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // 정육면체 메시 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* cubeMesh;

    // 영역 소유자
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
    ETerritoryOwner territoryOwner;

    // 투명도 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float opacity;

private:
    // 소유자에 따른 색깔 업데이트
    void UpdateColorByOwner();

    // 메시에 색상 적용
    void ApplyColorToMesh();

public:	
    // 투명도 변경 함수
    UFUNCTION(BlueprintCallable, Category = "Appearance")
    void SetOpacity(float newOpacity);

    // 영역 소유자 설정 함수
    UFUNCTION(BlueprintCallable, Category = "Territory")
    void SetTerritoryOwner(ETerritoryOwner newOwner);

    // 영역 소유자 가져오기 함수
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Territory")
    ETerritoryOwner GetTerritoryOwner() const { return territoryOwner; }

    // 적의 영역인지 확인
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Territory")
    bool IsEnemyTerritory() const { return territoryOwner == ETerritoryOwner::Enemy; }

    // 아군의 영역인지 확인
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Territory")
    bool IsFriendlyTerritory() const { return territoryOwner == ETerritoryOwner::Friendly; }

    // 중립 영역인지 확인
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Territory")
    bool IsNeutralTerritory() const { return territoryOwner == ETerritoryOwner::Neutral; }
};
